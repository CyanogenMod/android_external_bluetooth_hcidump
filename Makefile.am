#
#  $Id$
#

SUBDIRS = parser

sbin_PROGRAMS = hcidump

LDADD = parser/libparser.a

man_MANS = hcidump.8

AM_CFLAGS = @BLUEZ_CFLAGS@

EXTRA_DIST = $(man_MANS) hcidump.spec

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in \
	missing install-sh mkinstalldirs
