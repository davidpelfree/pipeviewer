/*
 * Display functions.
 *
 * Copyright 2010 Andrew Wood, distributed under the Artistic License 2.0.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"
#include "pv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>


/*
 * Fill in opts->width and opts->height with the current terminal size,
 * if possible.
 */
void pv_screensize(opts_t opts)
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
 * Calculate the percentage transferred so far and return it.
 */
static long pv__calc_percentage(long long so_far, const long long total)
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
static long pv__calc_eta(const long long so_far, const long long total,
			 const long elapsed)
{
	long long amount_left;

	if (so_far < 1)
		return 0;

	amount_left = total - so_far;
	amount_left *= (long long) elapsed;
	amount_left /= so_far;

	return (long) amount_left;
}

/*
 * Given a long double value, it is divided or multiplied by the ratio until
 * a value in the range 1.0 to 999.999... is found. The character that
 * 'prefix' points to is updated to the corresponding SI prefix.
 *
 * Submitted by Henry Gebhardt <hsggebhardt@googlemail.com> and then
 * modified.
 */
static void pv__si_prefix(long double *value, char *prefix,
			  const long double ratio)
{
	static char *pfx = NULL;
	static char const *pfx_middle = NULL;
	char const *i;

	if (pfx == NULL) {
		pfx = _("yzafpnum kMGTPEZY");
	}

	if (pfx_middle == NULL) {
		/*
		 * We can't assign this in the declaration above because
		 * that wouldn't be constant, so we do it here.
		 */
		pfx_middle = strchr(pfx, ' ');
	}
	i = pfx_middle;

	*prefix = '\0';			    /* Make the prefix start empty. */

	/*
	 * Force an empty prefix if the value is zero to avoid "0yB".
	 */
	if (*value == 0.0)
		return;

	while ((*value >= 1000.0) && (*(i += 1) != '\0')) {
		*value /= ratio;
		*prefix = *i;
	}

	while ((*value < 1.0) && ((i -= 1) != (pfx - 1))) {
		*value *= ratio;
		*prefix = *i;
	}
}


/*
 * Structure to hold the internal data for a single display.
 */
struct pv_display_state {
	long percentage;
	long double prev_elapsed_sec;
	long double prev_rate;
	long double prev_trans;
	char *outbuffer;
	long outbufsize;
	opts_t opts;
};

/*
 * Initialise the given display structure from the given option set. Note
 * that this copies the given "opts" pointer, not the underlying structure,
 * so if "opts" is freed, the state's copy is invalidated and must not be
 * used.
 */
static void pv__state_init(struct pv_display_state *state, opts_t opts)
{
	if (state == NULL)
		return;
	memset(state, 0, sizeof(struct pv_display_state));
	state->opts = opts;
}


/*
 * Return a pointer to a string (which must not be freed), containing status
 * information formatted according to the state held within the given
 * structure, where "elapsed_sec" is the seconds elapsed since the transfer
 * started, "bytes_since_last" is the number of bytes transferred since the
 * last update, and "total_bytes" is the total number of bytes transferred
 * so far.
 *
 * If "bytes_since_last" is negative, this is the final update so the rate
 * is given as an an average over the whole transfer; otherwise the current
 * rate is shown.
 *
 * In line mode, "bytes_since_last" and "total_bytes" are in lines, not bytes.
 *
 * If "total_bytes" is negative, then free all allocated memory and return
 * NULL.
 */
