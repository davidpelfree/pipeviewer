/*
 * Very minimal (and stupid) implementation of gettext, with a fixed lookup
 * table.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETTEXT

struct msgtable_s {
	char *msgid;
	char *msgstr;
};

struct msgtable_s *minigettext__gettable(char *);

struct msgtable_s *minigettext__table = NULL;


char *minisetlocale(char *a, char * b)
{
	return NULL;
}


char *minibindtextdomain(char *a, char *b)
{
	return NULL;
}


char *minitextdomain(char *a)
{
	return NULL;
}


char *minigettext_noop(char *msgid)
{
	return msgid;
}


char *minigettext(char *msgid)
{
	char *lang;
	int i;

	if (msgid == NULL)
		return msgid;

	if (minigettext__table == NULL) {
		lang = getenv("LANGUAGE");
		if (lang)
			minigettext__table = minigettext__gettable(lang);
	}

	if (minigettext__table == NULL) {
		lang = getenv("LANG");
		if (lang)
			minigettext__table = minigettext__gettable(lang);
	}

	if (minigettext__table == NULL) {
		lang = getenv("LC_ALL");
		if (lang)
			minigettext__table = minigettext__gettable(lang);
	}

	if (minigettext__table == NULL) {
		lang = getenv("LC_MESSAGES");
		if (lang)
			minigettext__table = minigettext__gettable(lang);
	}

	if (minigettext__table == NULL)
		return msgid;

	for (i = 0; minigettext__table[i].msgid; i++) {
		if (strcmp(minigettext__table[i].msgid, msgid) == 0)
			return minigettext__table[i].msgstr;
	}

	return msgid;
}

#endif				/* HAVE_GETTEXT */

/* EOF */
