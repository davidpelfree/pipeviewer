AC_PREREQ(2.5)

dnl Check NLS options

AC_DEFUN(md_PATH_PROG,
  [AC_PATH_PROG($1,$2,$3)dnl
   if echo $$1 | grep openwin > /dev/null; then
     echo "WARNING: Do not use OpenWin's $2.  (Better remove it.) >&AC_FD_MSG"
     ac_cv_path_$1=$2
     $1=$2
   fi
])

AC_DEFUN(ud_LC_MESSAGES,
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], ud_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       ud_cv_val_LC_MESSAGES=yes, ud_cv_val_LC_MESSAGES=no)])
    if test $ud_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES)
    fi
  fi])

AC_DEFUN(ud_WITH_NLS,
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      nls_cv_use_nls=$enableval, nls_cv_use_nls=yes)
    AC_MSG_RESULT($nls_cv_use_nls)

    dnl If we use NLS figure out what method
    if test "$nls_cv_use_nls" = "yes"; then
      AC_DEFINE(ENABLE_NLS)
      AC_MSG_CHECKING([for explicitly using GNU gettext])
      AC_ARG_WITH(gnu-gettext,
        [  --with-gnu-gettext      use the GNU gettext library],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      if test "$nls_cv_force_use_gnu_gettext" = "yes"; then
        nls_cv_use_gnu_gettext=yes
      else
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If gettext or catgets are available (in this order) we
        dnl use this.  Else we have to fall back to GNU NLS library.
        AC_CHECK_LIB(intl, main)
        AC_CHECK_LIB(i, main)
        CATOBJEXT=NONE
        AC_CHECK_FUNC(gettext,
          [AC_DEFINE(HAVE_GETTEXT)
           md_PATH_PROG(MSGFMT, msgfmt, no)dnl
	   if test "$MSGFMT" != "no"; then
	     AC_CHECK_FUNCS(dcgettext)
	     md_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
	     md_PATH_PROG(XGETTEXT, xgettext, xgettext)
             CATOBJEXT=.mo
	     INSTOBJEXT=.mo
	     DATADIRNAME=lib
	   fi])

        if test "$CATOBJEXT" = "NONE"; then
          dnl No gettext in C library.  Try catgets next.
          AC_CHECK_FUNC(catgets,
            [AC_DEFINE(HAVE_CATGETS)
             INTLOBJS="src/nls/intl-cat.o src/nls/cat-id-tbl.o"
             AC_PATH_PROG(GENCAT, gencat, no)dnl
	     if test "$GENCAT" != "no"; then
	       AC_PATH_PROGS(GMSGFMT, [gmsgfmt msgfmt], msgfmt)
	       md_PATH_PROG(XGETTEXT, xgettext, xgettext)
               CATOBJEXT=.cat
	       INSTOBJEXT=.cat
	       DATADIRNAME=lib
               INTLDEPS="src/nls/intl.o"
	       INTLLIBS=$INTLDEPS
	       LIBS=`echo $LIBS | sed -e 's/-lintl//'`
               CPPFLAGS="$CPPFLAGS -I$srcdir/src/nls"
	       nls_cv_header_intl=intl/libintl.h
	       nls_cv_header_libgt=intl/libgettext.h
	     fi])
        fi

        if test "$CATOBJEXT" = "NONE"; then
	  dnl Neither gettext nor catgets in included in the C library.
	  dnl Fall back on GNU gettext library.
	  nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="src/nls/intl-gett.o"
        md_PATH_PROG(MSGFMT, msgfmt, msgfmt)
        md_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
        md_PATH_PROG(XGETTEXT, xgettext, xgettext)
        AC_SUBST(MSGFMT)
        CATOBJEXT=.gmo
        INSTOBJEXT=.mo
        DATADIRNAME=share
        INTLDEPS="src/nls/intl.o"
        INTLLIBS=$INTLDEPS
	LIBS=`echo $LIBS | sed -e 's/-lintl//'`
        CPPFLAGS="$CPPFLAGS -I$srcdir/src/nls"
        nls_cv_header_intl=intl/libintl.h
        nls_cv_header_libgt=intl/libgettext.h
      fi

      # We need to process the intl/ and po/ directory.
      INTLSUB=intl
      POSUB=src/nls/pofiles.made
      POCATOBJ=src/nls/cat-id-tbl.o
    else
      DATADIRNAME=share
      nls_cv_header_intl=intl/libintl.h
      nls_cv_header_libgt=intl/libgettext.h
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(DATADIRNAME)
    AC_SUBST(GMOFILES)
    AC_SUBST(INSTOBJEXT)
    AC_SUBST(INTLDEPS)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(INTLSUB)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)
    AC_SUBST(POCATOBJ)
  ])

AC_DEFUN(ud_GNU_GETTEXT,
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl

   AC_CHECK_HEADERS([limits.h locale.h nl_types.h malloc.h string.h unistd.h values.h])
   AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp])

   if test "${ac_cv_func_stpcpy+set}" != "set"; then
     AC_CHECK_FUNCS(stpcpy)
   fi
   if test "${ac_cv_func_stpcpy}" = "yes"; then
     AC_DEFINE(HAVE_STPCPY)
   fi

   ud_LC_MESSAGES
   ud_WITH_NLS

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for lang in ${LINGUAS=$ALL_LINGUAS}; do
         case "$ALL_LINGUAS" in
          *$lang*) NEW_LINGUAS="$NEW_LINGUAS $lang" ;;
         esac
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do
         CATALOGS="$CATALOGS \$(srcdir)/src/nls/po/$lang$CATOBJEXT";
       done
     fi
   fi

   dnl Determine which catalog format we have (if any is needed)
   dnl For now we know about two different formats:
   dnl   Linux and the normal X/Open format
   if test "$CATOBJEXT" = ".cat"; then
     AC_CHECK_HEADER(linux/version.h, msgformat=linux, msgformat=xopen)
     AC_SUBST(msgformat)
   fi
  ])

dnl EOF
