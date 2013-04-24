/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Entries file to Files file
 * 
 * Creates the file Files containing the names that comprise the project, from
 * the Entries file.
 */

#include "cvs.h"
#include "getline.h"

static Node *AddEntryNode (List * list, Entnode *entnode);

static Entnode *fgetentent(FILE *, char *, int *);
static int fgetententex(List *entries, FILE *, char *);
int   fputentent(FILE *, Entnode *);
int   fputententex(FILE *, Entnode *);

static Entnode *subdir_record (int, const char *, const char *);

static FILE *entfile, *entexfile;
static char *entfilename, *entexfilename;		/* for error messages */

/*
 * Construct an Entnode
 */
static Entnode *Entnode_Create(
    enum ent_type type,
    const char *user,
    const char *vn,
    const char *ts,
    const char *options,
    const char *tag,
    const char *date,
    const char *ts_conflict,
	const char *merge_from_tag_1,
	const char *merge_from_tag_2,
	time_t rcs_timestamp,
	const char *edit_revision,
	const char *edit_tag,
	const char *edit_bugid)
{
    Entnode *ent;
    
    /* Note that timestamp and options must be non-NULL */
    ent = (Entnode *) xmalloc (sizeof (Entnode));
    ent->type      = type;
    ent->user      = xstrdup (user);
    ent->version   = xstrdup (vn);
    ent->timestamp = xstrdup (ts ? ts : "");
    ent->options   = xstrdup (options ? options : "");
    ent->tag       = xstrdup (tag);
    ent->date      = xstrdup (date);
    ent->conflict  = xstrdup (ts_conflict);
	ent->rcs_timestamp = rcs_timestamp;
    ent->merge_from_tag_1 = xstrdup(merge_from_tag_1?merge_from_tag_1:"");
    ent->merge_from_tag_2 = xstrdup(merge_from_tag_2?merge_from_tag_2:"");
	ent->edit_revision = xstrdup(edit_revision?edit_revision:"");
	ent->edit_tag = xstrdup(edit_tag?edit_tag:"");
	ent->edit_bugid = xstrdup(edit_bugid?edit_bugid:"");

    return ent;
}

/*
 * Destruct an Entnode
 */

static void Entnode_Destroy (Entnode *ent)
{
    xfree (ent->user);
    xfree (ent->version);
    xfree (ent->timestamp);
    xfree (ent->options);
    xfree (ent->tag);
    xfree (ent->date);
    xfree (ent->conflict);
    xfree (ent->merge_from_tag_1);
    xfree (ent->merge_from_tag_2);
	xfree (ent->edit_revision);
	xfree (ent->edit_tag);
	xfree (ent->edit_bugid);

    xfree (ent);
}

/*
 * Write out the line associated with a node of an entries file
 */
static int write_ent_proc (Node *node, void *closure)
{
    Entnode *entnode;

    entnode = (Entnode *) node->data;

    if (closure != NULL && entnode->type != ENT_FILE)
		*(int *) closure = 1;

    if (fputentent(entfile, entnode))
		error (1, errno, "cannot write %s", entfilename);

    return (0);
}

/*
 * Write out the line associated with a node of an entries.extra file
 */
static int write_ent_ex_proc (Node *node, void *closure)
{
    Entnode *entnode;

    entnode = (Entnode *) node->data;

    if (closure != NULL && entnode->type != ENT_FILE)
	*(int *) closure = 1;

    if (fputentent(entfile, entnode))
		error (1, errno, "cannot write %s", entfilename);
    if (fputententex(entexfile, entnode))
		error (1, errno, "cannot write %s", entexfilename);

    return (0);
}

/*
 * write out the current entries file given a list,  making a backup copy
 * first of course
 */
