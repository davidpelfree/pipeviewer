#
# Variables for Make.
#

srcdir = @srcdir@

prefix = @prefix@
exec_prefix = @exec_prefix@
bindir = @bindir@
infodir = @infodir@
mandir = @mandir@
etcdir = @prefix@/etc
datadir = @datadir@
sbindir = @sbindir@

VPATH = $(srcdir)

localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale
gettextsrcdir = $(prefix)/share/gettext
aliaspath = $(localedir):.

msgformat = @msgformat@
GMOFILES = @GMOFILES@
POFILES = @POFILES@
CATALOGS = @CATALOGS@
GENCAT = @GENCAT@
GMSGFMT = @GMSGFMT@
MSGFMT = @MSGFMT@
XGETTEXT = @XGETTEXT@
MSGMERGE = msgmerge
POSUB = @POSUB@
CATOBJEXT = @CATOBJEXT@
INSTOBJEXT = @INSTOBJEXT@

@SET_MAKE@
SHELL = /bin/sh
CC = @CC@
LD = ld
DO_GZIP = @DO_GZIP@
INSTALL = @INSTALL@
INSTALL_DATA = @INSTALL_DATA@
UNINSTALL = rm -f

LDFLAGS = -r
LINKFLAGS = @LDFLAGS@
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\" -DGNULOCALEDIR=\"$(gnulocaledir)\" -DLOCALE_ALIAS_PATH=\"$(aliaspath)\" 
CFLAGS = @CFLAGS@
CPPFLAGS = @CPPFLAGS@ -I$(srcdir)/src/include -Isrc/include $(DEFS)
LIBS = @LIBS@ @INTLDEPS@ @INTLOBJS@

alltarg = @PACKAGE@

# EOF
