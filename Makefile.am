#
#  $Id$
#

SUBDIRS := parser

mandir = $(prefix)/share/man

sbin_PROGRAMS = hcidump

hcidump_SOURCES = hcidump.c hcidump.h
hcidump_LDADD   = parser/libparser.la

man_MANS = hcidump.8

EXTRA_DIST = $(man_MANS) hcidump.spec
