## Process this file with automake to produce Makefile.in
# Makefile for GNU CVS program.
# Copyright (C) 1986, 1988-1990, 2000 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Publ.cpp License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Publ.cpp License for more details.

if WITH_ZLIB
  zlib_inc = 
  zlib_lib = @ZLIB_LIBS@
else
  zlib_inc = -I$(top_srcdir)/zlib
  zlib_lib = ../zlib/libz.a 
endif

if WITH_PCRE
  pcre_lib = @PCRE_LIBS@
else
  pcre_lib = ../pcre/.libs/libpcreposix.a ../pcre/.libs/libpcre.a
endif

if WITH_PAM
  pam_lib = @PAM_LIBS@
else
  pam_lib =
endif

if WITH_MAC_HFS_SUPPORT
  mac_hfs_inc = -I$(top_srcdir)/osx
  mac_hfs_lib = ../osx/libmachfs.a
else
  mac_hfs_inc =
  mac_hfs_lib =
endif

if GSERVER_PROTOCOL_SUPPORT
  gssapi_lib = @GSSAPI_LIBS@
else
  gssapi_lib =
endif

if SSERVER_PROTOCOL_SUPPORT
  ssl_lib = @SSL_LIBS@
else
  ssl_lib =
endif

libdl_lib = $(LIBLTDL)

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/cvsapi -I$(top_srcdir)/cvstools -I$(top_srcdir)/diff -I$(top_srcdir)/cvsapi $(zlib_inc) $(includeopt) $(INCLTDL) $(mac_hfs_inc)

bin_PROGRAMS = cvsnt

# The cvs executable
cvsnt_SOURCES = \
	add.cpp \
	admin.cpp \
	annotate.cpp \
	buffer.cpp \
	chacl.cpp \
	chown.cpp \
	checkin.cpp \
	checkout.cpp \
	classify.cpp \
	client.cpp \
	commit.cpp \
	create_adm.cpp \
	cvsrc.cpp \
	cvsrcs.cpp \
	diff.cpp \
	edit.cpp \
	entries.cpp \
	error.cpp \
	expand_path.cpp \
	fileattr.cpp \
	filesubr.cpp \
	find_names.cpp \
	hash.cpp \
	history.cpp \
	ignore.cpp \
	info.cpp \
	import.cpp \
	lock.cpp \
	log.cpp \
	login.cpp \
	logmsg.cpp \
	ls.cpp \
	lsacl.cpp \
	main.cpp \
	mapping.cpp \
	mkmodules.cpp \
	modules.cpp \
	myndbm.cpp \
	no_diff.cpp \
	parseinfo.cpp \
	passwd.cpp \
	patch.cpp \
	perms.cpp \
	rcs.cpp \
	rcscmds.cpp \
	recurse.cpp \
	release.cpp \
	remove.cpp \
	rename.cpp \
	repos.cpp \
	root.cpp \
	run.cpp \
	savecwd.cpp \
	scramble.cpp \
	server.cpp \
	status.cpp \
	stripslash.cpp \
	subr.cpp \
	tag.cpp \
	update.cpp \
	version.cpp \
	vers_ts.cpp \
	watch.cpp \
	wrapper.cpp \
	xgetwd.cpp \
	xdiff.cpp \
	zlib.cpp \
	buffer.h \
	client.h \
	cvs.h \
	edit.h \
	error.h \
	fileattr.h \
	hardlink.h \
	hash.h \
	mapping.h \
	myndbm.h \
	rcs.h \
	savecwd.h \
	server.h \
	update.h \
	watch.h

cvsnt_LDADD = \
	-L$(DESTDIR)/$(libdir) \
	../diff/libdiff.a \
	../lib/libcvs.a \
	../cvsdelta/libcvsdelta.a \
	../cvsapi/libcvsapi.la \
	../cvstools/libcvstools.la \
	$(zlib_lib) \
	$(pcre_lib) \
	$(libdl_lib) \
	$(pam_lib) \
	$(mac_hfs_lib)

# extra clean targets
# wish this could be distclean-hdr-local but it's not part of automake
DISTCLEANFILES = options.h-SAVED check.log check.plog

# General
EXTRA_DIST = \
	.cvsignore \
	ChangeLog-9194 \
	ChangeLog-9395 \
	ChangeLog-96 \
	ChangeLog-97 

## MAINTAINER Targets

# for backwards compatibility with the old makefiles
.PHONY: realclean
realclean: maintainer-clean

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/cvs

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/cvs
	$(LN_S) cvsnt $(DESTDIR)$(bindir)/cvs

