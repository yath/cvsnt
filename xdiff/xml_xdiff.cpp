/*	cvsnt xml xdiff
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <config.h>
#include <system.h>
#include <stdarg.h>
#include <stack>
#include "xdiff.h"
#include "../cvsapi/cvsapi.h"
#include "../diff/diffrun.h"
#include <cvstools.h>
#include "../version.h"

static int (*g_outputFn)(const char *text,size_t len);

struct diffStruct_t
{
	char type;
	size_t start1,end1;
	size_t start2,end2;
	int origStart;
	int size;
};

static int compareTree(CXmlNode *tree1, CXmlNode *tree2,std::vector<std::pair<CXmlNode*,bool> >& changed);
static const char *getPath(const CXmlNode *node, const char *prefix);
static bool nodeEqual(const CXmlNode* node1, const CXmlNode* node2);
static bool walk_tree(CXmlNode *left, CXmlNode *right, int oldline, int& oldmin, int& oldmax, int& newmin, int& newmax);
static bool transform_line_number(CXmlNode *left, CXmlNode *right, size_t& line);
static bool transform_context(CXmlNode *left, CXmlNode *right, const char *line, std::vector<diffStruct_t>& diffArray, diffStruct_t& change);
static bool transform_diff(CXmlNode *left, CXmlNode *right, const char *diff_in, std::vector<diffStruct_t>& diffArray);

static int xdiff_print(const char *fmt, ...)
{
	cvs::string str;
	va_list va;

	va_start(va, fmt);
	cvs::vsprintf(str,256,fmt,va);
	va_end(va);
	g_outputFn(str.c_str(),str.length());
	return str.length();
}

int xdiff_function(const char *name, const char *file1, const char *file2, const char *label1, const char *label2, int argc, const char *const*argv, int (*output_fn)(const char *,size_t))
{
	std::vector<std::pair<CXmlNode*,bool> > changed;
	std::vector<cvs::string> ignore_tag;
	g_outputFn = output_fn;
	bool standard_diff = false;
	CXmlTree tree;

	CTokenLine tok(argc,argv);
	size_t argnum=0;
	CGetOptions opt(tok,argnum,"i:d");
	if(argnum!=argc)
	{
		xdiff_print("Invalid arguments to xdiff.  Aborting.");
		return 1;
	}
	
	for(argnum=0; argnum<opt.count(); argnum++)
	{
		switch(opt[argnum].option)
		{
		case 'i':
			ignore_tag.push_back(opt[argnum].arg);
			break;
		case 'd':
			standard_diff = true;
			break;
		default:
			break;
		}
	}

	FILE *fp1 = fopen(file1,"r");
	if(!fp1)
	{
		xdiff_print("Couldn't open file '%s' (error %d)\n",file1,errno);
		return 1;
	}
	FILE *fp2 = fopen(file2,"r");
	if(!fp2)
	{
		xdiff_print("Couldn't open file '%s' (error %d)\n",file2,errno);
		fclose(fp1);
		return 1;
	}
	cvs::smartptr<CXmlNode> file1Root = tree.ReadXmlFile(fp1,ignore_tag);
	cvs::smartptr<CXmlNode> file2Root = tree.ReadXmlFile(fp2,ignore_tag);
	fclose(fp1);
	fclose(fp2);
	if(!file1Root || !file2Root)
	{
		xdiff_print("Couldn't read file '%s' as valid XML",name);
		return 1;
	}

	if(standard_diff)
	{
//#ifdef _DEBUG
//		cvs::string tempfile1 = "d:\\t\\xml1.out";
//		cvs::string tempfile2 = "d:\\t\\xml2.out";
//		cvs::string difftemp1 = "d:\\t\\diff1.out";
//#else
		cvs::string tempfile1 = CFileAccess::tempfilename("xdiff");
		cvs::string tempfile2 = CFileAccess::tempfilename("xdiff");
		cvs::string difftemp1 = CFileAccess::tempfilename("xdiff");
//#endif
		FILE *f=fopen(tempfile1.c_str(),"w");
		file1Root->WriteXmlFile(f);
		fclose(f);
		f=fopen(tempfile2.c_str(),"w");
		file2Root->WriteXmlFile(f);
		fclose(f);
		CTokenLine diffargs;
		diffargs.addArg("xmldiff");
		diffargs.addArg("-bBNw");
		diffargs.addArg(tempfile1.c_str());
		diffargs.addArg(tempfile2.c_str());
		int ret = diff_run(diffargs.size(),(char**)diffargs.toArgv(),difftemp1.c_str(),NULL);
//#ifndef _DEBUG
		CFileAccess::remove(tempfile1.c_str());
//#endif
		fp2=fopen(tempfile2.c_str(),"r");
		if(!fp2)
		{
			xdiff_print("Internal error - couldn't read partial result file");
//#ifndef _DEBUG
			CFileAccess::remove(tempfile2.c_str());
			CFileAccess::remove(difftemp1.c_str());
//#endif
			return 1;
		}
		file1Root = NULL;
		cvs::smartptr<CXmlNode> temp2Root = tree.ReadXmlFile(fp2,ignore_tag);
		fclose(fp2);

		std::vector<diffStruct_t> diffArray;
		transform_diff(temp2Root,file2Root,difftemp1.c_str(),diffArray);
		temp2Root=NULL;
		file2Root=NULL;
//#ifndef _DEBUG
		CFileAccess::remove(tempfile2.c_str());
		CFileAccess::remove(difftemp1.c_str());
//#endif
		for(size_t n=0; n<diffArray.size(); n++)
		{
			xdiff_print("%c %d %d -> %d %d\n",diffArray[n].type,diffArray[n].start1,diffArray[n].end1,diffArray[n].start2,diffArray[n].end2);
		}
		return ret;
	}
	else
	{
		compareTree(file1Root,file2Root,changed);

		for(size_t i=0; i<changed.size(); i++)
		{
			if(i+1<changed.size() && changed[i].second!=changed[i+1].second && nodeEqual(changed[i].first,changed[i+1].first))
				changed.erase(&changed[i+1]);
			else
				xdiff_print("%s",getPath(changed[i].first,changed[i].second?"- ":"+ "));
		}
		return changed.size()?1:0;
	}
}

static int compareTree(CXmlNode *tree1, CXmlNode *tree2,std::vector<std::pair<CXmlNode*,bool> >& changed)
{
	CXmlNode::ChildArray_t::iterator i = tree1->begin();
	CXmlNode::ChildArray_t::iterator j = tree2->begin();

	while(i!=tree1->end() && j!=tree2->end())
	{
		CXmlNode* c1 = *i;
		CXmlNode* c2 = *j;
		int diff = strcmp(c1->GetName(),c2->GetName());
		if(diff<0)
		{
			changed.push_back(std::pair<CXmlNode*,bool>(c1,true));
			++i;
		}
		else if(diff>0)
		{
			changed.push_back(std::pair<CXmlNode*,bool>(c2,false));
			++j;
		}
		else
		{
			diff = strcmp(c1->GetValue(),c2->GetValue());
			if(diff<0)
			{
				changed.push_back(std::pair<CXmlNode*,bool>(c1,true));
				++i;
			}
			else if(diff>0)
			{
				changed.push_back(std::pair<CXmlNode*,bool>(c2,false));
				++j;
			}
			else
			{
				compareTree(c1,c2,changed);
				++i;
				++j;
			}
		}
	}		
	while(i!=tree1->end())
	{
		CXmlNode* c1 = *i;
		changed.push_back(std::pair<CXmlNode*,bool>(c1,true));
		++i;
	}
	while(j!=tree2->end())
	{
		CXmlNode* c2 = *j;
		changed.push_back(std::pair<CXmlNode*,bool>(c2,false));
		++j;
	}
	return 0;
}

static const char *getPath(const CXmlNode *node, const char *prefix)
{
	std::stack<const CXmlNode*> nodes;
	static cvs::string path;
	int lev;
	char tmp[64];

	do
	{
		nodes.push(node);
		node = node->getParent();
	} while(node);

	path="";

	lev=0;
	while(nodes.size())
	{
		node = nodes.top();
		snprintf(tmp,10,"%5d",node->getStartLine());
		path+=tmp;
		path+=": ";
		path+=prefix;
		path.append(lev,' ');
		path.append("<");
		path+= node->GetName();
		path.append(">");
		nodes.pop();
		if(nodes.size())
			path+= "\n";
		lev++;
	}
	path+=node->GetValue();
	--lev;
	path.append("</");
	path+=node->GetName();
	path.append(">");
	path+="\n";
	while((node=node->getParent())!=NULL)
	{
		--lev;
		snprintf(tmp,10,"%5d",node->getEndLine());
		path+=tmp;
		path+=": ";
		path+=prefix;
		path.append(lev,' ');
		path.append("</");
		path+=node->GetName();
		path.append(">");
		path+="\n";
	}
	return path.c_str();
}

static bool nodeEqual(const CXmlNode* node1, const CXmlNode* node2)
{
	if((!node1 && node2) || (node2 && !node1))
		return false;
	if(!node1 && !node2)
		return true;

	if(strcmp(node1->GetName(),node2->GetName()))
		return false;
	if(strcmp(node1->GetValue(),node2->GetValue()))
		return false;
	return nodeEqual(node1->getParent(),node2->getParent());
}

/* Walk two semantically identical trees and find map source line numbers */
static bool walk_tree(CXmlNode *left, CXmlNode *right, int oldline, int& oldmin, int& oldmax, int& newmin, int& newmax)
{
	CXmlNode::ChildArray_t::iterator i = left->begin();
	CXmlNode::ChildArray_t::iterator j = right->begin();

	while(i!=left->end() && j!=right->end())
	{
		CXmlNode* c1 = *i;
		CXmlNode* c2 = *j;
		if(strcmp(c1->GetName(),c2->GetName()) || c1->size()!=c2->size())
		{
			xdiff_print("Internal error - trees not identical");
		}
		if(strcmp(c1->GetValue(),c2->GetValue()))
		{
//			printf("value different:\n");
//			printf("%s\n-------%s\n",c1->GetValue(),c2->GetValue());
		}
		if(c1->getStartLine()>=oldmin && c1->getEndLine()<=oldmax && c1->getStartLine()<=oldline && c1->getEndLine()>=oldline)
		{
			oldmin=c1->getStartLine();
			oldmax=c1->getEndLine();
			newmin=c2->getStartLine();
			newmax=c2->getEndLine();
		}	
		walk_tree(c1,c2,oldline,oldmin,oldmax,newmin,newmax);
		++i;
		++j;
	}		
	return 0;
}