static void write_entries (List *list)
{
    int sawdir;

    sawdir = 0;

    /* open the new one and walk the list writing entries */
    entfilename = CVSADM_ENTBAK;
    entfile = CVS_FOPEN (entfilename, "w+");
    if (entfile == NULL)
    {
	/* Make this a warning, not an error.  For example, one user might
	   have checked out a working directory which, for whatever reason,
	   contains an Entries.Log file.  A second user, without write access
	   to that working directory, might want to do a "cvs log".  The
	   problem rewriting Entries shouldn't affect the ability of "cvs log"
	   to work, although the warning is probably a good idea so that
	   whether Entries gets rewritten is not an inexplicable process.  */
	/* FIXME: should be including update_dir in message.  */
	error (0, errno, "cannot rewrite %s", entfilename);

	/* Now just return.  We leave the Entries.Log file around.  As far
	   as I know, there is never any data lying around in 'list' that
	   is not in Entries.Log at this time (if there is an error writing
	   Entries.Log that is a separate problem).  */
	return;
    }

	entexfilename= CVSADM_ENTEXTBAK;
    entexfile = CVS_FOPEN (entexfilename, "w+");
    if (entexfile == NULL)
    {
	/* Make this a warning, not an error.  For example, one user might
	   have checked out a working directory which, for whatever reason,
	   contains an Entries.Log file.  A second user, without write access
	   to that working directory, might want to do a "cvs log".  The
	   problem rewriting Entries shouldn't affect the ability of "cvs log"
	   to work, although the warning is probably a good idea so that
	   whether Entries gets rewritten is not an inexplicable process.  */
	/* FIXME: should be including update_dir in message.  */
	error (0, errno, "cannot rewrite %s", entfilename);

	/* Now just return.  We leave the Entries.Log file around.  As far
	   as I know, there is never any data lying around in 'list' that
	   is not in Entries.Log at this time (if there is an error writing
	   Entries.Log that is a separate problem).  */
	return;
    }

	(void) walklist (list, write_ent_ex_proc, (void *) &sawdir);
    if (! sawdir)
    {
	struct stickydirtag *sdtp;

	/* We didn't write out any directories.  Check the list
           private data to see whether subdirectory information is
           known.  If it is, we need to write out an empty D line.  */
	sdtp = (struct stickydirtag *) list->list->data;
	if (sdtp == NULL || sdtp->subdirs)
	    if (fprintf (entfile, "D\n") < 0)
		error (1, errno, "cannot write %s", entfilename);
    }
    if (fclose (entfile) == EOF)
	error (1, errno, "error closing %s", entfilename);
    if (fclose (entexfile) == EOF)
	error (1, errno, "error closing %s", entfilename);

    /* now, atomically (on systems that support it) rename it */
	/* First make a copy in the .Old files (note we don't rename so that the
	   entries files always exist) */
	copy_file (CVSADM_ENT, CVSADM_ENTOLD, 1, 0);
    rename_file (entfilename, CVSADM_ENT);
	copy_file (CVSADM_ENTEXT, CVSADM_ENTEXTOLD, 1, 0);
    rename_file (entexfilename, CVSADM_ENTEXT);

    /* now, remove the log file */
    if (unlink_file (CVSADM_ENTLOG) < 0
	&& !existence_error (errno))
		error (0, errno, "cannot remove %s", CVSADM_ENTLOG);
    /* now, remove the log file */
    if (unlink_file (CVSADM_ENTEXTLOG) < 0
	&& !existence_error (errno))
		error (0, errno, "cannot remove %s", CVSADM_ENTEXTLOG);
}

/*
 * Removes the argument file from the Entries file if necessary.
 */
void Scratch_Entry (List *list, const char *fname)
{
    Node *node;

	TRACE(1,"Scratch_Entry(%s)",PATCH_NULL(fname));

    /* hashlookup to see if it is there */
    if ((node = findnode_fn (list, fname)) != NULL)
    {
		if (!noexec)
		{
			entfilename = CVSADM_ENTLOG;
			entexfilename = CVSADM_ENTEXTLOG;
			entfile = CVS_FOPEN (entfilename, "a");
			entexfile = CVS_FOPEN (entexfilename, "a");

			if (fprintf (entfile, "R ") < 0)
				error (1, errno, "cannot write %s", entfilename);
			if (fprintf (entexfile, "R ") < 0)
				error (1, errno, "cannot write %s", entexfilename);

			write_ent_proc (node, NULL);
			write_ent_ex_proc (node, NULL);

			if (fclose (entfile) == EOF)
				error (1, errno, "error closing %s", entfilename);
			if (fclose (entexfile) == EOF)
				error (1, errno, "error closing %s", entfilename);
		}

		delnode (node);			/* delete the node */

#ifdef SERVER_SUPPORT
		if (server_active)
			server_scratch (fname);
#endif
    }
}

void Rename_Entry (List *list, const char *from, const char *to)
{
    Node *node,*newnode;
	Entnode *ent;

	TRACE(1,"Rename_Entry(%s,%s)",PATCH_NULL(from),PATCH_NULL(to));

    /* hashlookup to see if it is there */
    if ((node = findnode_fn (list, from)) != NULL)
    {
		if (!noexec)
		{
			entfilename = CVSADM_ENTLOG;
			entexfilename = CVSADM_ENTEXTLOG;
			entfile = CVS_FOPEN (entfilename, "a");
			entexfile = CVS_FOPEN (entexfilename, "a");

			ent = (Entnode*)node->data;

			if (fprintf (entfile, "R ") < 0)
				error (1, errno, "cannot write %s", entfilename);
			if (fprintf (entexfile, "R ") < 0)
				error (1, errno, "cannot write %s", entexfilename);

			write_ent_proc (node, NULL);
			write_ent_ex_proc (node, NULL);

			xfree(ent->user);
			ent->user = xstrdup(to);

			if (fprintf (entfile, "A ") < 0)
				error (1, errno, "cannot write %s", entfilename);
			if (fprintf (entexfile, "A ") < 0)
				error (1, errno, "cannot write %s", entexfilename);

			write_ent_proc (node, NULL);
			write_ent_ex_proc (node, NULL);

			if (fclose (entfile) == EOF)
				error (1, errno, "error closing %s", entfilename);
			if (fclose (entexfile) == EOF)
				error (1, errno, "error closing %s", entfilename);
		}

		newnode = getnode();
		newnode->key=xstrdup(to);
		newnode->data=node->data; /* Has been renamed above */
		newnode->delproc=node->delproc;
		node->data=NULL;
		node->delproc=NULL;

		delnode (node);			/* delete the old node */
		addnode (list, newnode); /* Add the new node */

#ifdef SERVER_SUPPORT
//		if (server_active)
//			server_rename (from,to);
#endif
    }
	else
		error(0,0,"%s not in entries file?",from);
}

