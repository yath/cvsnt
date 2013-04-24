/*
	CVSNT Generic API
    Copyright (C) 2004 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "lib/api_system.h"
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <ctype.h>
#include <string.h>
#include <expat.h>
#include <algorithm>
#include <string>

#include "cvs_string.h"
#include "cvs_smartptr.h"
#include "Codepage.h"
#include "ServerIO.h"
#define INC_EXPAT
#include "Xmlapi.h"

static bool operator==(CXmlNode::ChildArray_t::value_type& a, CXmlNode* b) 
{
	return &(*a)==b; 
};


static bool operator<(const CXmlNode::ChildArray_t::value_type& a, const std::pair<CXmlNode::XmlTypeEnum,const char *>& b) 
{
	if(a->getType()==CXmlNode::XmlTypeAttribute && b.first!=CXmlNode::XmlTypeAttribute)
		return true;
	if(a->getType()!=CXmlNode::XmlTypeAttribute && b.first==CXmlNode::XmlTypeAttribute)
		return false;
	return strcmp(a->GetName(),b.second)<0?true:false;
};

CXmlNode::CXmlNode(const CXmlNode& other)
{
	name = other.name;
	text = other.text;
	Parent = other.Parent; //?? Is this ever true?
	XmlType = other.XmlType;
	StartLineNumber = other.StartLineNumber;
	EndLineNumber = other.EndLineNumber;
	Children = other.Children;
	keyNum = other.keyNum;
	sorted = other.sorted;

	for(ChildArray_t::iterator i=Children.begin(); i!=Children.end(); ++i)
		(*i)->Parent = this;
}

CXmlNode *CXmlNode::_New(XmlTypeEnum Type, const char *name, const char *value)
{
	/* Can't add child node to attribute */
	if(this->XmlType==XmlTypeAttribute)
		return NULL;

	Children.push_back(new CXmlNode(m_tree));
	sorted=false;
	CXmlNode *n = Children[Children.size()-1];

	n->XmlType=Type;
	n->name=name;
	if(value)
		n->SetValue(value);
	n->Parent=this;
	return n;
}

/* very simple xpath-like syntax.  Basically all we understand is '/' and '@',
which is enough for interrogating the tree.  */
/* Extended by =F and =U for filename and username comparisons */
CXmlNode *CXmlNode::Lookup(const char *path, bool autoCreate /* = false */)
{
	ChildArray_t::iterator i,j;
	const char *p;
	char *q,*r,*s,*tag;
	static char tmp[256];
	bool isattr;
	char qt;
	int (*cmp)(const char *,const char *);

	for(p=path,qt=0; *p && (qt || *p!='/'); p++)
	{
		if(qt && (qt=='\\' || *p==qt))
			qt=0;
		else if(*p=='"' || *p=='\'' || *p=='\\')
			qt=*p;
	}

	if((p-path)<sizeof(tmp))
	{
		memcpy(tmp,path,p-path);
		tmp[p-path]='\0';
	}
	else
	{
		/* Overflow case.. normally wouldn't happen */
		strncpy(tmp,path,sizeof(tmp)-1);
		tmp[sizeof(tmp)-1]='\0';
	}

	q=strchr(tmp,'[');
	s=NULL;
	if(q && *(q+1)=='@' && ((r=strchr(q+2,'='))!=NULL) && ((s=strchr(r+1,']'))!=NULL))
	{
		*q='\0';
		q+=2;
		*(r++)='\0';
		*s='\0';
		if(*r=='U' && *(r+1)=='\'')
		{
			cmp=usercmp;
			r++;
		}
		else if(*r=='F' && *(r+1)=='\'')
		{
			cmp=fncmp;
			r++;
		}
		else
			cmp=strcmp;

		if(*r=='\'' && *(s-1)=='\'')
		{
			*(r++)='\0';
			*(--s)='\0';
		}
	}

	tag=tmp; isattr=false;
	if(!*p && tag[0]=='@') { tag++; isattr=true; }
	SortMe();
	i=std::lower_bound(Children.begin(),Children.end(),std::pair<CXmlNode::XmlTypeEnum,const char *>(isattr?XmlTypeAttribute:XmlTypeNode,tag));
	if(i!=Children.end() && !strcmp(tag,(*i)->name.c_str()))
	{	
		do
		{
			if(!isattr && q && s)
			{
				(*i)->SortMe();
				j=std::lower_bound((*i)->Children.begin(),(*i)->Children.end(),std::pair<CXmlNode::XmlTypeEnum,const char *>(XmlTypeAttribute,q));
				if(j==(*i)->Children.end() || strcmp(q,(*j)->name.c_str()) )
					continue;
				if(cmp((*j)->GetValue(),r))
					continue;
			}
			if(*p)
			{
				CXmlNode *node = (*i)->Lookup(p+1,autoCreate);
				if(node)
					return node;
			}
			else
			{
				if((isattr && (*i)->XmlType!=XmlTypeAttribute) ||
					(!isattr && (*i)->XmlType!=XmlTypeNode))
					break;
				return *i;
			}
		} while(++i!=Children.end() && (*i)->name==tmp);
	}

	if(autoCreate)
	{
		CXmlNode *node = NewNode(tmp,NULL);
		if(q && r)
			node->NewAttribute(q,r);
		if(*p)
			return node->Lookup(p+1,autoCreate);
		else
			return node;
	}
	return NULL;
}

