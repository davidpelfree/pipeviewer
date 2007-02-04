/*
 * Functions for transferring between file descriptors.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#define BUFFER_SIZE	409600
#define BUFFER_SIZE_MAX	524288

#define MAXIMISE_BUFFER_FILL	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


static unsigned long long pvmtbufsize = BUFFER_SIZE;


/*
 * Set the buffer size for transfers.
 */
void main_transfer_bufsize(unsigned long long sz, int force)
{
	if ((sz > BUFFER_SIZE_MAX) && (!force))
		sz = BUFFER_SIZE_MAX;
	pvmtbufsize = sz;
}


/*
 * Transfer some data from "fd" to standard output, timing out after 9/100
 * of a second. If opts->rate_limit is >0, only up to "allowed" bytes can
 * be written.
 *
 * Returns the number of bytes written, or negative on error.
 *
 * If "opts" is NULL, then the transfer buffer is freed, and zero is
 * returned.
 */
long main_transfer(opts_t opts, int fd, int *eof_in, int *eof_out,
		   unsigned long long allowed)
{
	static unsigned char *buf = NULL;
	static unsigned long in_buffer = 0;
	static unsigned long bytes_written = 0;
	struct timeval tv;
	fd_set readfds;
	fd_set writefds;
	int max_fd;
	long to_write, written;
	ssize_t r, w;
	int n;

	if (opts == NULL) {
		if (buf)
			free(buf);
		buf = NULL;
		in_buffer = 0;
		bytes_written = 0;
		return 0;
	}

	if (buf == NULL) {
		buf = (unsigned char *) malloc(pvmtbufsize + 32);
		if (buf == NULL) {
			fprintf(stderr, "%s: %s: %s\n",
				opts->program_name,
				_("buffer allocation failed"),
				strerror(errno));
			return -1;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 90000;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	max_fd = 0;

	if ((!(*eof_in)) && (in_buffer < pvmtbufsize)) {
		FD_SET(fd, &readfds);
		if (fd > max_fd)
		        max_fd = fd;
	}

	to_write = in_buffer - bytes_written;
	if (opts->rate_limit > 0) {
		if (to_write > allowed) {
			to_write = allowed;
		}
	}

	if ((!(*eof_out)) && (to_write > 0)) {
		FD_SET(STDOUT_FILENO, &writefds);
		if (STDOUT_FILENO > max_fd)
		        max_fd = STDOUT_FILENO;
	}

	if ((*eof_in) && (*eof_out))
		return 0;

	n = select(max_fd + 1, &readfds, &writefds, NULL, &tv);

	if (n < 0) {
		if (errno == EINTR)
			return 0;
		fprintf(stderr, "%s: %s: %s: %d: %s\n",
			opts->program_name, opts->current_file,
			_("select call failed"), n, strerror(errno));
		return -1;
	}

	written = 0;

	if (FD_ISSET(fd, &readfds)) {
		r = read( /* RATS: ignore (checked OK) */ fd,
			 buf + in_buffer, pvmtbufsize - in_buffer);
		if (r < 0) {
			/*
			 * If a read error occurred but it was EINTR or
			 * EAGAIN, just wait a bit and then return zero,
			 * since this was a transient error.
			 */
			if ((errno == EINTR) || (errno == EAGAIN)) {
				tv.tv_sec = 0;
				tv.tv_usec = 10000;
				select(0, NULL, NULL, NULL, &tv);
				return 0;
			}
			fprintf(stderr, "%s: %s: %s: %s\n",
				opts->program_name,
				opts->current_file,
				_("read failed"), strerror(errno));
			*eof_in = 1;
			if (bytes_written >= in_buffer)
				*eof_out = 1;
		} else if (r == 0) {
			*eof_in = 1;
			if (bytes_written >= in_buffer)
				*eof_out = 1;
		} else {
			in_buffer += r;
		}
	}

	if (FD_ISSET(STDOUT_FILENO, &writefds)
	    && (in_buffer > bytes_written)
	    && (to_write > 0)) {

		signal(SIGALRM, SIG_IGN);	/* RATS: ignore */
	    	alarm(1);

		w = write(STDOUT_FILENO, buf + bytes_written, to_write);

		alarm(0);

		if (w < 0) {
			/*
			 * If a write error occurred but it was EINTR or
			 * EAGAIN, just wait a bit and then return zero,
			 * since this was a transient error.
			 */
			if ((errno == EINTR) || (errno == EAGAIN)) {
				tv.tv_sec = 0;
				tv.tv_usec = 10000;
				select(0, NULL, NULL, NULL, &tv);
				return 0;
			}
			/*
			 * SIGPIPE means we've finished. Don't output an
			 * error because it's not really our error to report. 
			 */
			if (errno == EPIPE) {
			        *eof_in = 1;
                                *eof_out = 1;
                                return 0;
			}
			fprintf(stderr, "%s: %s: %s\n",
				opts->program_name,
				_("write failed"), strerror(errno));
			*eof_out = 1;
			written = -1;
		} else if (w == 0) {
			*eof_out = 1;
		} else {
			bytes_written += w;
			written += w;
			if (bytes_written >= in_buffer) {
				bytes_written = 0;
				in_buffer = 0;
				if (*eof_in)
					*eof_out = 1;
			}
		}
	}

#ifdef MAXIMISE_BUFFER_FILL
        /*
         * Rotate the written bytes out of the buffer so that it can be
         * filled up completely by the next read.
         */
	if (bytes_written > 0) {
	        if (bytes_written < in_buffer) {
	                memmove(buf, buf + bytes_written, in_buffer - bytes_written);
	                in_buffer -= bytes_written;
	                bytes_written = 0;
	        } else {
	                bytes_written = 0;
	                in_buffer = 0;
	        }
	}
#endif	/* MAXIMISE_BUFFER_FILL */

	return written;
}

/* EOF */
