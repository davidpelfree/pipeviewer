#
# Rules for all phony targets.
#

.PHONY: all dep depend depclean make check test \
  clean distclean cvsclean \
  index manhtml update-po \
  doc dist \
  install uninstall \
  rpm deb

all: $(alltarg) $(CATALOGS)

make:
	echo > $(srcdir)/autoconf/make/filelist.mk~
	echo > $(srcdir)/autoconf/make/modules.mk~
	cd $(srcdir); \
	bash autoconf/scripts/makemake.sh \
	     autoconf/make/filelist.mk~ \
	     autoconf/make/modules.mk~
	sh ./config.status

dep depend: $(alldep)
	echo '#' > $(srcdir)/autoconf/make/depend.mk~
	echo '# Dependencies.' >> $(srcdir)/autoconf/make/depend.mk~
	echo '#' >> $(srcdir)/autoconf/make/depend.mk~
	echo >> $(srcdir)/autoconf/make/depend.mk~
	cat $(alldep) >> $(srcdir)/autoconf/make/depend.mk~
	sh ./config.status

clean:
	rm -f $(allobj)

depclean:
	rm -f $(alldep)

update-po: $(srcdir)/src/nls/$(PACKAGE).pot
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  lang=$(srcdir)/`echo $$cat | sed 's/$(CATOBJEXT)$$//'`; \
	  mv $$lang.po $$lang.old.po; \
	  if $(MSGMERGE) $$lang.old.po $(srcdir)/src/nls/$(PACKAGE).pot > $$lang.po; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	    chmod 644 $$lang.po; \
	  fi; \
	done

distclean: clean depclean
	rm -f $(alltarg) src/include/config.h
	rm -rf $(package)-$(version).tar* $(package)-$(version)
	rm -f *.html config.*
	rm Makefile

cvsclean: distclean
	rm -f doc/$(package).info
	rm -f doc/lsm
	rm -f doc/manual.html
	rm -f doc/manual.texi
	rm -f doc/$(package).spec
	rm -f doc/quickref.1
	rm -f doc/quickref.txt
	rm -f configure
	rm -f src/nls/*.gmo src/nls/*.mo
	echo > $(srcdir)/autoconf/make/depend.mk~
	echo > $(srcdir)/autoconf/make/filelist.mk~
	echo > $(srcdir)/autoconf/make/modules.mk~

doc: doc/$(package).info doc/manual.html doc/quickref.txt

index:
	(cd $(srcdir); sh autoconf/scripts/index.sh $(srcdir)) > index.html

manhtml:
	@man2html ./doc/quickref.1 \
	| sed -e '1,/<BODY/d' -e '/<\/BODY/,$$d' \
	      -e 's|<A [^>]*>&nbsp;</A>||ig' \
	      -e 's|<A [^>]*>\([^<]*\)</A>|\1|ig' \
	      -e '/<H1/d' -e 's|\(</H[0-9]>\)|\1<P>|ig' \
	| sed -e '1,/<HR/d' -e '/<H2>Index/,/<HR/d'

dist: doc update-po
	rm -rf $(package)-$(version)
	mkdir $(package)-$(version)
	cp -dprf Makefile $(distfiles) $(package)-$(version)
	cd $(package)-$(version); $(MAKE) distclean
	cp -dpf doc/lsm             $(package)-$(version)/doc/
	cp -dpf doc/$(package).spec $(package)-$(version)/doc/
	cp -dpf doc/$(package).info $(package)-$(version)/doc/
	cp -dpf doc/manual.html     $(package)-$(version)/doc/
	cp -dpf doc/quickref.txt    $(package)-$(version)/doc/
	chmod 644 `find $(package)-$(version) -type f -print`
	chmod 755 `find $(package)-$(version) -type d -print`
	chmod 755 `find $(package)-$(version)/autoconf/scripts`
	chmod 755 $(package)-$(version)/configure
	chmod 755 $(package)-$(version)/debian/rules
	rm -rf DUMMY `find $(package)-$(version) -type d -name CVS`
	tar cf $(package)-$(version).tar $(package)-$(version)
	rm -rf $(package)-$(version)
	$(DO_GZIP) $(package)-$(version).tar

check test: $(alltarg)
	@FAIL=0; PROG=./$(package); TMP1=.tmp1; TMP2=.tmp2; \
	export PROG TMP1 TMP2; \
	for SCRIPT in $(srcdir)/tests/*; do \
	  test -f $$SCRIPT || continue; \
	  echo `basename $$SCRIPT`: | tr "\n" ' '; \
	  STATUS=0; \
	  sh -e $$SCRIPT || STATUS=1; \
	  test $$STATUS -eq 1 && FAIL=1; \
	  test $$STATUS -eq 1 && echo FAILED || echo OK; \
	done; rm -f $$TMP1 $$TMP2; exit $$FAIL

install: all doc
	$(srcdir)/autoconf/scripts/mkinstalldirs \
	  "$(DESTDIR)/$(bindir)"
	$(srcdir)/autoconf/scripts/mkinstalldirs \
	  "$(DESTDIR)/$(mandir)/man1"
	$(srcdir)/autoconf/scripts/mkinstalldirs \
	  "$(DESTDIR)/$(infodir)"
	$(INSTALL) -m 755 $(package) \
	                  "$(DESTDIR)/$(bindir)/$(package)"
	$(INSTALL) -m 644 doc/quickref.1 \
	                  "$(DESTDIR)/$(mandir)/man1/$(package).1"
	$(INSTALL) -m 644 doc/$(package).info \
	                  "$(DESTDIR)/$(infodir)/$(package).info"
	$(DO_GZIP) "$(DESTDIR)/$(mandir)/man1/$(package).1"      || :
	$(DO_GZIP) "$(DESTDIR)/$(infodir)/$(package).info"       || :
	if test -n "$(CATALOGS)"; then \
	  catalogs='$(CATALOGS)'; \
	  for cat in $$catalogs; do \
	    name=`echo $$cat | sed 's,^.*/,,g'`; \
	    if test "`echo $$name | sed 's/.*\(\..*\)/\1/'`" = ".gmo"; then \
	      destdir=$(gnulocaledir); \
	    else \
	      destdir=$(localedir); \
	    fi; \
	    lang=`echo $$name | sed 's/$(CATOBJEXT)$$//'`; \
	    dir=$(DESTDIR)/$$destdir/$$lang/LC_MESSAGES; \
	    $(srcdir)/autoconf/scripts/mkinstalldirs $$dir; \
	    $(INSTALL_DATA) $$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	  done; \
	fi

