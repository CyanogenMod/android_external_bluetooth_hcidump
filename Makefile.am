#
#  $Id$
#

SUBDIRS = parser src

sbin_PROGRAMS = hcidump

LDADD = parser/libparser.a

man_MANS = hcidump.8

AM_CFLAGS = @BLUEZ_CFLAGS@

EXTRA_DIST = $(man_MANS)

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in \
	missing install-sh mkinstalldirs
