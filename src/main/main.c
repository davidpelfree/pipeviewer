/*
 * Main program entry point - read the command line options, then perform
 * the appropriate actions.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


void main_transfer_bufsize(unsigned long);
long main_transfer(opts_t, int, int *, int *, unsigned long long);
void cursor_init(opts_t);
void cursor_fini(opts_t);
void main_display(opts_t, long double, long long, long long);
void main_getsize(opts_t);
int main_nextfd(opts_t, int, int);
void sig_init(void);
void sig_nopause(void);
void sig_allowpause(void);
extern struct timeval sig__toffset;
extern sig_atomic_t sig__newsize;


/*
 * Fill in options->width with the current terminal size, if possible.
 */
void get_width(opts_t options)
{
#ifdef TIOCGWINSZ
	struct winsize wsz;

	if (isatty(STDERR_FILENO)) {
		if (ioctl(STDERR_FILENO, TIOCGWINSZ, &wsz) == 0) {
			options->width = wsz.ws_col;
		}
	}
#endif
}


/*
 * Pipe data from standard input to standard output, giving information
 * about the transfer on standard error according to the given options.
 *
 * Returns nonzero on error.
 */
int main_loop(opts_t options)
{
	long written;
	long long total_written, since_last, cansend, donealready, target;
	int eof_in, eof_out, final_update;
	struct timeval start_time, next_update, next_reset, cur_time;
	struct timeval init_time;
	long double elapsed, tilreset;
	struct stat sb;
	int fd, n;

	fd = -1;

	cursor_init(options);

	eof_in = 0;
	eof_out = 0;
	total_written = 0;
	since_last = 0;

	gettimeofday(&start_time, NULL);
	gettimeofday(&cur_time, NULL);

	next_update.tv_sec = start_time.tv_sec + (long) options->interval;
	next_update.tv_usec = start_time.tv_usec
	  + ((options->interval - ((long)options->interval)) * 1000000);
	if (next_update.tv_usec >= 1000000) {
		next_update.tv_sec++;
		next_update.tv_usec -= 1000000;
	}

	next_reset.tv_sec = start_time.tv_sec + 1;
	next_reset.tv_usec = start_time.tv_usec;

	cansend = 0;
	donealready = 0;
	final_update = 0;

	n = 0;
	if (n >= options->argc) {
		fd = STDIN_FILENO;
	} else {
		fd = main_nextfd(options, n, -1);
		if (fd < 0)
			return 1;
		if (fstat(fd, &sb) == 0) {
			main_transfer_bufsize (sb.st_blksize * 32);
		}
	}

	while ((!(eof_in && eof_out)) || (!final_update)) {

		tilreset = next_reset.tv_sec - cur_time.tv_sec;
		tilreset += (next_reset.tv_usec - cur_time.tv_usec)/1000000.0;
		if (tilreset < 0)
			tilreset = 0;

		if (options->rate_limit > 0) {
			target = (1.03 - tilreset) * options->rate_limit;
			cansend = target - donealready;
			if (target < donealready)
				cansend = 0;
		}

		written = main_transfer(options, fd, &eof_in, &eof_out,
					cansend);
		if (written < 0)
			return 1;

		since_last += written;
		total_written += written;
		if (options->rate_limit > 0)
			donealready += written;

		if (eof_in && eof_out && n < (options->argc - 1)) {
			n++;
			fd = main_nextfd(options, n, fd);
			if (fd < 0)
				return 1;
			eof_in = 0;
			eof_out = 0;
		}

		gettimeofday(&cur_time, NULL);

		if (eof_in && eof_out) {
			final_update = 1;
			next_update.tv_sec = cur_time.tv_sec - 1;
		}

		if ((cur_time.tv_sec > next_reset.tv_sec)
		    || (cur_time.tv_sec == next_reset.tv_sec
		        && cur_time.tv_usec >= next_reset.tv_usec))
		{
			next_reset.tv_sec++;
			if (next_reset.tv_sec < cur_time.tv_sec)
				next_reset.tv_sec = cur_time.tv_sec;
			donealready = 0;
		}

		if (options->no_op) continue;

		/*
		 * If -W given, we don't output anything until we have read
		 * a byte, at which point we then count time as if we
		 * started when the first byte was received
		 */
		if (options->wait) {
			if (written < 1) continue;  /* nothing written yet */

			options->wait = 0;

			sig_nopause();
			gettimeofday(&start_time, NULL);
			sig__toffset.tv_sec = 0;
			sig__toffset.tv_usec = 0;
			sig_allowpause();

			next_update.tv_sec = start_time.tv_sec
			  + (long) options->interval;
			next_update.tv_usec = start_time.tv_usec
			  + ((options->interval - ((long)options->interval))
			     * 1000000);
			if (next_update.tv_usec >= 1000000) {
				next_update.tv_sec++;
				next_update.tv_usec -= 1000000;
			}
		}

		if ((cur_time.tv_sec < next_update.tv_sec)
		    || (cur_time.tv_sec == next_update.tv_sec
		        && cur_time.tv_usec < next_update.tv_usec))
		{
			continue;
		}

		next_update.tv_sec = next_update.tv_sec
		  + (long) options->interval;
		next_update.tv_usec = next_update.tv_usec
		  + ((options->interval - ((long)options->interval))
		     * 1000000);
		if (next_update.tv_usec >= 1000000) {
			next_update.tv_sec++;
			next_update.tv_usec -= 1000000;
		}
		if (next_update.tv_sec < cur_time.tv_sec) {
			next_update.tv_sec = cur_time.tv_sec;
			next_update.tv_usec = cur_time.tv_usec;
		} else if (next_update.tv_sec == cur_time.tv_sec
		           && next_update.tv_usec < cur_time.tv_usec) {
			next_update.tv_usec = cur_time.tv_usec;
		}

		init_time.tv_sec = start_time.tv_sec + sig__toffset.tv_sec;
		init_time.tv_usec = start_time.tv_usec + sig__toffset.tv_usec;
		if (init_time.tv_usec >= 1000000) {
			init_time.tv_sec ++;
			init_time.tv_usec -= 1000000;
		}
		if (init_time.tv_usec < 0) {
			init_time.tv_sec --;
			init_time.tv_usec += 1000000;
		}

		elapsed = cur_time.tv_sec - init_time.tv_sec;
		elapsed += (cur_time.tv_usec - init_time.tv_usec) / 1000000.0;

		if (final_update)
			since_last = -1;

		if (sig__newsize) {
			sig__newsize = 0;
			get_width(options);
		}

		main_display(options, elapsed, since_last, total_written);

		since_last = 0;
	}

	if (options->cursor) {
		cursor_fini(options);
	} else {
		if (!options->numeric)
			if (!options->no_op)
				write(STDERR_FILENO, "\n", 1);
	}

	return 0;
}


