#
# Compilation rules.
#

.SUFFIXES: .c .d .o

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c.d:
	sh $(srcdir)/autoconf/scripts/depend.sh \
	   $(CC) $< $(<:%.c=%) $(srcdir) $(CFLAGS) $(CPPFLAGS) > $@

doc/quickref.txt: doc/quickref.1
	man $+ | sed 's/.//g' | cat -s > $@ || :
	chmod 644 $@ || :

doc/$(package).info: doc/manual.texi
	-makeinfo --no-split doc/manual.texi -o doc/$(package).info
	-chmod 644 doc/$(package).info

doc/manual.html: doc/manual.texi
	-texi2html -monolithic doc/manual.texi
	-test -e manual.html || mv $(package).html manual.html
	-perl $(srcdir)/autoconf/scripts/htmlmunge.pl < manual.html > $@
	rm -f manual.html
	-chmod 644 doc/manual.html

#
# NLS stuff
#

%.mo: %.po
	$(MSGFMT) -o $@ $<
	@touch $@
	@chmod 644 $@

%.gmo: %.po
	rm -f $@
	$(GMSGFMT) -o $@ $<
	@touch $@
	@chmod 644 $@

$(srcdir)/src/nls/$(PACKAGE).pot: $(allsrc)
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

src/nls/table.c: $(POFILES)
	sh $(srcdir)/autoconf/scripts/po2table.sh $(POFILES) > src/nls/table.c

