/* Implementation for file attribute munging features.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"
#include "getline.h"
#include "fileattr.h"

static const char *stored_repos; /* Current directory */
static CXmlTree g_tree; /* Global xml context */
static CXmlNode *stored_root; /* Node tree of current directory */
static int modified; /* Set when tree changed */

static void fileattr_read();

static void fileattr_convert(CXmlNode *& root, const char *oldfile);
static void owner_convert(CXmlNode *& root, const char *oldfile);
static void perms_convert(CXmlNode *& root, const char *oldfile);
static void fileattr_convert_write(CXmlNode *& root, const char *xmlfile);
static void fileattr_convert_line(CXmlNode *node, char *line);

static char printf_buf[512];

/* Note that if noone calls fileattr_xxx, this is very cheap.  No stat(),
   no open(), no nothing.  */
void fileattr_startdir (const char *repos)
{
    assert (stored_repos == NULL || !fncmp(repos,stored_repos));
	assert (stored_root == NULL);

	stored_repos = xstrdup (repos);
    modified = 0;
	TRACE(3,"fileattr_startdir(%s)",PATCH_NULL(repos));
}

CXmlNode *_fileattr_find(CXmlNode *node, const char *exp, ...)
{
	va_list va;
	va_start(va, exp);
	vsnprintf(printf_buf,sizeof(printf_buf),exp,va);
	va_end(va);

	return (XmlHandle_t)node->Lookup(printf_buf,false);
}

/* Search for a given node.  A little like xpath but not nearly as complex */
XmlHandle_t fileattr_find(XmlHandle_t root, const char *exp, ...)
{
	TRACE(3,"fileattr_find(%s)",exp);
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	va_list va;
	va_start(va, exp);
	vsnprintf(printf_buf,sizeof(printf_buf),exp,va);
	va_end(va);

	return (XmlHandle_t)node->Lookup(printf_buf,false);
}

/* Search for a given node.  A little like xpath but not nearly as complex */
/* This version creates any nodes that aren't in the path */
XmlHandle_t fileattr_create(XmlHandle_t root, const char *exp, ...)
{
	TRACE(3,"fileattr_create(%s)",exp);
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	va_list va;
	va_start(va, exp);
	vsnprintf(printf_buf,sizeof(printf_buf),exp,va);
	va_end(va);

	CXmlNode *ret = node->Lookup(printf_buf,false);
	if(ret)
		return (XmlHandle_t)ret;

	modified = 1;
	return (XmlHandle_t)node->Lookup(printf_buf,true);
}


/* Return the next node on this level with this name, for walking lists */
XmlHandle_t fileattr_next(XmlHandle_t root)
{
	TRACE(3,"fileattr_next()");
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) return NULL;
	CXmlNode *next = node->Next();
	if(!next) return NULL;
	
	if(!strcmp(node->GetName(),next->GetName()))
		return (XmlHandle_t)next;
	return NULL;
}

/* Delete a value under the node.  */
void fileattr_delete(XmlHandle_t root, const char *exp, ...)
{
	TRACE(3,"fileattr_delete(%s)",exp);
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	va_list va;
	va_start(va, exp);
	vsnprintf(printf_buf,sizeof(printf_buf),exp,va);
	va_end(va);

	CXmlNode *child = node->Lookup(printf_buf,false);

	if(child)
	{
		node->Delete(child);
		modified = 1;
	}
}

/* Delete a value under the node.  */
void fileattr_delete_child(XmlHandle_t parent, XmlHandle_t child)
{
	TRACE(3,"fileattr_delete_child()");

	if(!parent)
		return;

	if(child)
	{
		parent->Delete(child);
		modified = 1;
	}
}

/* Delete a value under the node at the next prune.  */
void fileattr_batch_delete(XmlHandle_t root)
{
	TRACE(3,"fileattr_batch_delete()");
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	node->BatchDelete();
	modified = 1;
}

/* Get a single value from a node.  Pass null to get value of this node. */
const char *fileattr_getvalue(XmlHandle_t root, const char *name)
{
	TRACE(3,"fileattr_getvalue(%s)",name);
	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	if(name) node = node->Lookup(name);
	if(node) return node->GetValue();
	return NULL;
}

