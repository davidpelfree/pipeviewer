/*
 * Cursor positioning functions.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


int getnum_i(char *);


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
	char tmp[32];			 /* RATS: ignore (checked OK) */
	char *ttyfile;
	int fd;

	if (!options->cursor)
		return;

	ttyfile = ttyname(STDERR_FILENO);   /* RATS: ignore (unimportant) */
	if (!ttyfile)
		return;

	fd = open(ttyfile, O_RDWR);	    /* RATS: ignore (no race) */
	if (fd < 0) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to open terminal"), strerror(errno));
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
	read(fd, tmp, 6);		    /* RATS: ignore (OK) */
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
 * Output a single-line update, moving the cursor to the correct position to
 * do so.
 */
void cursor_update(opts_t options, char *str)
{
	struct flock lock;
	char tmp[32];			 /* RATS: ignore (checked OK) */

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLKW, &lock);

	sprintf(tmp, "\033[%d;1H", cursor__y);

	write(STDERR_FILENO, tmp,
	      strlen(tmp));		/* RATS: ignore */
	write(STDERR_FILENO, str,
	      strlen(str));		/* RATS: ignore */

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLK, &lock);
}


/*
 * Reposition the cursor to a final position.
 */
void cursor_fini(opts_t options)
{
	char tmp[128];			 /* RATS: ignore (checked OK) */
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLKW, &lock);

	if (cursor__y < 0)
		cursor__y = 0;
	if (cursor__y > 10000)
		cursor__y = 10000;

	sprintf(tmp, "\033[%d;1H\n", cursor__y);	/* RATS: ignore */
	write(STDERR_FILENO, tmp, strlen(tmp));	/* RATS: ignore */

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(STDERR_FILENO, F_SETLK, &lock);
}

/* EOF */