static bool transform_line_number(CXmlNode *left, CXmlNode *right, size_t& line)
{
	int oldmin,oldmax,newmin,newmax;

	oldmin=-1;
	oldmax=0x7fffffff;

	walk_tree(left,right,line,oldmin,oldmax,newmin,newmax);
	line=(line-oldmin)+newmin;
	return true;
}

static bool transform_context(CXmlNode *left, CXmlNode *right, const char *line, std::vector<diffStruct_t>& diffArray, diffStruct_t& change)
{
	const char *p;
	/* Line is <start>[,<end>]<type><start>[,end] */

	memset(&change,0,sizeof(change));

	p=line;
	while(isdigit(*p)) p++;
	change.start1=atoi(line);
	if(*p==',')
	{
		line=++p;
		while(isdigit(*p)) p++;
		change.end1=atoi(line);
	}
	change.type=*p;
	line=++p;
	while(isdigit(*p)) p++;
	change.start2=atoi(line);
	if(*p==',')
	{
		line=++p;
		while(isdigit(*p)) p++;
		change.end2=atoi(line);
	}

	change.origStart=change.start2;
/*
	for(size_t n=0; n<diffArray.size(); n++)
	{
		if(change.start2>=diffArray[n].origStart)
			change.start2+=diffArray[n].size;
		if(change.end2>=diffArray[n].origStart)
			change.end2+=diffArray[n].size;
	}
*/
	transform_line_number(left,right,change.start1);
	if(change.end1)
		transform_line_number(left,right,change.end1);
	transform_line_number(left,right,change.start2);
	if(change.end2)
		transform_line_number(left,right,change.end2);
/*
	for(size_t n=0; n<diffArray.size(); n++)
	{
		if(change.start2>=diffArray[n].start2)
			change.start2-=diffArray[n].size;
		if(change.end2>=diffArray[n].start2)
			change.end2-=diffArray[n].size;
	}
*/
	return true;
}