/* Set a single value for a node.  Pass null to set value of this node. */
void fileattr_setvalue(XmlHandle_t root, const char *name, const char *value)
{
	TRACE(3,"fileattr_setvalue(%s,%s)",name,value?value:"");
	if(!stored_root)
		fileattr_read();

	assert(!name || !strchr(name,'/'));

	CXmlNode *node=(CXmlNode*)root;
	CXmlNode *val;
	if(!node) node = stored_root;

	modified = 1;

	val=node;
	if(name) val = node->Lookup(name);
	if(val) return val->SetValue(value);
	else
	{
		if(name[0]=='@')
			node->NewAttribute(name+1,value);
		else
			node->NewNode(name,value);
	}
}

void fileattr_newfile (const char *filename)
{
	TRACE(3,"fileattr_newfile(%s)",filename);
	if(!stored_root)
		fileattr_read();

	CXmlNode *dir_default = stored_root->Lookup("directory/default");
	if(dir_default)
	{
		CXmlNode *file = stored_root->NewNode("file",NULL);
		file->NewAttribute("name",filename);
		file->Paste(dir_default->Copy());
		modified = 1;
	}
}

void fileattr_write ()
{
	char *fname;
	FILE *fp;

	TRACE(3,"fileattr_write()");
	if(noexec)
		return;
	if(!modified || !stored_root)
		return;

    fname = (char*)xmalloc (strlen (stored_repos)
		     + sizeof (CVSREP_FILEATTR) + 20);

    sprintf(fname,"%s/%s",stored_repos,CVSADM);
    if(!isdir(fname))
      make_directory(fname);

    sprintf(fname,"%s/%s",stored_repos,CVSREP_FILEATTR);

    fp = CVS_FOPEN (fname, FOPEN_BINARY_WRITE);
    if (fp == NULL)
    {
		error (0, errno, "cannot write %s", fn_root(fname));
		xfree (fname);
		return;
    }

	if(!stored_root->WriteXmlFile(fp))
		error (0, errno, "cannot write %s", fn_root(fname));
	xfree(fname);
	fclose(fp);

	modified = 0;
}

void fileattr_free ()
{
	TRACE(3,"fileattr_free()");
	xfree(stored_repos);
	delete stored_root;
	stored_root = NULL;
}

void fileattr_read()
{
	_fileattr_read(stored_root, stored_repos);
	if(!stored_root)
		error(1,0,"Malformed fileattr.xml file in %s/CVS.  Please fix or delete this file",fn_root(stored_repos));
}

void _fileattr_read(CXmlNode*& root, const char *repos)
{
	char *fname,*ofname;
	FILE *fp;

	TRACE(3,"fileattr_read(%s)",repos);

	if(root)
		return; /* Boilerplate, should never happen */

    fname = (char*)xmalloc (strlen (repos)
		     + sizeof (CVSREP_FILEATTR) + 20);

	sprintf(fname,"%s/%s",repos,CVSREP_FILEATTR);

	if(!isfile(fname))
	{
		ofname = (char*)xmalloc (strlen (repos)
				+ sizeof (CVSREP_OLDFILEATTR) + 20);
		sprintf(ofname,"%s/%s",repos,CVSREP_OLDFILEATTR);
		if(isfile(ofname))
		{
			fileattr_convert(root,ofname);
			CVS_UNLINK(ofname);
		}

		sprintf(ofname,"%s/%s",repos,CVSREP_OLDOWNER);
		if(isfile(ofname))
		{
			owner_convert(root,ofname);
			CVS_UNLINK(ofname);
		}

		sprintf(ofname,"%s/%s",repos,CVSREP_OLDPERMS);
		if(isfile(ofname))
		{
			perms_convert(root,ofname);
			CVS_UNLINK(ofname);
		}

		if(root)
		{
			sprintf(ofname,"%s/%s",repos,CVSADM);
			if(!isdir(ofname))
				make_directory(ofname);
			fileattr_convert_write(root,fname);
		}
		else
			root = new CXmlNode(&g_tree,CXmlNode::XmlTypeNode,"fileattr",NULL);

		xfree(ofname);
		xfree(fname);
		return;
	}

    fp = CVS_FOPEN (fname, FOPEN_BINARY_READ);
    if (fp == NULL)
    {
		if (!existence_error (errno))
			error (0, errno, "cannot read %s", fn_root(fname));
		xfree (fname);
		root = new CXmlNode(&g_tree,CXmlNode::XmlTypeNode,"fileattr",NULL);
		return;
    }

	root = g_tree.ReadXmlFile(fp);
	fclose(fp);
	xfree(fname);
}

