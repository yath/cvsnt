/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 *
 * Checking permissions for the current user in the specified
 * directory.
 *
 */

#include "cvs.h"
#include "fileattr.h"

#ifndef _WIN32
#include <sys/types.h>
#include <grp.h>
#include <limits.h>
#endif

#ifndef _MAX_PATH
#define _MAX_PATH PATH_MAX
#endif

#include <getline.h>

#include <map>
#include <string>

// Acl mode: 0=none,1=compat,2=normal
int acl_mode = 1;

typedef std::map<cvs::username, int> valid_groups_t;
typedef std::map<cvs::filename, CXmlNode*> directory_cache_t;
static valid_groups_t valid_groups;
static directory_cache_t directory_cache;

static char perms_cache_dir[_MAX_PATH] = {0};
static char *perms_cache = NULL;

int perms_close()
{
	xfree(perms_cache);
	valid_groups.clear();

	for(directory_cache_t::iterator i = directory_cache.begin(); i!=directory_cache.end(); i++)
		delete i->second;
	directory_cache.clear();

	return 0;
}

static void add_valid_group(const char *name)
{
	assert(name);
	TRACE(3,"add_valid_group(%s)",name);
	valid_groups[name]++;
}

/* Add the username to the list of valid names, and then find all the
   groups the user is in and add them to the list of valid names. */
static void get_valid_groups()
{
    char   *filename;
    char   *names;
    char   *name;
    char   *group;
    FILE   *fp;
    char   *linebuf = NULL;
    size_t linebuf_len;

    if (!valid_groups.empty())
		return;

    filename = (char*)xmalloc (strlen (current_parsed_root->directory)
			+ strlen (CVSROOTADM)
			+ strlen (CVSROOTADM_GROUP)
			+ 10);

    strcpy (filename, current_parsed_root->directory);
    strcat (filename, "/" CVSROOTADM "/");
    strcat (filename, CVSROOTADM_GROUP);

    fp = CVS_FOPEN (filename, "r");
    if (fp != NULL)
	{
		while (getline (&linebuf, &linebuf_len, fp) >= 0)
		{
			group = cvs_strtok(linebuf, ":\n");
			if (group == NULL)
				continue;
			names = cvs_strtok(NULL, ":\n");
			if (names == NULL)
				continue;

			name = cvs_strtok(names, ", \t");
			while (name != NULL)
			{
				if(!strcasecmp(name,"admin"))
					error(0,0,"The group 'admin' is automatically assigned to repository administrators");
				if (!usercmp (CVS_Username, name))
				{
					add_valid_group(group);
					break;
				}
				name = cvs_strtok(NULL, ", \t");
			}

			if( linebuf != NULL )
			{
				xfree (linebuf);
				linebuf = NULL;
			}
		}

		if (ferror (fp))
			error (1, errno, "cannot read %s", filename);
		if (fclose (fp) < 0)
			error (0, errno, "cannot close %s", filename);
    }
    xfree(filename);
    /* If we can't open the group file, we just don't do groups. */

	if(verify_admin())
		add_valid_group("admin");

#ifdef _WIN32
	{
		void *grouplist;
		int count = win32_getgroups_begin(&grouplist);
		for(int n=0; n<count; n++)
			add_valid_group(win32_getgroup(grouplist,n));
		win32_getgroups_end(&grouplist);
	}
#else
	{
	  gid_t gr[NGROUPS_MAX];
	  struct group *grn;
	  int ngroups,n;

	   ngroups = getgroups(NGROUPS_MAX,gr);
	   for(n=0; n<ngroups; n++)
	   {
		grn = getgrgid(gr[n]);
		if(grn)
		   add_valid_group(grn->gr_name);
	   }
	}
#endif
}

static int verify_valid_name(const char *name)
{
	TRACE(3,"verify_valid_name(%s)",name);
	if(CVS_Username && !usercmp(name,CVS_Username))
		return 1;
	return valid_groups.find(name)!=valid_groups.end();
}

