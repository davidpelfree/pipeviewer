/*
 * Functions for transferring between file descriptors.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#define BUFFER_SIZE	409600
#define BUFFER_SIZE_MAX	524288

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


static unsigned long pvmtbufsize = BUFFER_SIZE;


/*
 * Set the buffer size for transfers.
 */
void main_transfer_bufsize(unsigned long sz)
{
	if (sz > BUFFER_SIZE_MAX)
		sz = BUFFER_SIZE_MAX;
	pvmtbufsize = sz;
}


/*
 * Transfer some data from "fd" to standard output, timing out after 9/100
 * of a second. If options->rate_limit is >0, only up to "allowed" bytes can
 * be written.
 *
 * Returns the number of bytes written, or negative on error.
 */
long main_transfer(opts_t options, int fd, int *eof_in, int *eof_out, unsigned long long allowed)
{
	static unsigned char * buf = NULL;
	static unsigned long in_buffer = 0;
	static unsigned long bytes_written = 0;
	struct timeval tv;
	fd_set readfds;
	fd_set writefds;
	long to_write, written;
	ssize_t r, w;
	int n;

	if (buf == NULL) {
		buf = (unsigned char *) malloc(pvmtbufsize + 32);
		if (buf == NULL) {
			fprintf(stderr, "%s: %s: %s\n", options->program_name,
				_("buffer allocation failed"),
				strerror(errno));
			return -1;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 90000;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	if ((!(*eof_in)) && (in_buffer < pvmtbufsize)) {
		FD_SET(fd, &readfds);
	}

    	to_write = in_buffer - bytes_written;
    	if (options->rate_limit > 0) {
    		if (to_write > allowed) {
    			to_write = allowed;
    		}
    	}

	if ((!(*eof_out)) && (to_write > 0)) {
		FD_SET(STDOUT_FILENO, &writefds);
	}

	if ((*eof_in) && (*eof_out))
		return 0;

	n = select(FD_SETSIZE, &readfds, &writefds, NULL, &tv);

	if (n < 0) {
		if (errno == EINTR)
			return 0;
		fprintf(stderr, "%s: %s: %s: %d: %s\n", options->program_name,
			options->current_file, _("select call failed"), n,
			strerror(errno));
		return -1;
	}

	written = 0;

	if (FD_ISSET(fd, &readfds)) {
		r = read(fd, buf + in_buffer, pvmtbufsize - in_buffer);
		if (r < 0) {
			fprintf(stderr, "%s: %s: %s: %s\n",
				options->program_name,
				options->current_file,
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
	    && (in_buffer > bytes_written)) {
		w = write(STDOUT_FILENO, buf + bytes_written, to_write);
		if (w < 0) {
			fprintf(stderr, "%s: %s: %s\n",
				options->program_name,
				_("write failed"),
				strerror(errno));
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

	return written;
}

/* EOF */
