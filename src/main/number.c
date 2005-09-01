/*
 * Functions for converting strings to numbers.
 *
 * Copyright 2005 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/*
 * This function is used instead of the macro from <ctype.h> because
 * including <ctype.h> causes weird versioned glibc dependencies on certain
 * Red Hat systems, complicating package management.
 */
static int my_isdigit(char c)
{
	return ((c >= '0') && (c <= '9'));
}


/*
 * Return the numeric value of "str", as a long long.
 */
long long getnum_ll(char *str)
{
	long long n = 0;
	long long decimal = 0;
	int decdivisor = 1;
	int shift = 0;

	while (str[0] != 0 && (!my_isdigit(str[0])))
		str++;

	for (; my_isdigit(str[0]); str++) {
		n = n * 10;
		n += (str[0] - '0');
	}

	/*
	 * If a decimal value was given, skip the decimal part.
	 */
	if ((str[0] == '.') || (str[0] == ',')) {
		str++;
		for (; my_isdigit(str[0]); str++) {
			if (decdivisor < 10000) {
				decimal = decimal * 10;
				decimal += (str[0] - '0');
				decdivisor = decdivisor * 10;
			}
		}
	}

	/*
	 * Parse any units given (K=KiB=*1024, M=MiB=1024KiB, G=GiB=1024MiB,
	 * T=TiB=1024GiB).
	 */
	if (str[0]) {
		while ((str[0] == ' ') || (str[0] == '\t'))
			str++;
		switch (str[0]) {
		case 'k':
		case 'K':
			shift = 10;
			break;
		case 'm':
		case 'M':
			shift = 20;
			break;
		case 'g':
		case 'G':
			shift = 30;
			break;
		case 't':
		case 'T':
			shift = 40;
			break;
		default:
			break;
		}
	}

	/*
	 * Binary left-shift the supplied number by "shift" times, i.e.
	 * apply the given units (KiB, MiB, etc) to it, but never shift left
	 * more than 30 at a time to avoid overflows.
	 */
	while (shift > 0) {
		int shiftby;

		shiftby = shift;
		if (shiftby > 30)
			shiftby = 30;

		n = n << shiftby;
		decimal = decimal << shiftby;
		shift -= shiftby;
	}

	/*
	 * Add any decimal component.
	 */
	decimal = decimal / decdivisor;
	n += decimal;

	return n;
}


/*
 * Return the numeric value of "str", as a double.
 */
double getnum_d(char *str)
{
	double n = 0.0;
	double step = 1;

	while (str[0] != 0 && (!my_isdigit(str[0])))
		str++;

	for (; my_isdigit(str[0]); str++) {
		n = n * 10;
		n += (str[0] - '0');
	}

	if ((str[0] != '.') && (str[0] != ','))
		return n;

	for (; my_isdigit(str[0]) && step < 1000000; str++) {
		step = step * 10;
		n += (str[0] - '0') / step;
	}

	return n;
}


/*
 * Return the numeric value of "str", as an int.
 */
int getnum_i(char *str)
{
	return (int) getnum_ll(str);
}

/* EOF */
