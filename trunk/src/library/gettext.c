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


char *minisetlocale(char *a, char *b)
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
	static struct msgtable_s *table = NULL;
	static int tried_lang = 0;
	char *lang;
	int i;

	if (msgid == NULL)
		return msgid;

	if (tried_lang == 0) {
		lang = getenv("LANGUAGE");  /* RATS: ignore */
		if (lang)
			table = minigettext__gettable(lang);

		if (table == NULL) {
			lang = getenv("LANG");	/* RATS: ignore */
			if (lang)
				table = minigettext__gettable(lang);
		}

		if (table == NULL) {
			lang = getenv("LC_ALL");	/* RATS: ignore */
			if (lang)
				table = minigettext__gettable(lang);
		}

		if (table == NULL) {
			lang = getenv("LC_MESSAGES");	/* RATS: ignore */
			if (lang)
				table = minigettext__gettable(lang);
		}

		tried_lang = 1;
	}

	if (table == NULL)
		return msgid;

	for (i = 0; table[i].msgid; i++) {
		if (strcmp(table[i].msgid, msgid) == 0)
			return table[i].msgstr;
	}

	return msgid;
}

#endif				/* HAVE_GETTEXT */

/* EOF */
