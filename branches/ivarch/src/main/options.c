/*
 * Parse command-line options.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"
#include "getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


void display_help(void);
void display_license(void);
void display_version(void);
long long getnum_ll(char *);
double getnum_d(char *);
int getnum_i(char *);


/*
 * Free an opts_t object.
 */
void parse_options__destroy(opts_t object)
{
	if (!object) return;
	if (object->argv) free(object->argv);
	free(object);
}


/*
 * Parse the given command-line arguments into an opts_t object, handling
 * "help", "license" and "version" options internally.
 *
 * Returns an opts_t, or 0 on error.
 *
 * Note that the contents of *argv[] (i.e. the command line parameters)
 * aren't copied anywhere, just the pointers are copied, so make sure the
 * command line data isn't overwritten or argv[1] free()d or whatever.
 */
opts_t parse_options(int argc, char ** argv)
{
	struct option long_options[] = {
		{"help",	0, 0, 'h'},
		{"license",	0, 0, 'l'},
		{"version",	0, 0, 'V'},
		{"progress",	0, 0, 'p'},
		{"timer",	0, 0, 't'},
		{"eta",		0, 0, 'e'},
		{"rate",	0, 0, 'r'},
		{"bytes",	0, 0, 'b'},
		{"force",	0, 0, 'f'},
		{"numeric",	0, 0, 'n'},
		{"quiet",	0, 0, 'q'},
		{"cursor",	0, 0, 'c'},
		{"rate-limit",	1, 0, 'L'},
		{"wait",	0, 0, 'W'},
		{"size",	1, 0, 's'},
		{"interval",	1, 0, 'i'},
		{"width",	1, 0, 'w'},
		{"name",	1, 0, 'N'},
		{0, 0, 0, 0}
	};
	char * short_options = "hlVpterbfnqcL:Ws:i:w:N:";
	int option_index = 0;
	int c, n, numopts;
	opts_t options;

	n = 0;
	long_options[n++].name = _("help");
	long_options[n++].name = _("license");
	long_options[n++].name = _("version");
	long_options[n++].name = _("progress");
	long_options[n++].name = _("timer");
	long_options[n++].name = _("eta");
	long_options[n++].name = _("rate");
	long_options[n++].name = _("bytes");
	long_options[n++].name = _("force");
	long_options[n++].name = _("numeric");
	long_options[n++].name = _("quiet");
	long_options[n++].name = _("cursor");
	long_options[n++].name = _("rate-limit");
	long_options[n++].name = _("wait");
	long_options[n++].name = _("size");
	long_options[n++].name = _("interval");
	long_options[n++].name = _("width");
	long_options[n++].name = _("name");

	options = calloc(1, sizeof(*options));
	if (!options) {
		fprintf(stderr,
		  _("%s: option structure allocation failed (%s)"),
		  argv[0],
		  strerror(errno));
		fprintf(stderr, "\n");
		return 0;
	}

	options->program_name = argv[0];

	options->destructor = parse_options__destroy;

	options->argc = 0;
	options->argv = calloc(argc + 1, sizeof(char *));
	if (!options->argv) {
		fprintf(stderr,
		  _("%s: option structure argv allocation failed (%s)"),
		  argv[0],
		  strerror(errno));
		fprintf(stderr, "\n");
		options->destructor(options);
		return 0;
	}

	numopts = 0;

	options->interval = 1;

	do {
		c = getopt_long(argc, argv,
		  short_options, long_options, &option_index);
		if (c < 0) continue;

		switch (c) {
			case 'h':
				display_help();
				options->do_nothing = 1;
				return options;
				break;
			case 'l':
				display_license();
				options->do_nothing = 1;
				return options;
				break;
			case 'V':
				display_version();
				options->do_nothing = 1;
				return options;
				break;
			case 'p':
				options->progress = 1;
				numopts++;
				break;
			case 't':
				options->timer = 1;
				numopts++;
				break;
			case 'e':
				options->eta = 1;
				numopts++;
				break;
			case 'r':
				options->rate = 1;
				numopts++;
				break;
			case 'b':
				options->bytes = 1;
				numopts++;
				break;
			case 'f':
				options->force = 1;
				break;
			case 'n':
				options->numeric = 1;
				numopts++;
				break;
			case 'q':
				options->no_op = 1;
				numopts++;
				break;
			case 'c':
				options->cursor = 1;
				break;
			case 'L':
				options->rate_limit = getnum_ll(optarg);
				break;
			case 'W':
				options->wait = 1;
				break;
			case 's':
				options->size = getnum_ll(optarg);
				break;
			case 'i':
				options->interval = getnum_d(optarg);
				break;
			case 'w':
				options->width = getnum_i(optarg);
				break;
			case 'N':
				options->name = optarg;
				break;
			default :
				fprintf(stderr,
				  _("Try `%s --help' for more information."),
				  argv[0]);
				fprintf(stderr, "\n");
				options->destructor(options);
				return 0;
				break;
		}

	} while (c != -1);

	if (numopts == 0) {
		options->progress = 1;
		options->timer = 1;
		options->eta = 1;
		options->rate = 1;
		options->bytes = 1;
	}

	/* Interval must be at least 0.1 second at at most 10 minutes */
	if (options->interval < 0.1) options->interval = 0.1;
	if (options->interval > 600) options->interval = 600;

	while (optind < argc) {
		options->argv[options->argc++] = argv[optind++];
	}

	return options;
}

/* EOF $Id$ */