void CXmlTree::startElement(void *userData, const char *name, const char **atts)
{
	CXmlTree *tree=(CXmlTree*)userData;
	CXmlNode *node,*attnode;
	int line;

	CXmlNode *parent = tree->m_lastNode;

	if(tree->m_ignore_level)
	{
		tree->m_ignore_level++;
		parent->text.append("<");
		parent->text.append(name);
		parent->text.append(">");
		return;
	}

	line = XML_GetCurrentLineNumber(tree->m_parser);

	if(parent)
		node = parent->NewNode(name,NULL);
	else
		node = new CXmlNode(tree,CXmlNode::XmlTypeNode,name,NULL);

	node->StartLineNumber = line;
	while(*atts)
	{
		void *att = NULL;
		size_t attlen;
		if(tree->m_cp.ConvertEncoding(atts[1],strlen(atts[1])+1,att,attlen))
		{
			attnode = node->NewAttribute(atts[0],(const char *)att);
			free(att);
		}
		else
			attnode = node->NewAttribute(atts[0],atts[1]);
		attnode->StartLineNumber = attnode->EndLineNumber = line;
		atts+=2;
	}
	tree->m_lastNode = node;

	/* If the tag is 'ignored' we treat everything within it as literal */
	if(std::find(tree->m_ignore_tag.begin(),tree->m_ignore_tag.end(),name)!=tree->m_ignore_tag.end())
		tree->m_ignore_level++;
}

void CXmlTree::endElement(void *userData, const char *name)
{
	CXmlTree *tree=(CXmlTree*)userData;
	int line;
	size_t n;

	CXmlNode *parent = tree->m_lastNode;

	if(tree->m_ignore_level && --tree->m_ignore_level)
	{
		parent->text.append("</");
		parent->text.append(name);
		parent->text.append(">");
		return;
	}

	line = XML_GetCurrentLineNumber(tree->m_parser);
	parent->EndLineNumber = line;

	for(n=0; n<parent->text.length(); n++)
	{
		if(!isspace(parent->text[n]))
			break;
	}
	if(n==parent->text.length())
		parent->text=""; // GCC 2.95 fix - broken STL doesn't support clear

	parent->SortMe();

	if(parent->Parent) /* Theoretically this should be set unless we're at the root node (end of parsing) */
		tree->m_lastNode = parent->Parent;
}

void CXmlTree::charData(void *userData, const char * s, int len)
{
	CXmlTree *tree=(CXmlTree*)userData;
	void *str = NULL;
	size_t strLen;
	CXmlNode *node = tree->m_lastNode;

	if(tree->m_cp.ConvertEncoding(s,len,str,strLen))
	{
		node->AppendValue((const char*)str,strLen);
		free(str);
	}
	else
		node->AppendValue((const char *)s,len);
}

int CXmlTree::getEncoding(void *userData, const XML_Char *name, XML_Encoding *info)
{
	return 0;
}

CXmlNode *CXmlTree::ReadXmlFile(FILE * file)
{
	std::vector<cvs::string> ignore_tag;
	return ReadXmlFile(file,ignore_tag);
}

CXmlNode *CXmlTree::ParseXmlFromMemory(const char *data)
{
	std::vector<cvs::string> ignore_tag;
	return ParseXmlFromMemory(data,ignore_tag);
}

