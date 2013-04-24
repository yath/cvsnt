## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib $(includeopt) 

libmdnsclient_la_SOURCES = \
	dns.c \
	dns.h \
	mdnsclient.c \
	mdnsclient.h \
	util.c \
	util.h

libmdnsclient_la_LDFLAGS = -release $(VERSION) 

lib_LTLIBRARIES = libmdnsclient.la
