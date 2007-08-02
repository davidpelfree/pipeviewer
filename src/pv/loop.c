/*
 * Main program entry point - read the command line options, then perform
 * the appropriate actions.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"
#include "pv.h"

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

extern struct timeval pv_sig__toffset;
extern sig_atomic_t pv_sig__newsize;


/*
 * Pipe data from a list of files to standard output, giving information
 * about the transfer on standard error according to the given options.
 *
 * Returns nonzero on error.
 */
int pv_main_loop(opts_t opts)
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

	pv_crs_init(opts);

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

	fd = pv_next_file(opts, n, -1);
	if (fd < 0) {
		return 1;
	}

	if (fstat64(fd, &sb) == 0) {
		pv_set_buffer_size(sb.st_blksize * 32, 0);
	}

	if (opts->buffer_size > 0) {
		pv_set_buffer_size(opts->buffer_size, 1);
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
		    pv_transfer(opts, fd, &eof_in, &eof_out, cansend);
		if (written < 0)
			return 1;

		since_last += written;
		total_written += written;
		if (opts->rate_limit > 0)
			donealready += written;

		if (eof_in && eof_out && n < (opts->argc - 1)) {
			n++;
			fd = pv_next_file(opts, n, fd);
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
			pv_sig_nopause();
			gettimeofday(&start_time, NULL);
			pv_sig__toffset.tv_sec = 0;
			pv_sig__toffset.tv_usec = 0;
			pv_sig_allowpause();

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

		init_time.tv_sec =
		    start_time.tv_sec + pv_sig__toffset.tv_sec;
		init_time.tv_usec =
		    start_time.tv_usec + pv_sig__toffset.tv_usec;
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

		if (pv_sig__newsize) {
			pv_sig__newsize = 0;
			pv_screensize(opts);
		}

		pv_display(opts, elapsed, since_last, total_written);

		since_last = 0;
	}

	if (opts->cursor) {
		pv_crs_fini(opts);
	} else {
		if ((!opts->numeric) && (!opts->no_op))
			write(STDERR_FILENO, "\n", 1);
	}

	/*
	 * Free up the buffers used by the display and data transfer
	 * routines.
	 */
	pv_display(0, 0, 0, 0);
	pv_transfer(0, -1, 0, 0, 0);

	return 0;
}

/* EOF */