static bool transform_diff(CXmlNode *left, CXmlNode *right, const char *diff_in, std::vector<diffStruct_t>& diffArray)
{
	CFileAccess in;
	CFileAccess out;
	size_t n;

	if(!in.open(diff_in,"r"))
		return false;

	cvs::string line;

	while(in.getline(line))
	{
		diffStruct_t change;
		transform_context(left,right,line.c_str(),diffArray,change);
		switch(change.type)
		{
		case 'd':
			n=0;
			in.getline(line);
			if(change.end1)
				for(; n<(change.end1-change.start1) && in.getline(line); n++)
					;
			change.size=-(int)(n+1);
			break;
		case 'a':
			n=0;
			in.getline(line);
			if(change.end2)
				for(; n<(change.end2-change.start2) && in.getline(line); n++)
					;
			change.size=n+1;
			break;
		case 'c':
			n=0;
			in.getline(line);
			if(change.end1)
				for(; n<(change.end1-change.start1) && in.getline(line); n++)
					;
			change.size=-(int)(n+1);
			in.getline(line); // Separator
			n=0;
			in.getline(line);
			if(change.end2)
				for(; n<(change.end2-change.start2) && in.getline(line); n++)
					;
			change.size+=n+1;
			break;
		default:
			xdiff_print("Unknown change type '%c'?",change.type);
			break;
		}
		diffArray.push_back(change);
	}
	return true;
}

static int init(const struct plugin_interface *plugin);
static int destroy(const struct plugin_interface *plugin);
static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param);

static xdiff_interface xml_xdiff =
{
	{
		PLUGIN_INTERFACE_VERSION,
		"XML XDiff handler",CVSNT_PRODUCTVERSION_STRING,"XmlXDiff",
		init,
		destroy,
		get_interface,
		NULL /* Configure */
	},
	xdiff_function
};

static int init(const struct plugin_interface *plugin)
{
	return 0;
}

static int destroy(const struct plugin_interface *plugin)
{
	return 0;
}

static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param)
{
	if(interface_type!=pitXdiff)
		return NULL;

	return (void*)&xml_xdiff;
}

plugin_interface *get_plugin_interface()
{
	return (plugin_interface*)&xml_xdiff;
}