/*
 * Process command-line arguments and set option flags, then call functions
 * to initialise, and finally enter the main loop.
 */
int main(int argc, char ** argv)
{
	struct termios t;
	opts_t options;
	int retcode = 0;

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	options = parse_options(argc, argv);
	if (!options) return 1;
	if (options->do_nothing) {
		options->destructor(options);
		return 0;
	}

	if (options->size == 0) {
		main_getsize(options);
	}

	if (options->size < 1) options->eta = 0;

	if ((isatty(STDERR_FILENO) == 0)
	    && (options->force == 0)
	    && (options->numeric == 0)) {
			options->no_op = 1;
	}

	if (options->width == 0)
		get_width(options);


	if (options->width < 1) options->width = 80;

	/* Try and make standard output use non-blocking I/O */
	fcntl(STDOUT_FILENO, F_SETFL,
	      O_NONBLOCK | fcntl(STDOUT_FILENO, F_GETFL));

	/* Set TOSTOP so we get SIGTTOU when writing in background */
	tcgetattr(STDERR_FILENO, &t);
	t.c_lflag |= TOSTOP;
	tcsetattr(STDERR_FILENO, TCSANOW, &t);

	options->current_file = "(stdin)";

	sig_init();

	retcode = main_loop(options);

	options->destructor(options);
	return retcode;
}

/* EOF $Id$ */
