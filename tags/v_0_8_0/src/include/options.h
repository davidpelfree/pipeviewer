/*
 * Global program option structure and the parsing function prototype.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#ifndef _OPTIONS_H
#define _OPTIONS_H 1

#ifdef __cplusplus
extern "C" {
#endif

struct opts_s;
typedef struct opts_s *opts_t;

struct opts_s {           /* structure describing run-time options */
	char *program_name;            /* name the program is running as */
	unsigned char do_nothing;      /* exit-without-doing-anything flag */
	unsigned char progress;        /* progress bar flag */
	unsigned char timer;           /* timer flag */
	unsigned char eta;             /* ETA flag */
	unsigned char rate;            /* rate counter flag */
	unsigned char bytes;           /* bytes transferred flag */
	unsigned char force;           /* force-if-not-terminal flag */
	unsigned char cursor;          /* whether to use cursor positioning */
	unsigned char numeric;         /* numeric output only */
	unsigned char wait;            /* wait for transfer before display */
	unsigned char no_op;           /* do nothing other than pipe data */
	unsigned long long rate_limit; /* rate limit, in bytes per second */
	unsigned long long size;       /* total size of data */
	double interval;               /* interval between updates */
	unsigned int width;            /* screen width */
	char *name;                    /* process name, if any */
	int argc;                      /* number of non-option arguments */
	char **argv;                   /* array of non-option arguments */
	char *current_file;            /* current file being read */
	void (*destructor)(opts_t);    /* call as "opts->destructor(opts)" */
};

extern opts_t parse_options(int, char **);


#ifdef __cplusplus
}
#endif

#endif /* _OPTIONS_H */

/* EOF */
