/*
 * Functions for converting strings to numbers.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


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

	while (str[0] != 0 && (!my_isdigit(str[0]))) str++;

	for (; my_isdigit(str[0]); str++) {
		n = n * 10;
		n += (str[0] - '0');
	}

	return n;
}


/*
 * Return the numeric value of "str", as a double.
 */
double getnum_d(char *str)
{
	double n = 0.0;
	double step = 1;

	while (str[0] != 0 && (!my_isdigit(str[0]))) str++;

	for (; my_isdigit(str[0]); str++) {
		n = n * 10;
		n += (str[0] - '0');
	}

	if ((str[0] != '.') && (str[0] != ',')) return n;

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
	return (int)getnum_ll(str);
}

/* EOF */