int verify_admin ()
{
	char   *filename;
	FILE   *fp;
	char   *linebuf = NULL;
	size_t linebuf_len;
	char   *name;
	static int is_the_admin = 0;
	static int admin_valid = 0;

	if(admin_valid)
		return is_the_admin;

	filename = (char*)xmalloc (strlen (current_parsed_root->directory)
		  + strlen ("/CVSROOT")
		  + strlen ("/admin")
		  + 1);

	strcpy (filename, current_parsed_root->directory);
	strcat (filename, "/CVSROOT");
	strcat (filename, "/admin");

	TRACE(1,"Checking admin file %s for user %s",
	   		PATCH_NULL(filename), PATCH_NULL(CVS_Username));

	fp = CVS_FOPEN (filename, "r");
	if (fp != NULL)
	{
		while (getline (&linebuf, &linebuf_len, fp) >= 0)
		{
			name = cvs_strtok(linebuf,"\n");
			if (!usercmp (CVS_Username, name))
			{
				is_the_admin = 1;
				xfree(linebuf);
				break;
			}
			xfree (linebuf);
		}
		if (ferror (fp))
			error (1, errno, "cannot read %s", filename);
		if (fclose (fp) < 0)
			error (0, errno, "cannot close %s", filename);
	}
	xfree(filename);

	if(system_auth && !is_the_admin)
	{
#ifdef WIN32
		if(win32_isadmin())
			is_the_admin = 1;
#else
#ifdef CVS_ADMIN_GROUP
	{
	  struct group *gr;
	  gid_t groups[NGROUPS_MAX];
	  int ngroups,n;

	  gr = getgrnam(CVS_ADMIN_GROUP);
	  if(gr)
	  {
	    ngroups = getgroups(NGROUPS_MAX,groups);
	    for(n=0; n<ngroups; n++)
	    {
	      if(groups[n]==gr->gr_gid)
	      {
	        is_the_admin = 1;
	        break;
	      }
	    }
	  }
	}
#endif
#endif
	}

	admin_valid = 1;
	return is_the_admin;
}

static int cache_directory_permissions(const char *dir)
{
	char *d=xstrdup(dir),*p;
	TRACE(3,"cache_directory_permissions(%s)",dir);
	do
	{
		if(directory_cache.find(d)==directory_cache.end())
		{
			CXmlNode *node=NULL;
			_fileattr_read(node, d);
			if(node)
				directory_cache[d]=node;
			else
				directory_cache[d]=NULL;
		}
		if(fncmp(d,current_parsed_root->directory))
		{
			p=(char*)last_component(d);
			*(--p)='\0';
		}
		else
			break;
	} while (1);
	xfree(d);
	return 0;
}

static int verify_owner (const char *dir)
{
	TRACE(3,"verify_owner(%s)",dir);

	if(verify_admin())
		return 1;

	get_valid_groups();

	cache_directory_permissions(dir);
	if(directory_cache.find(dir)!=directory_cache.end() && directory_cache[dir])
	{
		CXmlNode *n = directory_cache[dir]->Lookup("directory/owner");
		if(n)
			return verify_valid_name(n->GetValue());

	}
	return 0;
}