CXmlNode *CXmlTree::ReadXmlFile(FILE * file, std::vector<cvs::string>& ignore_tag)
{
	XML_Parser parser;
	char buf[BUFSIZ];
	int done;
	const char *enc;

	m_ignore_tag = ignore_tag;
	m_ignore_level=0;

	fgets(buf,sizeof(buf),file);
	if(strstr(buf,"encoding=\"UTF-8\""))
		enc="UTF-8";
	else
		enc="ISO-8859-1";
	fseek(file,0,0);
	m_lastNode = NULL;
	parser = XML_ParserCreate(enc);

	m_cp.BeginEncoding(CCodepage::Utf8Encoding,CCodepage::NullEncoding);
	m_cp.SetBytestream();

	m_parser = parser;

	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, charData);
	XML_SetUnknownEncodingHandler(parser, getEncoding, NULL);

	done = 0;

	do {
		size_t len = fread(buf, 1, sizeof(buf), file);
		done = len < sizeof(buf);
		if (!XML_Parse(parser, buf, (int)len, done))
		{
			CServerIo::error("Error in xml_read: %s at line %d\n",
					XML_ErrorString(XML_GetErrorCode(parser)),
					XML_GetCurrentLineNumber(parser));
			delete m_lastNode;
			m_cp.EndEncoding();
			return NULL;
		}
	} while (!done);
	XML_ParserFree(parser);
	m_cp.EndEncoding();

	return m_lastNode;
}

CXmlNode *CXmlTree::ParseXmlFromMemory(const char *data,std::vector<cvs::string>& ignore_tag)
{
	XML_Parser parser;
	int done;
	const char *enc;

	m_ignore_tag = ignore_tag;
	m_ignore_level=0;

	if(strstr(data,"encoding=\"UTF-8\""))
		enc="UTF-8";
	else
		enc="ISO-8859-1";
	m_lastNode = NULL;
	parser = XML_ParserCreate(enc);

	m_cp.BeginEncoding(CCodepage::Utf8Encoding,CCodepage::NullEncoding);
	m_cp.SetBytestream();

	m_parser = parser;

	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, charData);
	XML_SetUnknownEncodingHandler(parser, getEncoding, NULL);

	done = 0;

	if (!XML_Parse(parser, data, (int)strlen(data), 1))
	{
		CServerIo::error("Error in xml_read: %s at line %d\n",
					XML_ErrorString(XML_GetErrorCode(parser)),
					XML_GetCurrentLineNumber(parser));
		delete m_lastNode;
		m_cp.EndEncoding();
		return NULL;
	}

	XML_ParserFree(parser);
	m_cp.EndEncoding();

	return m_lastNode;
}