/* Read an old-style fileattr file and convert it to new-style */
void fileattr_convert(CXmlNode*& root, const char *oldfile)
{
	FILE *fp;
	char *line, *p;
	size_t linesize;
	CXmlNode *node;

	TRACE(3,"fileattr_convert(%s)",oldfile);	
    fp = CVS_FOPEN (oldfile, FOPEN_BINARY_READ);
    if (fp == NULL)
    {
		if (!existence_error (errno))
			error (0, errno, "cannot read %s", fn_root(oldfile));
		return;
    }

	if(!root)
		root = new CXmlNode(&g_tree,CXmlNode::XmlTypeNode,"fileattr",NULL);
	line = NULL;

	while(getline(&line,&linesize,fp)>=0)
	{
		line[linesize-1]='\0';
		if(!*line)
			continue;

		p=line+strlen(line)-1;
		while(isspace(*p))
			*(p--)='\0';

	    p = strchr (line, '\t');
	    if (p == NULL)
			error (1, 0, "file attribute database corruption: tab missing in %s",fn_root(oldfile));
	    *(p++)='\0';

		switch(line[0])
		{
		case 'D':
			node = root->Lookup("directory/default",true);
			fileattr_convert_line(node,p);
			break;
		case 'F':
			node = root->NewNode("file",NULL);
			node->NewAttribute("name",line+1);
			fileattr_convert_line(node,p);
			break;
		default:
			error(0,0,"Unrecognized fileattr type '%c'.  Not converting.",line[0]);
			break;
		}
	}
	
	fclose(fp);
	xfree(line);
}

void owner_convert(CXmlNode *& root, const char *oldfile)
{
	FILE *fp;
	char *line, *p;
	size_t linesize;

	TRACE(3,"owner_convert(%s)",oldfile);	
    fp = CVS_FOPEN (oldfile, FOPEN_BINARY_READ);
    if (fp == NULL)
    {
		if (!existence_error (errno))
			error (0, errno, "cannot read %s", fn_root(oldfile));
		return;
    }

	if(!root)
		root = new CXmlNode(&g_tree,CXmlNode::XmlTypeNode,"fileattr",NULL);

	line = NULL;
	if(getline(&line,&linesize,fp)>=0)
	{
		line[linesize-1]='\0';
		if(*line)
		{
			p=line+strlen(line)-1;
			while(isspace(*p))
				*(p--)='\0';

			CXmlNode *node = root->Lookup("directory/owner",true);
			node->SetValue(line);
		}
	}
	fclose(fp);
	xfree(line);
}

void perms_convert(CXmlNode *& root, const char *oldfile)
{
	FILE *fp;
	char *line, *p;
	size_t linesize;

	TRACE(3,"perms_convert(%s)",oldfile);	
    fp = CVS_FOPEN (oldfile, FOPEN_BINARY_READ);
    if (fp == NULL)
    {
		if (!existence_error (errno))
			error (0, errno, "cannot read %s", fn_root(oldfile));
		return;
    }

	if(!root)
		root = new CXmlNode(&g_tree,CXmlNode::XmlTypeNode,"fileattr",NULL);

	line = NULL;
	while(getline(&line,&linesize,fp)>=0)
	{
		const char *user, *branch=NULL, *perms;
		line[linesize-1]='\0';
		if(!*line)
			continue;

		p=line+strlen(line)-1;
		while(isspace(*p))
			*(p--)='\0';

		p = strchr(line,'{');
		if(p)
		{
			*(p++)='\0';
			branch = p;
			p=strchr(p,'}');
			if(!p)
			{
				error(0,0,"malformed ACL for user %s in directory",user);
				continue;
			}
			*(p++)='\0';
		}
		else
			p=line;
		user = p;
		p=strchr(p,':');
		if(!p)
		{
			error(0,0,"malformed ACL for user %s in directory",user);
			continue;
		}
		*(p++)='\0';
		perms = p;

		CXmlNode *node = root->Lookup("directory",true);
		CXmlNode *acl;
		
		acl = node->NewNode("acl",NULL);
		if(strcmp(user,"default"))
			acl->NewAttribute("user",user);
		if(branch)
			acl->NewAttribute("branch",branch);
		if(strchr(perms,'n'))
		{
			CXmlNode *n;
			n=acl->NewNode("all",NULL);
			n->NewAttribute("deny","1");
		}
		else
		{
			if(strchr(perms,'r'))
				acl->NewNode("read",NULL);
			if(strchr(perms,'w'))
			{
				acl->NewNode("write",NULL);
				acl->NewNode("tag",NULL);
			}
			if(strchr(perms,'c'))
				acl->NewNode("create",NULL);
		}
	}
	fclose(fp);
	xfree(line);
}