static char *pv__format(struct pv_display_state *state,
			long double elapsed_sec,
			long long bytes_since_last, long long total_bytes)
{
	long double time_since_last, rate, transferred;
	long eta;
	int component_count;
	int static_portion_size;
	char str_transferred[128];	 /* RATS: ignore (big enough) */
	char str_timer[128];		 /* RATS: ignore (big enough) */
	char str_rate[128];		 /* RATS: ignore (big enough) */
	char str_average_rate[128];	 /* RATS: ignore (big enough) */
	char str_eta[128];		 /* RATS: ignore (big enough) */
	char si_prefix[2] = " ";	 /* RATS: ignore (big enough) */
	char *units;
	long double average_rate;

	/* Quick sanity check - state must exist */
	if (state == NULL)
		return NULL;

	/* Negative total transfer - free memory and exit */
	if (total_bytes < 0) {
		if (state->outbuffer)
			free(state->outbuffer);
		state->outbuffer = NULL;
		return NULL;
	}

	/*
	 * In case the time since the last update is very small, we keep
	 * track of amount transferred since the last update, and just keep
	 * adding to that until a reasonable amount of time has passed to
	 * avoid rate spikes or division by zero.
	 */
	time_since_last = elapsed_sec - state->prev_elapsed_sec;
	if (time_since_last <= 0.01) {
		rate = state->prev_rate;
		state->prev_trans += bytes_since_last;
	} else {
		rate =
		    ((long double) bytes_since_last +
		     state->prev_trans) / time_since_last;
		state->prev_elapsed_sec = elapsed_sec;
		state->prev_trans = 0;
	}
	state->prev_rate = rate;

	/*
	 * We only calculate the overall average rate if this is the last
	 * update or if the average rate display is enabled. Otherwise it's
	 * not worth the extra CPU cycles.
	 */
	if ((bytes_since_last < 0) || (state->opts->average_rate)) {
		/* Sanity check to avoid division by zero */
		if (elapsed_sec < 0.000001)
			elapsed_sec = 0.000001;
		average_rate =
		    ((long double) total_bytes) /
		    (long double) elapsed_sec;
		if (bytes_since_last < 0)
			rate = average_rate;
	}

	if (state->opts->size <= 0) {
		/*
		 * If we don't know the total size of the incoming data,
		 * then for a percentage, we gradually increase the
		 * percentage completion as data arrives, to a maximum of
		 * 200, then reset it - we use this if we can't calculate
		 * it, so that the numeric percentage output will go
		 * 0%-100%, 100%-0%, 0%-100%, and so on.
		 */
		if (rate > 0)
			state->percentage += 2;
		if (state->percentage > 199)
			state->percentage = 0;
	} else if (state->opts->numeric || state->opts->progress) {
		/*
		 * If we do know the total size, and we're going to show
		 * the percentage (numeric mode or a progress bar),
		 * calculate the percentage completion.
		 */
		state->percentage =
		    pv__calc_percentage(total_bytes, state->opts->size);
	}

	/*
	 * Reallocate output buffer if width changes.
	 */
	if (state->outbuffer != NULL
	    && state->outbufsize < (state->opts->width * 2)) {
		free(state->outbuffer);
		state->outbuffer = NULL;
		state->outbufsize = 0;
	}

	/*
	 * Allocate output buffer if there isn't one.
	 */
	if (state->outbuffer == NULL) {
		state->outbufsize = (2 * state->opts->width) + 80;
		if (state->opts->name)
			state->outbufsize += strlen(state->opts->name);	/* RATS: ignore */
		state->outbuffer = malloc(state->outbufsize + 16);
		if (state->outbuffer == NULL) {
			fprintf(stderr, "%s: %s: %s\n",
				state->opts->program_name,
				_("buffer allocation failed"),
				strerror(errno));
			state->opts->exit_status |= 64;
			return NULL;
		}
		state->outbuffer[0] = 0;
	}

	/* In numeric output mode, our output is just a number. */
	if (state->opts->numeric) {
		if (state->percentage > 100) {
			/* As mentioned above, we go 0-100, then 100-0. */
			sprintf(state->outbuffer, "%ld\n",
				200 - state->percentage);
		} else {
			sprintf(state->outbuffer, "%ld\n",
				state->percentage);
		}
		return state->outbuffer;
	}

	/*
	 * First, work out what components we will be putting in the output
	 * buffer, and for those that don't depend on the total width
	 * available (i.e. all but the progress bar), prepare their strings
	 * to be placed in the output buffer.
	 */

	/* We start off with no components. */
	component_count = 0;
	static_portion_size = 0;
	str_transferred[0] = 0;
	str_timer[0] = 0;
	str_rate[0] = 0;
	str_average_rate[0] = 0;
	str_eta[0] = 0;

	/* If we're showing a name, add it to the list and the length. */
	if (state->opts->name) {
		int name_length;

		name_length = strlen(state->opts->name);
		if (name_length < 9)
			name_length = 9;
		if (name_length > 500)
			name_length = 500;

		component_count++;
		static_portion_size += name_length + 1;	/* +1 for ":" */
	}

	/* If we're showing bytes transferred, set up the display string. */
	if (state->opts->bytes) {
		transferred = total_bytes;

		if (state->opts->linemode) {
			pv__si_prefix(&transferred, si_prefix, 1000.0);
			units = "";
		} else {
			pv__si_prefix(&transferred, si_prefix, 1024.0);
			units = _("B");
		}

		/* Bounds check, so we don't overrun the prefix buffer. */
		if (transferred > 100000)
			transferred = 100000;

		sprintf(str_transferred, "%4.3Lg%.1s%.16s", transferred,
			si_prefix, units);

		component_count++;
		static_portion_size += strlen(str_transferred);
	}

	/* Timer - set up the display string. */
	if (state->opts->timer) {
		/*
		 * Bounds check, so we don't overrun the prefix buffer. This
		 * does mean that the timer will stop at a 100,000 hours,
		 * but since that's 11 years, it shouldn't be a problem.
		 */
		if (elapsed_sec > (long double) 360000000.0L)
			elapsed_sec = (long double) 360000000.0L;

		sprintf(str_timer, "%ld:%02ld:%02ld",
			((long) elapsed_sec) / 3600,
			(((long) elapsed_sec) / 60) % 60,
			((long) elapsed_sec) % 60);

		component_count++;
		static_portion_size += strlen(str_timer);
	}

	/* Rate - set up the display string. */
	if (state->opts->rate) {
		if (state->opts->linemode) {
			pv__si_prefix(&rate, si_prefix, 1000.0);
			units = _("/s");
		} else {
			pv__si_prefix(&rate, si_prefix, 1024.0);
			units = _("B/s");
		}

		/*
		 * Bounds check, so we don't overrun the prefix buffer.
		 * Hopefully 97TB/sec is fast enough.
		 */
		if (rate > 100000)
			rate = 100000;

		sprintf(str_rate, "[%4.3Lg%.1s%.16s]", rate, si_prefix,
			units);

		component_count++;
		static_portion_size += strlen(str_rate);
	}

	/* Average rate - set up the display string. */
	if (state->opts->average_rate) {
		if (state->opts->linemode) {
			pv__si_prefix(&average_rate, si_prefix, 1000.0);
			units = _("/s");
		} else {
			pv__si_prefix(&average_rate, si_prefix, 1024.0);
			units = _("B/s");
		}

		/* Bounds check, as above. */
		if (average_rate > 100000)
			average_rate = 100000;

		sprintf(str_average_rate, "[%4.3Lg%.1s%.16s]",
			average_rate, si_prefix, units);

		component_count++;
		static_portion_size += strlen(str_average_rate);
	}

	/* ETA (only if size is known) - set up the display string. */
	if (state->opts->eta && state->opts->size > 0) {
		eta =
		    pv__calc_eta(total_bytes, state->opts->size,
				 elapsed_sec);

		if (eta < 0)
			eta = 0;

		/*
		 * Bounds check, so we don't overrun the suffix buffer. This
		 * means the ETA will always be less than 100,000 hours.
		 */
		if (eta > (long) 360000000L)
			eta = (long) 360000000L;

		sprintf(str_eta, "%.16s %ld:%02ld:%02ld", _("ETA"),
			eta / 3600, (eta / 60) % 60, eta % 60);

		/*
		 * If this is the final update, show a blank space where the
		 * ETA used to be.
		 */
		if (bytes_since_last < 0) {
			int i;
			for (i = 0; i < sizeof(str_eta) && str_eta[i] != 0;
			     i++) {
				str_eta[i] = ' ';
			}
		}

		component_count++;
		static_portion_size += strlen(str_eta);
	}

	/*
	 * We now have all the static portions built; all that is left is
	 * the dynamically sized progress bar. So now we assemble the
	 * output buffer, inserting the progress bar at the appropriate
	 * point with the appropriate width.
	 */

	state->outbuffer[0] = 0;

	if (state->opts->name) {
		sprintf(state->outbuffer, "%9s:", state->opts->name);	/* RATS: ignore (OK) */
	}
#define PV_APPEND(x) if (x[0] != 0) { \
	if (state->outbuffer[0] != 0) \
		strcat(state->outbuffer, " "); \
	strcat(state->outbuffer, x); \
	}

	PV_APPEND(str_transferred);
	PV_APPEND(str_timer);
	PV_APPEND(str_rate);
	PV_APPEND(str_average_rate);

	if (state->opts->progress) {
		char pct[16];		 /* RATS: ignore (big enough) */
		int available_width, i;

		if (state->outbuffer[0] != 0)
			strcat(state->outbuffer, " ");
		strcat(state->outbuffer, "[");

		if (state->opts->size > 0) {
			if (state->percentage < 0)
				state->percentage = 0;
			if (state->percentage > 100000)
				state->percentage = 100000;
			sprintf(pct, "%2ld%%", state->percentage);
			available_width =
			    state->opts->width - static_portion_size -
			    component_count - strlen(pct) - 3;

			for (i = 0;
			     i <
			     (available_width * state->percentage) / 100 -
			     1; i++) {
				if (i < available_width)
					strcat(state->outbuffer, "=");
			}
			if (i < available_width) {
				strcat(state->outbuffer, ">");
				i++;
			}
			for (; i < available_width; i++) {
				strcat(state->outbuffer, " ");
			}
			strcat(state->outbuffer, "] ");
			strcat(state->outbuffer, pct);	/* RATS: ignore (OK) */
		} else {
			int p = state->percentage;
			available_width =
			    state->opts->width - static_portion_size -
			    component_count - 5;
			if (p > 100)
				p = 200 - p;
			for (i = 0; i < (available_width * p) / 100; i++) {
				if (i < available_width)
					strcat(state->outbuffer, " ");
			}
			strcat(state->outbuffer, "<=>");
			for (; i < available_width; i++) {
				strcat(state->outbuffer, " ");
			}
			strcat(state->outbuffer, "]");
		}
	}

	PV_APPEND(str_eta);

	return state->outbuffer;
}