bool CXmlNode::WriteXmlFile(FILE *file) const
{
	m_tree->m_cp.BeginEncoding(CCodepage::NullEncoding,CCodepage::Utf8Encoding);
	m_tree->m_cp.SetBytestream();
	fprintf(file,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	return WriteXmlNode(file,0);
}

bool CXmlNode::WriteXmlToString(cvs::string& string) const
{
	m_tree->m_cp.BeginEncoding(CCodepage::NullEncoding,CCodepage::Utf8Encoding);
	m_tree->m_cp.SetBytestream();
	string.reserve(1024);
	cvs::sprintf(string,64,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	return WriteXmlNodeToString(string,0);
}

bool CXmlNode::WriteXmlNode(FILE *file, int indent) const
{
	ChildArray_t::const_iterator i;
	int count;
	
	for(int n=0; n<indent; n++)
		fprintf(file,"  ");
	fprintf(file,"<%s",name.c_str());

	for(i=Children.begin(); i!=Children.end(); ++i)
	{
		if((*i)->name.empty())
			continue;

		if((*i)->XmlType==XmlTypeAttribute)
		{
			if((*i)->text.empty())
				fprintf(file," %s",(*i)->name.c_str());
			else
			{
				void *val = NULL;
				size_t vallen;
				std::string strval;
				std::string::size_type off;

				if(m_tree->m_cp.ConvertEncoding((*i)->text.c_str(),(*i)->text.length()+1,val,vallen))
					strval = (const char *)val;
				else
					strval = (*i)->text;

				/* We do &, because expat barfs otherwise, and ", because it's the end of the string */
				off=(std::string::size_type)-1;
				while((off=strval.find('&',off+1))!=std::string::npos)
					strval.replace(off,1,"&amp;");

				off=(std::string::size_type)-1;
				while((off=strval.find('"',off+1))!=std::string::npos)
					strval.replace(off,1,"&quot;");

				fprintf(file," %s=\"%s\"",(*i)->name.c_str(),strval.c_str());
				free(val);
			}
		}
	}
	count=0;
	for(i=Children.begin(); i!=Children.end(); ++i)
	{
		if((*i)->XmlType==XmlTypeNode)
			count++;
	}
	if(!count && text.empty())
	{
		fprintf(file," />\n");
	}
	else
	{
		if(count)
		{
			fprintf(file,">\n");
			for(i=Children.begin(); i!=Children.end(); ++i)
			{
				if((*i)->XmlType==XmlTypeNode)
					(*i)->WriteXmlNode(file,indent+1);
			}
			for(int n=0; n<indent; n++)
				fprintf(file,"  ");
			fprintf(file,"</%s>\n",name.c_str());
		}
		else
		{
			void *val = NULL;
			size_t vallen;
			std::string strval;
			std::string::size_type off;
			if(m_tree->m_cp.ConvertEncoding(text.c_str(),text.length()+1,val,vallen))
				strval = (const char *)val;
			else
				strval = text;

			/* We do &, because expat barfs otherwise, and <, because it's the start
			   of the next token */
			off=(std::string::size_type)-1;
			while((off=strval.find('&',off+1))!=std::string::npos)
				strval.replace(off,1,"&amp;");

			off=(std::string::size_type)-1;
			while((off=strval.find('<',off+1))!=std::string::npos)
				strval.replace(off,1,"&lt;");

			fprintf(file,">%s</%s>\n",strval.c_str(),name.c_str());
			free(val);
		}
	}
	return true;
}

bool CXmlNode::WriteXmlNodeToString(cvs::string& string, int indent) const
{
	ChildArray_t::const_iterator i;
	int count;
	
	for(int n=0; n<indent; n++)
		string+="  ";
	string+='<';
	string+=name.c_str();

	for(i=Children.begin(); i!=Children.end(); ++i)
	{
		if((*i)->name.empty())
			continue;

		if((*i)->XmlType==XmlTypeAttribute)
		{
			if((*i)->text.empty())
			{
				string+=' ';
				string+=(*i)->name.c_str();
			}
			else
			{
				void *val = NULL;
				size_t vallen;
				std::string strval;
				std::string::size_type off;

				if(m_tree->m_cp.ConvertEncoding((*i)->text.c_str(),(*i)->text.length()+1,val,vallen))
					strval = (const char *)val;
				else
					strval = (*i)->text;

				/* We do &, because expat barfs otherwise, and ", because it's the end of the string */
				off=(std::string::size_type)-1;
				while((off=strval.find('&',off+1))!=std::string::npos)
					strval.replace(off,1,"&amp;");

				off=(std::string::size_type)-1;
				while((off=strval.find('"',off+1))!=std::string::npos)
					strval.replace(off,1,"&quot;");

				string+=' ';
				string+=(*i)->name.c_str();
				string+="=\"";
				string+=strval.c_str();
				string+="\"";
				free(val);
			}
		}
	}
	count=0;
	for(i=Children.begin(); i!=Children.end(); ++i)
	{
		if((*i)->XmlType==XmlTypeNode)
			count++;
	}
	if(!count && text.empty())
	{
		string+=" />\n";
	}
	else
	{
		if(count)
		{
			string+=">\n";
			for(i=Children.begin(); i!=Children.end(); ++i)
			{
				if((*i)->XmlType==XmlTypeNode)
					(*i)->WriteXmlNodeToString(string,indent+1);
			}
			for(int n=0; n<indent; n++)
				string+="  ";
			string+="</";
			string+=name.c_str();
			string+=">\n";
		}
		else
		{
			void *val = NULL;
			size_t vallen;
			std::string strval;
			std::string::size_type off;
			if(m_tree->m_cp.ConvertEncoding(text.c_str(),text.length()+1,val,vallen))
				strval = (const char *)val;
			else
				strval = text;

			/* We do &, because expat barfs otherwise, and <, because it's the start
			   of the next token */
			off=(std::string::size_type)-1;
			while((off=strval.find('&',off+1))!=std::string::npos)
				strval.replace(off,1,"&amp;");

			off=(std::string::size_type)-1;
			while((off=strval.find('<',off+1))!=std::string::npos)
				strval.replace(off,1,"&lt;");

			string+='>';
			string+=strval.c_str();
			string+="</";
			string+=name.c_str();
			string+=">\n";
			free(val);
		}
	}
	return true;
}

CXmlNode *CXmlNode::Next()
{
	ChildArray_t::iterator i;

	if(!Parent)
		return NULL;

	i = Parent->FindIterator(this);
	if(i==Parent->Children.end())
		return NULL;
	++i;
	if(i==Parent->Children.end())
		return NULL;
	if((*i)->Parent!=Parent)
		return NULL;
	return *i;
}

CXmlNode *CXmlNode::Previous()
{
	ChildArray_t::iterator i;

	if(!Parent)
		return NULL;

	i = Parent->FindIterator(this);
	if(i==Parent->Children.end())
		return NULL;
	if(i==Parent->Children.begin())
		return NULL;
	if((*i)->Parent!=Parent)
		return NULL;
	--i;
	return *i;
}

bool CXmlNode::Delete(CXmlNode *child)
{
	ChildArray_t::iterator i;

	i = FindIterator(child);
	if(i!=Children.end())
		Children.erase(i);
	return true;
}

bool CXmlNode::BatchDelete()
{
	name=""; /* Having no name marks the tag for deletion */
	return true;
}

/* For some reason this doesn't work in the class (seems to be VC bug).  Make
   global static instead */
static bool operator==(std::pair<const std::string,CXmlNode>& a,CXmlNode *b)
{ 
	return &a.second == b;
}

CXmlNode::ChildArray_t::iterator CXmlNode::FindIterator(CXmlNode *node)
{
	return std::find(Children.begin(),Children.end(),node);
}

bool CXmlNode::Prune()
{
	ChildArray_t::iterator i;

	size_t count = 0;
	for(i=Children.begin(); i!=Children.end(); )
	{
		if((*i)->name.empty())
			Children.erase(i);
		else
		{ 
			if((*i)->XmlType==XmlTypeNode)
				count++;
			++i;
		}
	}

	if(!Parent)
		return true; /* We never prune the root node */

	CXmlNode *node = Parent;
	if(!count)
		node->Delete(this);
	return node->Prune();
}

CXmlNode *CXmlNode::Copy()
{
	CXmlNode *newNode = new CXmlNode(*this);
	return newNode;
}

bool CXmlNode::Paste(CXmlNode *from)
{

	text = from->text;
	std::copy(from->Children.begin(),from->Children.end(),std::inserter(Children,Children.end()));

	for(ChildArray_t::iterator i=Children.begin(); i!=Children.end(); ++i)
		(*i)->Parent = this;

	return true;
}

int CXmlNode::cmp(CXmlNode* b)
{
	/* Attributes always stack first.. looks nicer in the resultant file */
	if(XmlType==XmlTypeAttribute && b->XmlType==XmlTypeNode)
		return -1;
	if(XmlType==XmlTypeNode && b->XmlType==XmlTypeAttribute)
		return 1;

	int diff = strcmp(name.c_str(),b->name.c_str());
	if(!diff)
		diff = strcmp(text.c_str(),b->text.c_str());
	return diff;
}

bool CXmlNode::sortPred(ChildArray_t::value_type a, ChildArray_t::value_type b)
{
	int diff = a->cmp(b);
	if(!diff)
	{
		ChildArray_t::const_iterator i = a->begin();
		ChildArray_t::const_iterator j = b->begin();
		while(!diff && i!=a->end() && j!=b->end())
		{
			diff = (*i)->cmp(*j);
			i++;
			j++;
		}
		if(!diff)
			diff = (int)(a->size() - b->size());
	}
	return diff<0;
}

bool CXmlNode::SortMe()
{
	if(sorted)
		return true;

	for(ChildArray_t::iterator i = Children.begin(); i!=Children.end(); i++)
		(*i)->SortMe();

	std::sort(Children.begin(),Children.end(),sortPred);
	sorted = true;
	return true;
}

CXmlTree::CXmlTree()
{
}

CXmlTree::~CXmlTree()
{
}