/* Just create an entry in the entries file, do nothing else */
Node *Fast_Register (List *list, const char *fname, const char *vn,  const char *ts, const char *options, const char *tag,
    const char *date, const char *ts_conflict, const char *merge_from_tag_1,
	const char *merge_from_tag_2, time_t rcs_timestamp, 
	const char *edit_revision, const char *edit_tag, const char *edit_bugid)
{
	Entnode *entnode;
	Node *node;
    entnode = Entnode_Create(ENT_FILE, fname, vn, ts, options, tag, date,
			      ts_conflict,merge_from_tag_1,merge_from_tag_2,rcs_timestamp, edit_revision, edit_tag, edit_bugid);
    node = AddEntryNode (list, entnode);
	return node;
}

/*
 * Enters the given file name/version/time-stamp into the Entries file,
 * removing the old entry first, if necessary.
 */
void Register (List *list, const char *fname, const char *vn, const char *ts, const char *options, const char *tag,
    const char *date, const char *ts_conflict, const char *merge_from_tag_1,
	const char *merge_from_tag_2, time_t rcs_timestamp, 
	const char *edit_revision, const char *edit_tag, const char *edit_bugid)
{
    Entnode *entnode;
    Node *node;

	/* Silently disallow registering of CVS subdirectories */
	if(!fncmp(fname,CVSADM))
		return;
	if(!list)
		return;

#ifdef SERVER_SUPPORT
    if (server_active)
		server_register (fname, vn, ts, options, tag, date, ts_conflict, merge_from_tag_1, merge_from_tag_2, rcs_timestamp, edit_revision, edit_tag, edit_bugid);
#endif

	TRACE(1,"Register(%s, %s, %s%s%s, %s, %s %s, %s, %s, %s, %s)",
			PATCH_NULL(fname), PATCH_NULL(vn), ts ? ts : "",
			ts_conflict ? "+" : "", ts_conflict ? ts_conflict : "",
			PATCH_NULL(options), tag ? tag : "", date ? date : "",
			merge_from_tag_1 ? merge_from_tag_1:"",
			merge_from_tag_2 ? merge_from_tag_2:"", 
			edit_revision ? edit_revision:"",
			edit_tag ? edit_tag:"",
			edit_bugid ? edit_bugid:"");

    entnode = Entnode_Create(ENT_FILE, fname, vn, ts, options, tag, date,
			      ts_conflict,merge_from_tag_1,merge_from_tag_2,rcs_timestamp, edit_revision, edit_tag, edit_bugid);
    node = AddEntryNode (list, entnode);

    if (!noexec)
    {
		entfilename = CVSADM_ENTLOG;
		entexfilename = CVSADM_ENTEXTLOG;
		entfile = CVS_FOPEN (entfilename, "a");
		entexfile = CVS_FOPEN (entexfilename, "a");

		if (entfile == NULL)
		{
			/* Warning, not error, as in write_entries.  */
			/* FIXME-update-dir: should be including update_dir in message.  */
			error (0, errno, "cannot open %s", entfilename);
			return;
		}
		if (entexfile == NULL)
		{
			/* Warning, not error, as in write_entries.  */
			/* FIXME-update-dir: should be including update_dir in message.  */
			error (0, errno, "cannot open %s", entfilename);
			return;
		}

		if (fprintf (entfile, "A ") < 0)
			error (1, errno, "cannot write %s", entfilename);
		if (fprintf (entexfile, "A ") < 0)
			error (1, errno, "cannot write %s", entfilename);

		write_ent_proc (node, NULL);
		write_ent_ex_proc (node, NULL);

		if (fclose (entfile) == EOF)
			error (1, errno, "error closing %s", entfilename);
		if (fclose (entexfile) == EOF)
			error (1, errno, "error closing %s", entexfilename);
    }
}

/*
 * Node delete procedure for list-private sticky dir tag/date info
 */
static void freesdt (Node *p)
{
    struct stickydirtag *sdtp = (struct stickydirtag *) p->data;
	xfree (sdtp->tag);
	xfree (sdtp->date);
    xfree (sdtp);
}

/* Return the next real Entries line.  On end of file, returns NULL.
   On error, prints an error message and returns NULL.  */