/*
 * Output status information on standard error, where "esec" is the seconds
 * elapsed since the transfer started, "sl" is the number of bytes transferred
 * since the last update, and "tot" is the total number of bytes transferred
 * so far.
 *
 * If "sl" is negative, this is the final update so the rate is given as an
 * an average over the whole transfer; otherwise the current rate is shown.
 *
 * In line mode, "sl" and "tot" are in lines, not bytes.
 *
 * If "opts" is NULL, then free all allocated memory and return.
 */
void pv_display(opts_t opts, long double esec, long long sl, long long tot)
{
	static struct pv_display_state state;
	static int initialised = 0;
	char *display;

	if (!initialised) {
		pv__state_init(&state, opts);
		initialised = 1;
	}

	if (opts == NULL) {
		if (initialised)
			(void) pv__format(&state, 0, 0, -1);
		initialised = 0;
		return;
	}

	pv_sig_checkbg();

	display = pv__format(&state, esec, sl, tot);
	if (display == NULL)
		return;

	if (opts->numeric) {
		write(STDERR_FILENO, display, strlen(display));	/* RATS: ignore */
	} else if (opts->cursor) {
		pv_crs_update(opts, display);
	} else {
		write(STDERR_FILENO, display, strlen(display));	/* RATS: ignore */
		write(STDERR_FILENO, "\r", 1);
	}
}

/* EOF */
