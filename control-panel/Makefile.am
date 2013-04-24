## Process this file with automake to produce Makefile.in
# Makefile for library files used by GNU CVS.
# Copyright (C) 1986, 1988-1994, 2000 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

AM_CPPFLAGS = -I$(top_srcdir)/lib $(includeopt) @PTHREAD_CPPFLAGS@

cvs_targets = cvscontrold

cvscontrold_SOURCES = \
	cvscontrold.cpp \
	CvsControl.cpp \
	server.cpp 

bin_PROGRAMS=$(cvs_targets)

cvscontrold_LDADD = \
	../lib/libcvs.a @PTHREAD_LDFLAGS@

EXTRA_DIST = \
	.cvsignore 

# for backwards compatibility with the old makefiles
realclean: maintainer-clean
.PHONY: realclean