uninstall:
	$(UNINSTALL) "$(DESTDIR)/$(mandir)/man1/$(package).1"
	$(UNINSTALL) "$(DESTDIR)/$(infodir)/$(package).info"
	$(UNINSTALL) "$(DESTDIR)/$(mandir)/man1/$(package).1.gz"
	$(UNINSTALL) "$(DESTDIR)/$(infodir)/$(package).info.gz"
	if test -n "$(CATALOGS)"; then \
	  catalogs='$(CATALOGS)'; \
	  for cat in $$catalogs; do \
	    name=`echo $$cat | sed 's,^.*/,,g'`; \
	    if test "`echo $$name | sed 's/.*\(\..*\)/\1/'`" = ".gmo"; then \
	      destdir=$(gnulocaledir); \
	    else \
	      destdir=$(localedir); \
	    fi; \
	    lang=`echo $$name | sed 's/$(CATOBJEXT)$$//'`; \
	    dir=$(DESTDIR)/$$destdir/$$lang/LC_MESSAGES; \
	    $(UNINSTALL) $$dir/$(PACKAGE)$(INSTOBJEXT); \
	  done; \
	fi

rpm: dist
	echo macrofiles: `rpm --showrc \
	  | grep ^macrofiles \
	  | cut -d : -f 2- \
	  | sed 's,^[^/]*/,/,'`:`pwd`/rpmmacros > rpmrc
	echo %_topdir `pwd`/rpm > rpmmacros
	rm -rf rpm
	mkdir rpm
	mkdir rpm/SPECS rpm/BUILD rpm/SOURCES rpm/RPMS
	grep -hsv ^macrofiles /usr/lib/rpm/rpmrc /etc/rpmrc $$HOME/.rpmrc \
	  >> rpmrc
	rpmbuild $(RPMFLAGS) --rcfile=rpmrc -tb $(package)-$(version).tar.gz
	mv rpm/RPMS/*/$(package)-*.rpm .
	rm -rf rpm rpmmacros rpmrc

deb: dist
	rm -rf BUILD-DEB
	mkdir BUILD-DEB
	cd BUILD-DEB && tar xzf ../$(package)-$(version).tar.gz
	cd BUILD-DEB && cd $(package)-$(version) && ./debian/rules binary
	mv BUILD-DEB/*.deb .
	rm -rf BUILD-DEB
