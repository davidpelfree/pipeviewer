/*
 * Display functions.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
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
#include <unistd.h>


int getnum_i(char *);
void sig_checkbg(void);
void cursor_init(opts_t);
void cursor_update(opts_t, char *);
void cursor_fini(opts_t);


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
 * Output status information on standard error. If "sl" is negative, this is
 * the final update so the rate is given as an average over the whole
 * transfer; otherwise the rate is "sl"/"esec", i.e. the current rate.
 */
void main_display(opts_t opts, long double esec, long long sl,
		  long long tot)
{
	static long percentage = 0;
	static long double prev_esec = 0;
	static long double prev_rate = 0;
	static long double prev_trans = 0;
	static char *display = NULL;
	static long dispbufsz = 0;
	long double sincelast, rate, transferred;
	long eta;
	char tmp[1024];			 /* RATS: ignore (checked OK) */
	char tmp2[64];			 /* RATS: ignore (checked OK) */
	char *suffix;
	int avail, i;

	sig_checkbg();

	if (sl >= 0) {
		sincelast = esec - prev_esec;
		if (sincelast <= 0.01) {
			rate = prev_rate;
			prev_trans += sl;
		} else {
			rate = ((long double) sl + prev_trans) / sincelast;
			prev_esec = esec;
			prev_trans = 0;
		}
	} else {
		if (esec < 0.000001)
			esec = 0.000001;
		rate = ((long double) tot) / (long double) esec;
	}
	prev_rate = rate;

	if (rate > 0)
		percentage += 2;
	if (percentage > 199)
		percentage = 0;
	eta = 0;

	/*
	 * Reallocate display buffer if width changes.
	 */
	if (display != NULL && dispbufsz < (opts->width * 2)) {
		free(display);
		display = NULL;
		dispbufsz = 0;
	}

	/*
	 * Allocate display buffer if there isn't one.
	 */
	if (display == NULL) {
		dispbufsz = opts->width + 80;
		if (opts->name)
			dispbufsz += strlen(opts->name);	/* RATS: ignore */
		display = malloc(dispbufsz + 16);
		if (display == NULL)
			return;
		display[0] = 0;
	}

	if (opts->size > 0)
		percentage = calc_percentage(tot, opts->size);

	if (opts->numeric) {
		if (percentage > 100) {
			sprintf(display, "%ld\n", 200 - percentage);
		} else {
			sprintf(display, "%ld\n", percentage);
		}
		write(STDERR_FILENO, display, strlen(display));	/* RATS: ignore */
		return;
	}

	if (opts->size > 0)
		eta = calc_eta(tot, opts->size, esec);

	display[0] = 0;

	if (opts->name)
		sprintf(display, "%9s: ", opts->name);	/* RATS: ignore (OK) */

	if (opts->bytes) {
		transferred = tot;
		if (transferred >= 1000 * 1024 * 1024) {
			transferred = transferred / (1024 * 1024 * 1024);
			suffix = _("GB");
		} else if (transferred >= 1000 * 1024) {
			transferred = transferred / (1024 * 1024);
			suffix = _("MB");
		} else if (transferred >= 1000) {
			transferred = transferred / 1024;
			suffix = _("kB");
		} else {
			suffix = _("B");
		}
		sprintf(tmp, "%4.3Lg%.16s ", transferred, suffix);
		strcat(display, tmp);	    /* RATS: ignore (OK) */
	}

	if (opts->timer) {
		sprintf(tmp, "%ld:%02ld:%02ld ", ((long) esec) / 3600,
			(((long) esec) / 60) % 60, ((long) esec) % 60);
		strcat(display, tmp);	    /* RATS: ignore (OK) */
	}

	if (opts->rate) {
		if (rate >= 1000 * 1024 * 1024) {
			rate = rate / (1024 * 1024 * 1024);
			suffix = _("GB/s");
		} else if (rate >= 1000 * 1024) {
			rate = rate / (1024 * 1024);
			suffix = _("MB/s");
		} else if (rate >= 1000) {
			rate = rate / 1024;
			suffix = _("kB/s");
		} else {
			suffix = _("B/s ");
		}
		sprintf(tmp, "[%4.3Lg%.16s] ", rate, suffix);
		strcat(display, tmp);	    /* RATS: ignore (OK) */
	}

	tmp[0] = 0;
	if (opts->eta && opts->size > 0) {
		if (eta < 0) {
			eta = 0;
		}
		sprintf(tmp, " %.16s %ld:%02ld:%02ld", _("ETA"),
			eta / 3600, (eta / 60) % 60, eta % 60);
	}

	if (opts->progress) {
		strcat(display, "[");
		if (opts->size > 0) {
			if (percentage < 0)
				percentage = 0;
			if (percentage > 100000)
				percentage = 100000;
			sprintf(tmp2, "%2ld%%", percentage);
			avail = opts->width - strlen(display)	/* RATS: ignore */
			    -strlen(tmp)    /* RATS: ignore */
			    -strlen(tmp2) - 3;	/* RATS: ignore */
			for (i = 0; i < (avail * percentage) / 100 - 1;
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
			strcat(display, tmp2);	/* RATS: ignore (OK) */
		} else {
			int p = percentage;
			avail = opts->width - strlen(display)	/* RATS: ignore */
			    -strlen(tmp) - 5;	/* RATS: ignore */
			if (p > 100)
				p = 200 - p;
			for (i = 0; i < (avail * p) / 100; i++) {
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

	strcat(display, tmp);		    /* RATS: ignore (OK) */

	if (opts->cursor) {
		cursor_update(opts, display);
	} else {
		write(STDERR_FILENO, display, strlen(display));	/* RATS: ignore */
		write(STDERR_FILENO, "\r", 1);
	}
}

/* EOF */