static Entnode *fgetentent(FILE *fpin, char *cmd, int *sawdir)
{
    Entnode *ent;
    char *line;
    size_t line_chars_allocated;
    register char *cp;
    enum ent_type type;
    char *l, *user, *vn, *ts, *options;
    char *tag_or_date, *tag, *date, *ts_conflict;
    int line_length;

    line = NULL;
    line_chars_allocated = 0;

    ent = NULL;
    while ((line_length = getline (&line, &line_chars_allocated, fpin)) > 0)
    {
	l = line;

	/* If CMD is not NULL, we are reading an Entries.Log file.
	   Each line in the Entries.Log file starts with a single
	   character command followed by a space.  For backward
	   compatibility, the absence of a space indicates an add
	   command.  */
	if (cmd != NULL)
	{
	    if (l[1] != ' ')
		*cmd = 'A';
	    else
	    {
		*cmd = l[0];
		l += 2;
	    }
	}

	type = ENT_FILE;

	if (l[0] == 'D')
	{
	    type = ENT_SUBDIR;
	    *sawdir = 1;
	    ++l;
	    /* An empty D line is permitted; it is a signal that this
	       Entries file lists all known subdirectories.  */
	}

	if (l[0] != '/')
	    continue;

	user = l + 1;
	if ((cp = strchr (user, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	if(!fncmp(user,CVSADM))
		continue;
	vn = cp;
	if ((cp = strchr (vn, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	ts = cp;
	if ((cp = strchr (ts, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	if(cp[0]=='-' && cp[1]=='k')
		options = cp + 2;
	else
		options = cp;
	if ((cp = strchr (options, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	tag_or_date = cp;
	if ((cp = strchr (tag_or_date, '\n')) == NULL)
	    continue;
	*cp = '\0';
	tag = (char *) NULL;
	date = (char *) NULL;
	if (*tag_or_date == 'T')
	{
	    tag = tag_or_date + 1;
		if(!*tag || !RCS_check_tag(tag,false,false,true))
			tag=NULL;
	}
	else if (*tag_or_date == 'D')
	{
	    date = tag_or_date + 1;
		if(!*date)
			date=NULL;
	}

	if ((ts_conflict = strchr (ts, '+')))
	    *ts_conflict++ = '\0';
	    
	/*
	 * XXX - Convert timestamp from old format to new format.
	 *
	 * If the timestamp doesn't match the file's current
	 * mtime, we'd have to generate a string that doesn't
	 * match anyways, so cheat and base it on the existing
	 * string; it doesn't have to match the same mod time.
	 *
	 * For an unmodified file, write the correct timestamp.
	 */
	{
	    struct stat sb;
	    if (strlen (ts) > 30 && CVS_STAT (user, &sb) == 0)
	    {
		char *c = ctime (&sb.st_mtime);

		if (!strncmp (ts + 25, c, 24))
		    ts = time_stamp (user, 0);
		else
		{
		    ts += 24;
		    ts[0] = '*';
		}
	    }
	}

	ent = Entnode_Create (type, user, vn, ts, options, tag, date,
			      ts_conflict, NULL, NULL, (time_t)-1, NULL, NULL, NULL);
	break;
    }

    if (line_length < 0 && !feof (fpin))
	error (0, errno, "cannot read entries file");

    xfree (line);
    return ent;
}

/* Merge the Entries.Extra data */
static int fgetententex(List *entries, FILE *fpin, char *cmd)
{
    Entnode *ent;
	Node *node;
    char *line;
    size_t line_chars_allocated;
    register char *cp;
    char *l, *user, *tag1, *tag2, *rcs_timestamp_string, *edit_revision, *edit_tag, *edit_bugid;
    int line_length;

    line = NULL;
    line_chars_allocated = 0;

    ent = NULL;
    while ((line_length = getline (&line, &line_chars_allocated, fpin)) > 0)
    {
	l = line;

	/* If CMD is not NULL, we are reading an Entries.Log file.
	   Each line in the Entries.Log file starts with a single
	   character command followed by a space.  For backward
	   compatibility, the absence of a space indicates an add
	   command.  */
	if (cmd != NULL)
	{
	    if (l[1] != ' ')
			*cmd = 'A';
	    else
	    {
			*cmd = l[0];
			l += 2;
	    }
	}

	if (l[0] == 'D')
	{
	    ++l;
	}

	if (l[0] != '/')
	    continue;
	
	user = l + 1;
	if ((cp = strchr (user, '/')) == NULL)
	    continue;
	if(!fncmp(user,CVSADM))
		continue;
	*cp++ = '\0';
	
	node = findnode_fn (entries, user);
	if(!node)
		continue;
	ent = (Entnode*)node->data;

	tag1 = cp;
	if ((cp = strchr (tag1, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	tag2=cp;
	if ((cp = strchr (tag2, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	
	xfree(ent->merge_from_tag_1);
	ent->merge_from_tag_1=xstrdup(tag1);
	xfree(ent->merge_from_tag_2);
	ent->merge_from_tag_2=xstrdup(tag2);

	if(!*cp)
	   break;
	
	rcs_timestamp_string = cp;
	if ((cp = strchr (rcs_timestamp_string, '/')) == NULL)
	   continue;
	*cp++ = '\0';

	if(sscanf(rcs_timestamp_string,"%"TIME_T_SPRINTF"d",&ent->rcs_timestamp)!=1)
		ent->rcs_timestamp=(time_t)-1;
	
	if(!*cp)
		break;

	edit_revision=cp;
	if ((cp = strchr (edit_revision, '/')) == NULL)
		continue;
	*cp++ = '\0';

	xfree(ent->edit_revision);
	ent->edit_revision=xstrdup(edit_revision);

	if(!*cp)
		break;

	edit_tag=cp;
	if ((cp = strchr (edit_tag, '/')) == NULL)
		continue;
	*cp++ = '\0';

	xfree(ent->edit_tag);
	ent->edit_tag=xstrdup(edit_tag);

	if(!*cp)
		break;

	edit_bugid=cp;
	if ((cp = strchr (edit_bugid, '/')) == NULL)
		continue;
	*cp++ = '\0';

	xfree(ent->edit_bugid);
	ent->edit_bugid=xstrdup(edit_bugid);

	break;
	}

	xfree(line);
	return ent?0:-1;
}

int fputentent(FILE *fp, Entnode *p)
{
    switch (p->type)
    {
    case ENT_FILE:
        break;
    case ENT_SUBDIR:
        if (fprintf (fp, "D") < 0)
	    return 1;
	break;
    }

    if (fprintf (fp, "/%s/%s/%s", p->user, p->version, p->timestamp) < 0)
	return 1;
    if (p->conflict)
    {
	if (fprintf (fp, "+%s", p->conflict) < 0)
	    return 1;
    }
	if(p->options[0] && strcmp(p->options,"kv"))
	{
		if (fprintf (fp, "/-k%s/", p->options) < 0)
		return 1;
	}
	else
	{
		if (fprintf (fp, "//", p->options) < 0)
		return 1;
	}

    if (p->tag)
    {
	if (fprintf (fp, "T%s\n", p->tag) < 0)
	    return 1;
    }
    else if (p->date)
    {
	if (fprintf (fp, "D%s\n", p->date) < 0)
	    return 1;
    }
    else 
    {
	if (fprintf (fp, "\n") < 0)
	    return 1;
    }

    return 0;
}

int fputententex(FILE *fp, Entnode *p)
{
    switch (p->type)
    {
    case ENT_FILE:
        break;
    case ENT_SUBDIR:
        if (fprintf (fp, "D") < 0)
		    return 1;
	break;
    }

	if (fprintf (fp, "/%s/%s/%s/", p->user,p->merge_from_tag_1?p->merge_from_tag_1:"",p->merge_from_tag_2?p->merge_from_tag_2:"") < 0)
		return 1;
	if(p->rcs_timestamp!=(time_t)-1)
	{
		fprintf (fp, "%"TIME_T_SPRINTF"d", p->rcs_timestamp);
	}
	fprintf(fp,"/%s/%s/%s/\n",p->edit_revision?p->edit_revision:"",p->edit_tag?p->edit_tag:"",p->edit_bugid?p->edit_bugid:"");

    return 0;
}

List *Entries_Open_Dir (int aflag, const char *dir, const char *update_dir)
{
	const char *lastdir=xgetwd();
	List *list;

	if(CVS_CHDIR(dir))
		error(1,errno,"Couldn't chdir to %s",fn_root(dir));
	list = Entries_Open(aflag,update_dir);
	if(CVS_CHDIR(lastdir))
		error(1,errno,"Couldn't chdir to %s",fn_root(lastdir));
	xfree(lastdir);
	return list;
}

/* Read the entries file into a list, hashing on the file name.

   UPDATE_DIR is the name of the current directory, for use in error
   messages, or NULL if not known (that is, noone has gotten around
   to updating the caller to pass in the information).  

   dir is the directory relative to this one, or NULL */
List *Entries_Open (int aflag, const char *update_dir)
{
    List *entries;
    struct stickydirtag *sdtp = NULL;
    Entnode *ent;
    const char *dirtag, *dirdate;
    int dirnonbranch;
    int do_rewrite = 0;
    FILE *fpin;
    int sawdir;

    /* get a fresh list... */
    entries = getlist ();

    /*
     * Parse the CVS/Tag file, to get any default tag/date settings. Use
     * list-private storage to tuck them away for Version_TS().
     */
	ParseTag (&dirtag, &dirdate, &dirnonbranch, NULL);
    if (aflag || dirtag || dirdate)
    {
	sdtp = (struct stickydirtag *) xmalloc (sizeof (*sdtp));
	memset ((char *) sdtp, 0, sizeof (*sdtp));
	sdtp->aflag = aflag;
	sdtp->tag = xstrdup (dirtag);
	sdtp->date = xstrdup (dirdate);
	sdtp->nonbranch = dirnonbranch;

	/* feed it into the list-private area */
	entries->list->data = (char *) sdtp;
	entries->list->delproc = freesdt;
    }

    sawdir = 0;

    fpin = CVS_FOPEN (CVSADM_ENT, "r");
    if (!fpin && (!noexec || !existence_error(errno)))
    {
		if (update_dir != NULL)
			error (0, 0, "in directory %s:", update_dir);
		error (0, errno, "cannot open %s for reading", CVSADM_ENT);
    }
    else if(fpin)
    {
	while ((ent = fgetentent (fpin, (char *) NULL, &sawdir)) != NULL) 
	{
	    (void) AddEntryNode (entries, ent);
	}

	if (fclose (fpin) < 0)
	    /* FIXME-update-dir: should include update_dir in message.  */
	    error (0, errno, "cannot close %s", CVSADM_ENT);
    }

    fpin = CVS_FOPEN (CVSADM_ENTEXT, "r");
    if (fpin == NULL && !existence_error(errno))
    {
		if (update_dir != NULL)
			error (0, 0, "in directory %s:", update_dir);
		error (0, errno, "cannot open %s for reading", CVSADM_ENTEXT);
    }
    else if(fpin)
    {
		while (!fgetententex (entries,fpin,NULL)) 
			;
		if (fclose (fpin) < 0)
		    /* FIXME-update-dir: should include update_dir in message.  */
		    error (0, errno, "cannot close %s", CVSADM_ENTEXT);
    }

	fpin = CVS_FOPEN (CVSADM_ENTLOG, "r");
    if (fpin != NULL) 
    {
	char cmd;
	Node *node;

	while ((ent = fgetentent (fpin, &cmd, &sawdir)) != NULL)
	{
	    switch (cmd)
	    {
	    case 'A':
		(void) AddEntryNode (entries, ent);
		break;
	    case 'R':
		node = findnode_fn (entries, ent->user);
		if (node != NULL)
		    delnode (node);
		Entnode_Destroy (ent);
		break;
	    default:
		/* Ignore unrecognized commands.  */
	        break;
	    }
	}
	do_rewrite = 1;
	if (fclose (fpin) < 0)
	    /* FIXME-update-dir: should include update_dir in message.  */
	    error (0, errno, "cannot close %s", CVSADM_ENTLOG);
    }

    /* Update the list private data to indicate whether subdirectory
       information is known.  Nonexistent list private data is taken
       to mean that it is known.  */
    if (sdtp != NULL)
	sdtp->subdirs = sawdir;
    else if (! sawdir)
    {
	sdtp = (struct stickydirtag *) xmalloc (sizeof (*sdtp));
	memset ((char *) sdtp, 0, sizeof (*sdtp));
	sdtp->subdirs = 0;
	entries->list->data = (char *) sdtp;
	entries->list->delproc = freesdt;
    }

    if (do_rewrite && !noexec)
	write_entries (entries);

    /* clean up and return */
    if (dirtag)
	xfree (dirtag);
    if (dirdate)
	xfree (dirdate);
    return (entries);
}

void Entries_Close_Dir (List *list, const char *dir)
{
	const char *lastdir=xgetwd();
	if(CVS_CHDIR(dir))
		error(1,errno,"Couldn't chdir to %s",fn_root(dir));
	Entries_Close(list);
	if(CVS_CHDIR(lastdir))
		error(1,errno,"Couldn't chdir to %s",fn_root(lastdir));
	xfree(lastdir);
}

void Entries_Close(List *list)
{
    if (list)
    {
		if (!noexec) 
		{
			if (isfile (CVSADM_ENTLOG))
				write_entries (list);
		}
		dellist(&list);
    }
}


/*
 * Free up the memory associated with the data section of an ENTRIES type
 * node
 */
static void Entries_delproc (Node *node)
{
    Entnode *p;

    p = (Entnode *) node->data;
    Entnode_Destroy(p);
}

/*
 * Get an Entries file list node, initialize it, and add it to the specified
 * list
 */
static Node *AddEntryNode(List *list, Entnode *entdata)
{
    Node *p;

    /* was it already there? */
    if ((p  = findnode_fn (list, entdata->user)) != NULL)
    {
		/* Special case... if new entry contains an edit revision of '*', then
		   we copy the edit data from the old revision */
		if(p->data && entdata->edit_revision && !strcmp(entdata->edit_revision,"*"))
		{
			Entnode *oldent = (Entnode*)p->data;

			xfree(entdata->edit_revision);
			xfree(entdata->edit_tag);
			xfree(entdata->edit_bugid);
			entdata->edit_revision = oldent->edit_revision;
			entdata->edit_tag = oldent->edit_tag;
			entdata->edit_bugid = oldent->edit_bugid;
			oldent->edit_revision = NULL;
			oldent->edit_tag = NULL;
			oldent->edit_bugid = NULL;
		}
		/* take it out */
		delnode (p);
    }

    /* get a node and fill in the regular stuff */
    p = getnode ();
    p->type = ENTRIES;
    p->delproc = Entries_delproc;

    /* this one gets a key of the name for hashing */
    /* FIXME This results in duplicated data --- the hash package shouldn't
       assume that the key is dynamically allocated.  The user's free proc
       should be responsible for freeing the key. */
    p->key = xstrdup (entdata->user);
    p->data = (char *) entdata;

    /* put the node into the list */
    addnode (list, p);
    return (p);
}

/*
 * Write out/Clear the CVS/Tag file.
 */
void WriteTag(const char *dir, const char *tag, const char *date, int nonbranch, const char *update_dir, const char *repository, const char *vers)
{
    FILE *fout;
    char *tmp;

	if(!vers)
		vers=get_directory_version();

    if (noexec)
	return;

    tmp = (char*)xmalloc ((dir ? strlen (dir) : 0)
		   + sizeof (CVSADM_TAG)
		   + 10);
    if (dir == NULL)
	(void) strcpy (tmp, CVSADM_TAG);
    else
	(void) sprintf (tmp, "%s/%s", dir, CVSADM_TAG);

	if(tag||date||vers)
	{
		fout = open_file (tmp, "w+");
		if (tag)
		{
			if (nonbranch)
			{
			if (fprintf (fout, "N%s\n", tag) < 0)
				error (1, errno, "write to %s failed", tmp);
			}
			else
			{
				if(vers)
				{
					if (fprintf (fout, "T%s:%s\n", tag, vers) < 0)
						error (1, errno, "write to %s failed", tmp);
				}
				else
				{
					if (fprintf (fout, "T%s\n", tag) < 0)
						error (1, errno, "write to %s failed", tmp);
				}
			}
		}
		else if(date)
		{
			if (fprintf (fout, "D%s\n", date) < 0)
			error (1, errno, "write to %s failed", tmp);
		}
		else
		{
			if (fprintf (fout, "T:%s\n", vers) < 0)
			error (1, errno, "write to %s failed", tmp);
		}
		if (fclose (fout) == EOF)
			error (1, errno, "cannot close %s", tmp);
	}
	else
	{
		CVS_UNLINK(tmp);
	}
    xfree (tmp);
#ifdef SERVER_SUPPORT
    if (server_active && (!vers || strcmp(vers,"_H_")))
		server_set_sticky(update_dir, repository, tag, date, nonbranch, vers);
#endif
}

/* Parse the CVS/Tag file for the current directory.

   If it contains a date, sets *DATEP to the date in a newly malloc'd
   string, *TAGP to NULL, and *NONBRANCHP to an unspecified value.

   If it contains a branch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 0, and *DATEP to NULL.

   If it contains a nonbranch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 1, and *DATEP to NULL.

   If it does not exist, or contains something unrecognized by this
   version of CVS, set *DATEP and *TAGP to NULL and *NONBRANCHP to
   an unspecified value.

   If there is an error, print an error message, set *DATEP and *TAGP
   to NULL, and return.  */
void ParseTag (const char **tagp, const char **datep, int *nonbranchp, const char **version)
{
	ParseTag_Dir(NULL,tagp,datep,nonbranchp,version);
}

/* Parse the CVS/Tag file for a directory.

   If it contains a date, sets *DATEP to the date in a newly malloc'd
   string, *TAGP to NULL, and *NONBRANCHP to an unspecified value.

   If it contains a branch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 0, and *DATEP to NULL.

   If it contains a nonbranch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 1, and *DATEP to NULL.

   If it does not exist, or contains something unrecognized by this
   version of CVS, set *DATEP and *TAGP to NULL and *NONBRANCHP to
   an unspecified value.

   If there is an error, print an error message, set *DATEP and *TAGP
   to NULL, and return.  */
void ParseTag_Dir(const char *dir, const char **tagp, const char **datep, int *nonbranchp, const char **versionp)
{
    FILE *fp;
	char *p;
    cvs::string fn;

    if (tagp)
		*tagp = (char *) NULL;
    if (datep)
		*datep = (char *) NULL;
	if (versionp)
		*versionp = (char *) NULL;

    /* Always store a value here, even in the 'D' case where the value
       is unspecified.  Shuts up tools which check for references to
       uninitialized memory.  */
    if (nonbranchp != NULL)
		*nonbranchp = 0;
    if(dir)
    {
	cvs::sprintf(fn,80,"%s/%s",dir,CVSADM_TAG);
    }
    else
    {
	    fn=CVSADM_TAG;
    }
    fp = CVS_FOPEN(fn.c_str(),"r");
    if (fp)
    {
		char *line;
		int line_length;
		size_t line_chars_allocated;

		line = NULL;
		line_chars_allocated = 0;

		if ((line_length = getline (&line, &line_chars_allocated, fp)) > 0)
		{
			/* Remove any trailing newline.  */
			if (line[line_length - 1] == '\n')
				line[--line_length] = '\0';
			switch (*line)
			{
			case 'T':
				if(line[1]==':')
				{
					if(versionp != NULL)
						*versionp = xstrdup(line + 2);
				}
				else
				{
					if (tagp != NULL)
					{
						*tagp = xstrdup (line + 1);
						p=strchr(*tagp,':');
					}
					else
						p=strchr(line+1,':');

					if(p)
					{
						*p='\0';
						if(versionp)
							*versionp=xstrdup(p+1);
					}
				}
				break;
			case 'D':
				if (datep != NULL)
					*datep = xstrdup (line + 1);
				break;
			case 'N':
				if (tagp != NULL)
					*tagp = xstrdup (line + 1);
				if (nonbranchp != NULL)
					*nonbranchp = 1;
				break;
			default:
				/* Silently ignore it; it may have been
				written by a future version of CVS which extends the
				syntax.  */
				break;
			}
		}

		if (line_length < 0)
		{
			/* FIXME-update-dir: should include update_dir in messages.  */
			if (feof (fp))
			error (0, 0, "cannot read %s: end of file", CVSADM_TAG);
			else
			error (0, errno, "cannot read %s", CVSADM_TAG);
		}

		if (fclose (fp) < 0)
			/* FIXME-update-dir: should include update_dir in message.  */
			error (0, errno, "cannot close %s", CVSADM_TAG);

		xfree (line);
    }
    else if (!existence_error (errno))
	{
		/* FIXME-update-dir: should include update_dir in message.  */
		error (0, errno, "cannot open %s", fn_root(fn.c_str()));
	}
}

/*
 * This is called if all subdirectory information is known, but there
 * aren't any subdirectories.  It records that fact in the list
 * private data.
 */

void Subdirs_Known (List *entries)
{
    struct stickydirtag *sdtp;

    /* If there is no list private data, that means that the
       subdirectory information is known.  */
    sdtp = (struct stickydirtag *) entries->list->data;
    if (sdtp != NULL && ! sdtp->subdirs)
    {
	FILE *fp;

	sdtp->subdirs = 1;
	if (!noexec)
	{
	    /* Create Entries.Log so that Entries_Close will do something.  */
	    entfilename = CVSADM_ENTLOG;
	    fp = CVS_FOPEN (entfilename, "a");
	    if (fp == NULL)
	    {
		int save_errno = errno;

		/* As in subdir_record, just silently skip the whole thing
		   if there is no CVSADM directory.  */
		if (! isdir (CVSADM))
		    return;
		error (1, save_errno, "cannot open %s", entfilename);
	    }
	    else
	    {
		if (fclose (fp) == EOF)
		    error (1, errno, "cannot close %s", entfilename);
	    }
	}
    }
}

/* Record subdirectory information.  */

static Entnode *subdir_record (int cmd, const char *parent, const char *dir)
{
    Entnode *entnode;

    /* None of the information associated with a directory is
       currently meaningful.  */
    entnode = Entnode_Create (ENT_SUBDIR, dir, "", "", "",
			      (char *) NULL, (char *) NULL,
			      (char *) NULL, (char *) NULL,
			      (char *) NULL, (time_t) -1, NULL, NULL, NULL);

    if (!noexec)
    {
	if (parent == NULL)
	    entfilename = CVSADM_ENTLOG;
	else
	{
	    entfilename = (char*)xmalloc (strlen (parent)
				   + sizeof CVSADM_ENTLOG
				   + 10);
	    sprintf (entfilename, "%s/%s", parent, CVSADM_ENTLOG);
	}

	entfile = CVS_FOPEN (entfilename, "a");
	if (entfile == NULL)
	{
	    int save_errno = errno;

	    /* It is not an error if there is no CVS administration
               directory.  Permitting this case simplifies some
               calling code.  */

	    if (parent == NULL)
	    {
		if (! isdir (CVSADM))
		    return entnode;
	    }
	    else
	    {
		sprintf (entfilename, "%s/%s", parent, CVSADM);
		if (! isdir (entfilename))
		{
		    xfree (entfilename);
		    entfilename = NULL;
		    return entnode;
		}
	    }

	    error (1, save_errno, "cannot open %s", entfilename);
	}

	if (fprintf (entfile, "%c ", cmd) < 0)
	    error (1, errno, "cannot write %s", entfilename);

	if (fputentent (entfile, entnode) != 0)
	    error (1, errno, "cannot write %s", entfilename);

	if (fclose (entfile) == EOF)
	    error (1, errno, "error closing %s", entfilename);

	if (parent != NULL)
	{
	    xfree (entfilename);
	    entfilename = NULL;
	}
    }

    return entnode;
}

/*
 * Record the addition of a new subdirectory DIR in PARENT.  PARENT
 * may be NULL, which means the current directory.  ENTRIES is the
 * current entries list; it may be NULL, which means that it need not
 * be updated.
 */

void Subdir_Register (List *entries, const char *parent, const char *dir)
{
    Entnode *entnode;

    /* Ignore attempts to register ".".  These can happen in the
       server code.  */
    if (dir[0] == '.' && dir[1] == '\0')
	return;

    entnode = subdir_record ('A', parent, dir);

    if (entries != NULL && (parent == NULL || strcmp (parent, ".") == 0))
	(void) AddEntryNode (entries, entnode);
    else
	Entnode_Destroy (entnode);
}

/*
 * Record the removal of a subdirectory.  The arguments are the same
 * as for Subdir_Register.
 */

void Subdir_Deregister (List *entries, const char *parent, const char *dir)
{
    Entnode *entnode;

    entnode = subdir_record ('R', parent, dir);
    Entnode_Destroy (entnode);

    if (entries != NULL && (parent == NULL || strcmp (parent, ".") == 0))
    {
	Node *p;

	p = findnode_fn (entries, dir);
	if (p != NULL)
	    delnode (p);
    }
}