static void verify_acl(CXmlNode *acl, const char *action, const char *tag, const char *merge, int& user_state, int& group_state, bool first_iteration, const char **message)
{
	TRACE(3,"verify_acl(%s,%s,%s)",action,PATCH_NULL(tag),PATCH_NULL(merge));
	int max_priority = -1;
	while(acl)
	{
		CXmlNode *acl_branch = acl->Lookup("@branch");
		CXmlNode *acl_merge = acl->Lookup("@merge");
		CXmlNode *acl_user = acl->Lookup("@user");
		CXmlNode *acl_message = acl->Lookup("message");
		CXmlNode *acl_priority = acl->Lookup("@priority");
		const char *val_branch = acl_branch?acl_branch->GetValue():"_default_";
		const char *val_merge = acl_merge?acl_merge->GetValue():NULL;
		const char *val_user = acl_user?acl_user->GetValue():NULL;
		const char *val_message = acl_message?acl_message->GetValue():NULL;
		int val_priority = acl_priority?atoi(acl_priority->GetValue()):0;

		if(((!acl_user || verify_valid_name(val_user))) &&
			(!acl_branch || (tag && !strcmp(tag,val_branch))) &&
			(!acl_merge || (merge && !strcmp(merge,val_merge))))
		{
			bool isUser = acl_user && CVS_Username && !usercmp(CVS_Username,val_user);

			TRACE(3,"matched ACL user=%s, branch=%s, merge=%s",val_user?val_user:"",val_branch,val_merge?val_merge:"");

			if(!val_priority)
			{
				if(isUser) val_priority=10;
				else if(acl_user) val_priority=6; // No need to call verify_valid_name
				if(acl_branch && !strcmp(tag,val_branch)) val_priority+=4;
				if(merge && acl_merge && !strcmp(merge,val_merge)) val_priority+=5;
				TRACE(3,"calculated ACL priority is %d",val_priority);
			}
			else
				TRACE(3,"configured ACL priority is %d",val_priority);

			CXmlNode *prm = acl->Lookup(action);
			if(!prm)
				prm = acl->Lookup("all");
			if(val_priority>=max_priority && prm && (isUser || !valid_groups[val_branch]))
			{
				CXmlNode *deny=prm->Lookup("@deny");
				CXmlNode *inherit=prm->Lookup("@inherit");
				max_priority = val_priority;
				TRACE(3,"new max priority is %d",val_priority);
				if(!first_iteration && inherit && !atoi(inherit->GetValue()))
				{
					/* If we've reached a non-inheritable ACL ignore it and
						all ACLs above it. */
					if(isUser)
						user_state=2;
					else
						valid_groups[val_branch]=2;
				}
				else
				{
					if(deny && atoi(deny->GetValue()))
					{
						if(isUser)
						{
							/* user message overrides group message.  We won't get here twice. */
							if(message)
								*message = val_message;
							user_state=-1;
						}
						else
						{
							if(message && !*message)
								*message = val_message;

							group_state = -1;
							valid_groups[val_branch]=1;
						}
					}
					else
					{
						if(isUser)
							user_state = 1;
						else
						{
							group_state = 1;
							valid_groups[val_branch]=1;
						}
					}
				}
			}
		}
		acl = acl->Next();
	}
}

