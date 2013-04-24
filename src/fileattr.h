/* Declarations for file attribute munging features.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef FILEATTR_H
#define FILEATTR_H 

/* XML File containing per-file attributes.  
   Builtin attributes:

   <watched />: Present means the file is watched and should be checked out
   read-only.

   <watchers>: Users with watches for this file.  Value is
   <watcher name=WATCHER>
   <TYPE />
   </watcher>
   where WATCHER is a username, and TYPE is edit,unedit or commit (or nothing if none; there is no "none" or "all" keyword).

   <editors>: Users editing this file.  Value is
   <editor name=EDITOR>
   <time>TIME</time>
   <hostname>HOSTNAME</hostname>
   <pathname>PATHNAME</pathname>
   </editor>
   where EDITOR is a username, 
   TIME is when the "cvs edit" command happened,
   and HOSTNAME and PATHNAME are for the working directory.  */

#define CVSREP_OLDFILEATTR "CVS/fileattr"
#define CVSREP_OLDOWNER "/.owner"
#define CVSREP_OLDPERMS "/.perms"
#define CVSREP_FILEATTR "CVS/fileattr.xml"

class CXmlNode;
typedef CXmlNode *XmlHandle_t;

/* Prepare for a new directory with repository REPOS.  If REPOS is NULL,
   then prepare for a "non-directory"; the caller can call fileattr_write
   and fileattr_free, but must not call fileattr_get or fileattr_set.  */
void fileattr_startdir(const char *repos);

/* Search for a given node.  A little like xpath but not nearly as complex */
XmlHandle_t fileattr_find(XmlHandle_t root, const char *exp, ...);

/* Search for a given node.  A little like xpath but not nearly as complex */
/* This version creates any nodes that aren't in the path */
XmlHandle_t fileattr_create(XmlHandle_t root, const char *exp, ...);

/* Return the next node on this level with this name, for walking lists */
XmlHandle_t fileattr_next(XmlHandle_t node);

/* Delete a value under the node. */
void fileattr_delete(XmlHandle_t root, const char *exp, ...);

/* Delete a value under the node. */
void fileattr_delete_child(XmlHandle_t root, XmlHandle_t child);

/* Delete a value under the node at the next prune operation */
void fileattr_batch_delete(XmlHandle_t root);

/* If this node has no children, delete it & recurse upwards.  Rinse. Wash. Repeat. */
void fileattr_prune(XmlHandle_t node);

/* Get a single value from a node.  Pass null to get value of this node. */
const char *fileattr_getvalue(XmlHandle_t root, const char *name);

/* Set a single value for a node.  Pass null to set value of this node. */
void fileattr_setvalue(XmlHandle_t root, const char *name, const char *value);

/* Set the attributes for file FILENAME in whatever manner is appropriate
   for a newly created file.  */
void fileattr_newfile(const char *filename);

/* Write out all modified attributes.  */
void fileattr_write();

/* Free all memory allocated by fileattr_*.  */
void fileattr_free();

/* Create a copy of a subtree */
XmlHandle_t fileattr_copy(XmlHandle_t root);

/* Copy a subtree into an existing tree */
void fileattr_paste(XmlHandle_t root, XmlHandle_t source);

/* Free a copied subtree */
void fileattr_free_subtree(XmlHandle_t *root);

/* Force modified */
void fileattr_modified();

class CXmlNode;
/* This is is a read/convert and is better than just calling CXmlNode */
void _fileattr_read(CXmlNode*& root, const char *repos);
CXmlNode *_fileattr_find(CXmlNode *node, const char *exp, ...);

#endif /* fileattr.h */
