#
# Compilation rules.
#
# $Id$

.SUFFIXES: .c .d .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.c.d:
	sh $(srcdir)/autoconf/scripts/depend.sh \
	   $(CC) $< $(<:%.c=%) $(srcdir) $(CFLAGS) > $@

doc/quickref.txt: doc/quickref.1
	man doc/quickref.1 | col -b | cat -s > doc/quickref.txt         || :
	chmod 644 doc/quickref.txt                                      || :

doc/$(package).info: doc/manual.texi
	makeinfo --no-split doc/manual.texi -o doc/$(package).info;        :
	chmod 644 doc/$(package).info                                   || :

doc/manual.html: doc/manual.texi
	texi2html -monolithic doc/manual.texi                           || :
	test -e manual.html || mv $(package).html manual.html           || :
	perl $(srcdir)/autoconf/scripts/htmlmunge.pl < manual.html > $@ || :
	rm -f manual.html
	chmod 644 doc/manual.html                                       || :

#
# NLS stuff
#

%.pox: $(srcdir)/src/nls/po/$(PACKAGE).pot %.po
	$(TUPDATE) $(srcdir)/nls/po/$(PACKAGE).pot $< > $@
	@chmod 644 $@

%.mo: %.po
	$(MSGFMT) -o $@ $<
	@touch $@
	@chmod 644 $@

%.gmo: %.po
	rm -f $@
	$(GMSGFMT) -o $@ $<
	@touch $@
	@chmod 644 $@

%.cat: %.po
	sed -f $(srcdir)/autoconf/scripts/$(msgformat)-msg.sed \
	     < $< > $(<:%.cat:%.msg) \
	&& rm -f $@ \
	&& $(GENCAT) $@ $(<:%.cat:%.msg)
	@touch $@
	@chmod 644 $@

$(srcdir)/src/nls/po/$(PACKAGE).pot: $(allsrc)
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(srcdir) \
	            --add-comments --keyword=_ --keyword=N_ \
	            $(allsrc)
	if cmp -s $(PACKAGE).po $@; then \
	  rm -f $(PACKAGE).po; \
	else \
	  rm -f $@; \
	  mv $(PACKAGE).po $@; \
	  chmod 644 $@; \
	fi

src/nls/cat-id-tbl.c: $(srcdir)/src/nls/po/$(PACKAGE).pot
	sed -f $(srcdir)/autoconf/scripts/po2tbl.sed $< \
	| sed -e "s/@PACKAGE NAME@/$(PACKAGE)/" > $@
	chmod 644 $@

src/nls/pofiles.made: src/nls/cat-id-tbl.c $(CATALOGS)
	@touch $@