static int verify_perm(const char *dir, const char *file, const char *action, const char *tag, const char *merge, const char **message)
{
	const char *p;
	const char *d;
	int user_state,group_state;

	TRACE(3,"verify_perm(%s,%s,%s,%s,%s)",PATCH_NULL(dir),PATCH_NULL(file),PATCH_NULL(action),PATCH_NULL(tag),PATCH_NULL(merge));
	if(message)
		*message = NULL;

	if(acl_mode == 0)
	{
		TRACE(3,"verify_perm: bypassed because acl_mode == 0");
		return 1;
	}

	if(verify_owner(dir) && !strcmp(action,"control"))
		return 1;

	if(!tag || !*tag)
		tag="HEAD";

   /* The whole cvs code assumes any directory called 'EmptyDir'
	   is invalid wherever it is. */
	p=last_component(dir);
	if(!fncmp(p,CVSNULLREPOS))
		return 1; // Don't do a verify on CVSROOT/Emptydir

	get_valid_groups();

	if(cache_directory_permissions(dir))
	{
		TRACE(3,"Unable to read directory permission cache");
		return 1;
	}

	/* Order:
		1. Exact match (user)
		2. Default match (user)
		3. Exact match (group)
		4. Default match (group)

		Now we walk up the directory tree.
	   We prime the group map, then for each level,
	   set the flag:
			-1 - group has explicit DENY
			0 - no action for group
			1 - group has explicit ALLOW
			2 = noinherit flag set (==0, but don't override)
	   We also do this for the user.

	   Then consolidate the flags:
	      1. If the user flag is explicit ALLOW then ALLOW
		  2. else if the user flag is explict DENY then DENY
	      3. else if there are any group ALLOW then ALLOW
		  4. else if there are any group DENY then DENY
		  5. else if control then DENY
		  6. else ALLOW.

		  Change:  Step 6 depends on the ACL mode, compat = ALLOW, normal = DENY.

	*/
	d = xstrdup(dir);
	for(valid_groups_t::iterator i = valid_groups.begin(); i!=valid_groups.end(); i++)
		i->second=0;
	user_state = group_state = 0;

	bool first_iteration = true;
	if(file)
	{
		directory_cache_t::const_iterator dc = directory_cache.find(d);
		if(dc!=directory_cache.end() && dc->second)
		{
			CXmlNode *acl = _fileattr_find(dc->second,"file[@name=F'%s']/acl",file);
			if(acl)
			{
				TRACE(3,"ACL lookup on file %s",file);
				verify_acl(acl, action, tag, merge, user_state, group_state, true, message);
				TRACE(3,"user_state = %d, group_state = %d", user_state, group_state);
				if((user_state && user_state!=2) || group_state)
					goto match_found; /* Explicit user setting, or group allow */
				TRACE(3,"No match on file ACL");
			}
		}
	}

	do
	{
		directory_cache_t::const_iterator dc = directory_cache.find(d);
		if(dc!=directory_cache.end() && dc->second)
		{
			CXmlNode *acl = dc->second->Lookup("directory/acl");
			int max_priority = -1;
			TRACE(3,"ACL lookup on directory %s",d);
			verify_acl(acl, action, tag, merge, user_state, group_state, first_iteration, message);
		}

		TRACE(3,"user_state = %d, group_state = %d", user_state, group_state);

		if((user_state && user_state!=2) || group_state)
			break; /* Explicit user setting, or group allow */

		TRACE(3,"no match at this level");
		if(fncmp(d,current_parsed_root->directory))
		{
			char *p=(char*)last_component(d);
			*(--p)='\0';
		}
		else
			break;
		first_iteration = false;
	} while (1);

match_found:
	xfree(d);

	if(user_state<0)
		return 0;
	if(user_state>0)
		return 1;
	if(group_state<0)
		return 0;
	if(group_state>0)
		return 1;
	// If in mode 2 (normal) or this is a control action, default DENY
	if(acl_mode == 2 || !strcmp(action,"control"))
		return 0;
	return 1;
}

int verify_read (const char *dir, const char *name, const char *tag, const char **message,const char **type_message)
{
	if(type_message)
		*type_message="read from";
    return verify_perm(dir, name, "read", tag, NULL, message);
}

int verify_write (const char *dir, const char *name, const char *tag, const char **message, const char **type_message)
{
	if(type_message)
		*type_message="write to";
    return verify_perm(dir, name, "write", tag, NULL, message);
}

int verify_create (const char *dir, const char *name, const char *tag, const char **message, const char **type_message)
{
	if(type_message)
		*type_message="create";
    return verify_perm(dir, name, "create", tag, NULL,message);
}

int verify_tag (const char *dir, const char *name, const char *tag, const char **message, const char **type_message)
{
	if(type_message)
		*type_message="tag";
    return verify_perm(dir, name, "tag", tag, NULL, message);
}

int verify_control (const char *dir, const char *name, const char *tag, const char **message, const char **type_message)
{
	if(type_message)
		*type_message="modify permissions for";
    return verify_perm(dir, name, "control", tag, NULL, message);
}

int verify_merge (const char *dir, const char *name, const char *tag, const char *merge, const char **message)
{
    return verify_perm(dir, name, "write", tag, merge, message);
}

int change_owner (const char *user)
{
	fileattr_setvalue(fileattr_create(NULL,"directory/owner"),NULL,user);
	return 0;
}
