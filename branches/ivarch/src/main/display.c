/*
 * Display functions.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


int getnum_i(char *);


/*
 * Calculate the percentage transferred so far and return it.
 */
long calc_percentage(long long so_far, long long total)
{
	if (total < 1)
 		return 0;

	so_far *= 100;
	so_far /= total;

	return (long) so_far;
}


/*
 * Given how many bytes have been transferred, the total byte count to
 * transfer, and how long it's taken so far in seconds, return the estimated
 * number of seconds until completion.
 */
long calc_eta(long long so_far, long long total, long elapsed)
{
	long long bytes_left;

	if (so_far < 1)
		return 0;

	bytes_left = total - so_far;
	bytes_left *= (long long) elapsed;
	bytes_left /= so_far;

	return (long) bytes_left;
}


/*
 * Original Y co-ordinate.
 */
static int cursor__y = 0;


/*
 * Initialise the terminal for cursor positioning.
 */
void cursor_init(opts_t options)
{
	struct termios tty;
	struct termios old_tty;
	struct flock lock;
	char tmp[32];
	int fd;

	if (!options->cursor)
		return;

	fd = open("/dev/tty", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to open terminal"),
			strerror(errno));
		options->cursor = 0;
		return;
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(fd, F_SETLKW, &lock);

	tcgetattr(fd, &tty);
	tcgetattr(fd, &old_tty);
	tty.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fd, TCSANOW | TCSAFLUSH, &tty);
	write(fd, "\n\n\n\n\033M\033M\033M\033[6n", 14);
	memset(tmp, 0, sizeof(tmp));
	read(fd, tmp, 6);
	cursor__y = getnum_i(tmp + 2);
	tcsetattr(fd, TCSANOW | TCSAFLUSH, &old_tty);

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(fd, F_SETLK, &lock);

	close(fd);

	if (cursor__y < 0)
		options->cursor = 0;
}


/*
 * Reposition the cursor to its original position.
 */
void cursor_fini(opts_t options)
{
	char tmp[32];
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLKW, &lock);

	sprintf(tmp, "\033[%dH\n", cursor__y);
	write(STDERR_FILENO, tmp, strlen(tmp));

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLK, &lock);
}


/*
 * Output status information on standard error. If "sl" is negative, this is
 * the final update so the rate is given as an average over the whole
 * transfer; otherwise the rate is "sl"/"esec", i.e. the current rate.
 */
void main_display(opts_t opts, double esec, long long sl, long long tot)
{
	static long percentage = 0;
	static double prev_esec = 0;
	double sincelast;
	long double rate, transferred;
	long eta;
	char display[1024];
	char tmp[1024];
	char tmp2[8];
	char *suffix;
	int avail, i;
	struct flock lock;

	if (sl >= 0) {
		sincelast = esec - prev_esec;
		prev_esec = esec;
		if (sincelast <= 0)
			sincelast = 1;
		rate = sl / sincelast;
	} else {
		if (esec < 0.000001)
			esec = 0.000001;
		rate = tot / esec;
	}

	if (rate > 0) percentage += 2;
	if (percentage > 199) percentage = 0;
	eta = 0;

	if (opts->size > 0)
		percentage = calc_percentage(tot, opts->size);

	if (opts->numeric) {
		if (percentage > 100) {
			sprintf(display, "%ld\n", 200 - percentage);
		} else {
			sprintf(display, "%ld\n", percentage);
		}
		write(STDERR_FILENO, display, strlen(display));
		return;
	}

	if (opts->size > 0)
		eta = calc_eta(tot, opts->size, esec);

	display[0] = 0;

	if (opts->name)
		sprintf(display, "%9s: ", opts->name);

	if (opts->bytes) {
		transferred = tot;
		if (transferred >= 1000*1024*1024) {
			transferred = transferred / (1024*1024*1024);
			suffix = _("GB");
		} else if (transferred >= 1000*1024) {
			transferred = transferred / (1024*1024);
			suffix = _("MB");
		} else if (transferred >= 1000) {
			transferred = transferred / 1024;
			suffix = _("kB");
		} else {
			suffix = _("B");
		}
		sprintf(tmp, "%4.3Lg%s ", transferred, suffix);
		strcat(display, tmp);
	}

	if (opts->timer) {
		sprintf(tmp, "%ld:%02ld:%02ld ", ((long)esec) / 3600,
			(((long)esec) / 60) % 60, ((long)esec) % 60);
		strcat(display, tmp);
	}

	if (opts->rate) {
		if (rate >= 1000*1024*1024) {
			rate = rate / (1024*1024*1024);
			suffix = _("GB/s");
		} else if (rate >= 1000*1024) {
			rate = rate / (1024*1024);
			suffix = _("MB/s");
		} else if (rate >= 1000) {
			rate = rate / 1024;
			suffix = _("kB/s");
		} else {
			suffix = _("B/s ");
		}
		sprintf(tmp, "[%4.3Lg%s] ", rate, suffix);
		strcat(display, tmp);
	}

	tmp[0] = 0;
	if (opts->eta && opts->size > 0) {
		if (eta < 0) {
			eta = 0;
		}
		sprintf(tmp, " %s %ld:%02ld:%02ld", _("ETA"),
			eta / 3600, (eta / 60) % 60, eta % 60);
	}

	if (opts->progress) {
		strcat(display, "[");
		if (opts->size > 0) {
			sprintf(tmp2, "%2ld%%", percentage);
			avail = opts->width - strlen(display)
				- strlen(tmp) - strlen(tmp2) - 3;
			for (i = 0; i < (avail*percentage)/100 - 1;
			     i++) {
				if (i < avail)
					strcat(display, "=");
			}
			if (i < avail) {
				strcat(display, ">");
				i++;
			}
			for (; i < avail; i++) {
				strcat(display, " ");
			}
			strcat(display, "] ");
			strcat(display, tmp2);
		} else {
			int p = percentage;
			avail = opts->width - strlen(display)
				- strlen(tmp) - 5;
			if (p > 100) p = 200 - p;
			for (i = 0; i < (avail*p)/100; i++) {
				if (i < avail)
					strcat(display, " ");
			}
			strcat(display, "<=>");
			for (; i < avail; i++) {
				strcat(display, " ");
			}
			strcat(display, "]");
		}
	}

	strcat(display, tmp);

	if (opts->cursor) {
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 1;
		fcntl(STDERR_FILENO, F_SETLKW, &lock);
		sprintf(tmp, "\033[%dH", cursor__y);
		write(STDERR_FILENO, tmp, strlen(tmp));
		write(STDERR_FILENO, display, strlen(display));
		lock.l_type = F_UNLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 1;
		fcntl(STDERR_FILENO, F_SETLK, &lock);
	} else {
		write(STDERR_FILENO, display, strlen(display));
		write(STDERR_FILENO, "\r", 1);
	}

}

/* EOF $Id$ */