void fileattr_convert_write(CXmlNode *& root, const char *xmlfile)
{
	FILE *fp = fopen(xmlfile,FOPEN_BINARY_WRITE);
    if (fp == NULL)
    {
		error (0, errno, "cannot write %s", fn_root(xmlfile));
		return;
	}
	if(!root->WriteXmlFile(fp))
	{
		error (0, errno, "cannot write %s", fn_root(xmlfile));
		return;
	}
	fclose(fp);
}

void fileattr_convert_line(CXmlNode *node, char *line)
{
	char *l = line, *e, *p, *type,*name,*next, *att, *nextatt;
	CXmlNode *subnode;
	do
	{
		p = strchr(l,'=');
		if(!p)
			return;
		*(p++)='\0';
		e=strchr(p,';');
		if(e)
			*(e++)='\0';

		if(!strcmp(l,"_watched"))
		{
			node->NewNode("watched",NULL);
		}
		else if(!strcmp(l,"_watchers"))
		{
			name=p;
			do
			{
				type=strchr(name,'>');
				if(!type)
					 break;
				*(type++)='\0';
				next=strchr(type,',');
				if(next)
					*(next++)='\0';

				subnode=node->NewNode("watcher",NULL);
				subnode->NewAttribute("name",name);
				att=type;
				do
				{
					nextatt=strchr(att,'+');
					if(nextatt)
						*(nextatt++)='\0';
					subnode->NewNode(att,NULL);
					att=nextatt;
				} while(att);
				name = next;
			} while(name);
		}
		else if(!strcmp(l,"_editors"))
		{
			name=p;
			do
			{
				type=strchr(name,'>');
				if(!type)
					 break;
				*(type++)='\0';
				next=strchr(type,',');
				if(next)
					*(next++)='\0';

				subnode=node->NewNode("editor",NULL);
				subnode->NewAttribute("name",name);
				att=type;

				nextatt=strchr(att,'+');
				if(nextatt)
					*(nextatt++)='\0';
				subnode->NewNode("time",att);

				att=nextatt;
				nextatt=strchr(att,'+');
				if(nextatt)
					*(nextatt++)='\0';
				subnode->NewNode("hostname",att);

				att=nextatt;
				nextatt=strchr(att,'+');
				if(nextatt)
					*(nextatt++)='\0';
				subnode->NewNode("pathname",att);

				att=nextatt;
				name = next;
			} while(name);
		}
		else
		{
			error(0,0,"Unknown fileattr attribute '%s'.  Not converting.",l);
		}
		l=e;
	} while(l);
}

void fileattr_prune(XmlHandle_t node)
{
	if(!node)
		return;

	((CXmlNode*)node)->Prune();
}

XmlHandle_t fileattr_copy(XmlHandle_t root)
{
	if(!root)
		return NULL;
	
	return (XmlHandle_t)((CXmlNode*)root)->Copy();
}

void fileattr_paste(XmlHandle_t root, XmlHandle_t source)
{
	if(!source)
		return;

	if(!stored_root)
		fileattr_read();

	CXmlNode *node=(CXmlNode*)root;
	if(!node) node = stored_root;

	node->Paste((CXmlNode*)source);
	modified = 1;
}


void fileattr_free_subtree(XmlHandle_t *root)
{
	if(!root || !*root)
		return;

	delete *(CXmlNode**)root;
	*root=NULL;
}

void fileattr_modified()
{
	modified = 1;
}
