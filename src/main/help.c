/*
 * Output command-line help to stdout.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>


/*
 * Display command-line help.
 */
void display_help(void)
{
	printf(_("Usage: %s [OPTION] [FILE]..."), PROGRAM_NAME);
	printf("\n%s\n\n",
_("Concatenate FILE(s), or standard input, to standard output,\n"
  "with monitoring."));
	printf("  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n"
	       "  %s\n  %s\n  %s\n  %s\n  %s\n  %s\n\n",
_("-p, --progress        show progress bar"),
_("-t, --timer           show elapsed time"),
_("-e, --eta             show estimated time of arrival (completion)"),
_("-r, --rate            show data transfer rate counter"),
_("-b, --bytes           show number of bytes transferred"),
_("-f, --force           output even if standard error is not a terminal"),
_("-n, --numeric         output percentages, not visual information"),
_("-q, --quiet           do not output any transfer information at all"),
_("-c, --cursor          use cursor positioning escape sequences"),
_("-L, --rate-limit RATE limit transfer to RATE bytes per second"),
_("-W, --wait            display nothing until first byte transferred"),
_("-s, --size SIZE       set estimated data size to SIZE bytes"),
_("-i, --interval SEC    update every SEC seconds"),
_("-w, --width WIDTH     assume terminal is WIDTH characters wide"),
_("-N, --name NAME       prefix visual information with NAME"));
	printf("  %s\n  %s\n  %s\n\n",
_("-h, --help            show this help and exit"),
_("-l, --license         show the license this program is distributed under"),
_("-V, --version         show version information and exit"));
	printf(_("Please report any bugs to %s."), BUG_REPORTS_TO);
	printf("\n");
}

/* EOF */
