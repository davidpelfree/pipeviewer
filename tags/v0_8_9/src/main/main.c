/*
 * Main program entry point - read the command line options, then perform
 * the appropriate actions.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#include <termios.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

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
static void get_screensize(opts_t opts)
{
#ifdef TIOCGWINSZ
	struct winsize wsz;

	if (isatty(STDERR_FILENO)) {
		if (ioctl(STDERR_FILENO, TIOCGWINSZ, &wsz) == 0) {
			opts->width = wsz.ws_col;
			opts->height = wsz.ws_row;
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
static int main_loop(opts_t opts)
{
	long written;
	long long total_written, since_last, cansend, donealready, target;
	int eof_in, eof_out, final_update;
	struct timeval start_time, next_update, next_reset, cur_time;
	struct timeval init_time;
	long double elapsed, tilreset;
	struct stat64 sb;
	int fd, n;

	fd = -1;

	cursor_init(opts);

	eof_in = 0;
	eof_out = 0;
	total_written = 0;
	since_last = 0;

	gettimeofday(&start_time, NULL);
	gettimeofday(&cur_time, NULL);

	next_update.tv_sec = start_time.tv_sec + (long) opts->interval;
	next_update.tv_usec = start_time.tv_usec
	    + ((opts->interval - ((long) opts->interval)) * 1000000);
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
	if (n >= opts->argc) {
		fd = STDIN_FILENO;
	} else {
		fd = main_nextfd(opts, n, -1);
		if (fd < 0)
			return 1;
		if (fstat64(fd, &sb) == 0) {
			main_transfer_bufsize(sb.st_blksize * 32);
		}
	}

	while ((!(eof_in && eof_out)) || (!final_update)) {

		tilreset = next_reset.tv_sec - cur_time.tv_sec;
		tilreset +=
		    (next_reset.tv_usec - cur_time.tv_usec) / 1000000.0;
		if (tilreset < 0)
			tilreset = 0;

		if (opts->rate_limit > 0) {
			target = (1.03 - tilreset) * opts->rate_limit;
			cansend = target - donealready;
			if (target < donealready)
				cansend = 0;
		}

		written =
		    main_transfer(opts, fd, &eof_in, &eof_out, cansend);
		if (written < 0)
			return 1;

		since_last += written;
		total_written += written;
		if (opts->rate_limit > 0)
			donealready += written;

		if (eof_in && eof_out && n < (opts->argc - 1)) {
			n++;
			fd = main_nextfd(opts, n, fd);
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
			&& cur_time.tv_usec >= next_reset.tv_usec)) {
			next_reset.tv_sec++;
			if (next_reset.tv_sec < cur_time.tv_sec)
				next_reset.tv_sec = cur_time.tv_sec;
			donealready = 0;
		}

		if (opts->no_op)
			continue;

		/*
		 * If -W was given, we don't output anything until we have
		 * read a byte, at which point we then count time as if we
		 * started when the first byte was received
		 */
		if (opts->wait) {
			if (written < 1)
				continue;   /* nothing written yet */

			opts->wait = 0;

			/*
			 * Reset the timer offset counter now that data
			 * transfer has begun, otherwise if we had been
			 * stopped and started (with ^Z / SIGTSTOP)
			 * previously (while waiting for data), the timers
			 * will be wrongly offset.
			 *
			 * While we reset the offset counter we must disable
			 * SIGTSTOP so things don't mess up.
			 */
			sig_nopause();
			gettimeofday(&start_time, NULL);
			sig__toffset.tv_sec = 0;
			sig__toffset.tv_usec = 0;
			sig_allowpause();

			next_update.tv_sec = start_time.tv_sec
			    + (long) opts->interval;
			next_update.tv_usec = start_time.tv_usec
			    + ((opts->interval - ((long) opts->interval))
			       * 1000000);
			if (next_update.tv_usec >= 1000000) {
				next_update.tv_sec++;
				next_update.tv_usec -= 1000000;
			}
		}

		if ((cur_time.tv_sec < next_update.tv_sec)
		    || (cur_time.tv_sec == next_update.tv_sec
			&& cur_time.tv_usec < next_update.tv_usec)) {
			continue;
		}

		next_update.tv_sec = next_update.tv_sec
		    + (long) opts->interval;
		next_update.tv_usec = next_update.tv_usec
		    + ((opts->interval - ((long) opts->interval))
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
		init_time.tv_usec =
		    start_time.tv_usec + sig__toffset.tv_usec;
		if (init_time.tv_usec >= 1000000) {
			init_time.tv_sec++;
			init_time.tv_usec -= 1000000;
		}
		if (init_time.tv_usec < 0) {
			init_time.tv_sec--;
			init_time.tv_usec += 1000000;
		}

		elapsed = cur_time.tv_sec - init_time.tv_sec;
		elapsed +=
		    (cur_time.tv_usec - init_time.tv_usec) / 1000000.0;

		if (final_update)
			since_last = -1;

		if (sig__newsize) {
			sig__newsize = 0;
			get_screensize(opts);
		}

		main_display(opts, elapsed, since_last, total_written);

		since_last = 0;
	}

	if (opts->cursor) {
		cursor_fini(opts);
	} else {
		if ((!opts->numeric) && (!opts->no_op))
			write(STDERR_FILENO, "\n", 1);
	}

	/*
	 * Free up the buffers used by the display and data transfer
	 * routines.
	 */
	main_display(0, 0, 0, 0);
	main_transfer(0, -1, 0, 0, 0);

	return 0;
}


/*
 * Process command-line arguments and set option flags, then call functions
 * to initialise, and finally enter the main loop.
 */
int main(int argc, char **argv)
{
	struct termios t;
	opts_t opts;
	int retcode = 0;

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	opts = opts_parse(argc, argv);
	if (!opts)
		return 1;
	if (opts->do_nothing) {
		opts_free(opts);
		return 0;
	}

	if (opts->size == 0) {
		main_getsize(opts);
	}

	if (opts->size < 1)
		opts->eta = 0;

	if ((isatty(STDERR_FILENO) == 0)
	    && (opts->force == 0)
	    && (opts->numeric == 0)) {
		opts->no_op = 1;
	}

	if (opts->width == 0) {
		int tmpheight;
		tmpheight = opts->height;
		get_screensize(opts);
		if (tmpheight > 0)
			opts->height = tmpheight;
	}

	if (opts->height == 0) {
		int tmpwidth;
		tmpwidth = opts->width;
		get_screensize(opts);
		if (tmpwidth > 0)
			opts->width = tmpwidth;
	}

	/*
	 * Width and height bounds checking (and defaults).
	 */
	if (opts->width < 1)
		opts->width = 80;

	if (opts->height < 1)
		opts->height = 25;

	if (opts->width > 999999)
		opts->width = 999999;

	if (opts->height > 999999)
		opts->height = 999999;

	/*
	 * Try and make standard output use non-blocking I/O.
	 */
	fcntl(STDOUT_FILENO, F_SETFL,
	      O_NONBLOCK | fcntl(STDOUT_FILENO, F_GETFL));

	/*
	 * Set terminal option TOSTOP so we get signal SIGTTOU if we try to
	 * write to the terminal while backgrounded.
	 */
	tcgetattr(STDERR_FILENO, &t);
	t.c_lflag |= TOSTOP;
	tcsetattr(STDERR_FILENO, TCSANOW, &t);

	opts->current_file = "(stdin)";

	sig_init();

	retcode = main_loop(opts);

	opts_free(opts);
	return retcode;
}

/* EOF */
