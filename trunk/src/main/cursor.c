/*
 * Cursor positioning functions.
 *
 * If IPC is available, then a shared memory segment is used to co-ordinate
 * cursor positioning across multiple instances of `pv'. The shared memory
 * segment contains an integer which is the original "y" co-ordinate of the
 * first `pv' process.
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

#ifdef HAVE_IPC
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif				/* HAVE_IPC */


int getnum_i(char *);


#ifdef HAVE_IPC
static int cursor__shmid = -1;		 /* ID of our shared memory segment */
static int cursor__pvcount = 1;		 /* number of `pv' processes in total */
static int cursor__pvmax = 0;		 /* highest number of `pv's seen */
static int *cursor__y_top = 0;		 /* pointer to Y coord of topmost `pv' */
static int cursor__y_offset = 0;	 /* our Y offset from this top position */
#endif				/* HAVE_IPC */
static int cursor__y_start = 0;		 /* our initial Y coordinate */


/*
 * Lock the terminal on the given file descriptor.
 */
static void cursor_lock(int fd)
{
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	while (fcntl(fd, F_SETLKW, &lock) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "%s\n", strerror(errno));
			return;
		}
	}
}


/*
 * Unlock the terminal on the given file descriptor.
 */
static void cursor_unlock(int fd)
{
	struct flock lock;

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	fcntl(fd, F_SETLK, &lock);
}


#ifdef HAVE_IPC
/*
 * Get the current number of processes attached to our shared memory
 * segment, i.e. find out how many `pv' processes in total are running in
 * cursor mode (including us), and store it in cursor__pvcount. If this is
 * larger than cursor__pvmax, update cursor__pvmax.
 */
static void cursor_ipccount(void)
{
	struct shmid_ds buf;

	buf.shm_nattch = 0;

	shmctl(cursor__shmid, IPC_STAT, &buf);
	cursor__pvcount = buf.shm_nattch;

	if (cursor__pvcount > cursor__pvmax)
		cursor__pvmax = cursor__pvcount;
}
#endif				/* HAVE_IPC */


#ifdef HAVE_IPC
/*
 * Initialise the IPC data, returning nonzero on error.
 *
 * To do this, we attach to the shared memory segment (creating it if it
 * does not exist). If we are the only process attached to it, then we
 * initialise it with the current cursor position.
 *
 * There is a race condition here: another process could attach before we've
 * had a chance to check, such that no process ends up getting an "attach
 * count" of one, and so no initialisation occurs. So, we lock the terminal
 * with cursor_lock() while we are attaching and checking.
 */
static int cursor_ipcinit(opts_t options, char *ttyfile, int terminalfd)
{
	int key;

	key = ftok(ttyfile, 'p');
	if (key < 0) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to open terminal"), strerror(errno));
		return 1;
	}

	cursor_lock(terminalfd);

	cursor__shmid = shmget(key, sizeof(int), 0600 | IPC_CREAT);
	if (cursor__shmid < 0) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to open terminal"), strerror(errno));
		cursor_unlock(terminalfd);
		return 1;
	}

	cursor__y_top = shmat(cursor__shmid, 0, 0);

	cursor_ipccount();

	/*
	 * If nobody else is attached to the shared memory segment, we're
	 * the first, so we need to initialise the shared memory with our
	 * current Y cursor co-ordinate.
	 */
	if (cursor__pvcount < 2) {
		struct termios tty;
		struct termios old_tty;
		char cpr[32];		 /* RATS: ignore (checked) */

		tcgetattr(terminalfd, &tty);
		tcgetattr(terminalfd, &old_tty);
		tty.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(terminalfd, TCSANOW | TCSAFLUSH, &tty);
		write(terminalfd, "\033[6n", 4);
		memset(cpr, 0, sizeof(cpr));
		read(terminalfd, cpr, 6);   /* RATS: ignore (OK) */
		cursor__y_start = getnum_i(cpr + 2);
		tcsetattr(terminalfd, TCSANOW | TCSAFLUSH, &old_tty);

		*cursor__y_top = cursor__y_start;
	}

	cursor__y_offset = cursor__pvcount - 1;
	if (cursor__y_offset < 0)
		cursor__y_offset = 0;

	/*
	 * If anyone else had attached to the shared memory segment, we need
	 * to read the top Y co-ordinate from it.
	 */
	if (cursor__pvcount > 1) {
		cursor__y_start = *cursor__y_top;
	}

	cursor_unlock(terminalfd);

	return 0;
}
#endif				/* HAVE_IPC */


/*
 * Initialise the terminal for cursor positioning.
 */
void cursor_init(opts_t options)
{
	char *ttyfile;
	int fd;

	if (!options->cursor)
		return;

	ttyfile = ttyname(STDERR_FILENO);   /* RATS: ignore (unimportant) */
	if (!ttyfile) {
		options->cursor = 0;
		return;
	}

	fd = open(ttyfile, O_RDWR);	    /* RATS: ignore (no race) */
	if (fd < 0) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to open terminal"), strerror(errno));
		options->cursor = 0;
		return;
	}
#ifdef HAVE_IPC
	if (cursor_ipcinit(options, ttyfile, fd)) {
		options->cursor = 0;
		close(fd);
		return;
	}
#else				/* ! HAVE_IPC */

	/*
	 * Get current cursor position + 1.
	 */
	{
		struct termios tty;
		struct termios old_tty;
		char cpr[32];		 /* RATS: ignore (checked) */

		cursor_lock(fd);

		tcgetattr(fd, &tty);
		tcgetattr(fd, &old_tty);
		tty.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(fd, TCSANOW | TCSAFLUSH, &tty);
		write(fd, "\n\033[6n", 5);
		memset(cpr, 0, sizeof(cpr));
		read(fd, cpr, 6);	    /* RATS: ignore (OK) */
		cursor__y_start = getnum_i(cpr + 2);
		tcsetattr(fd, TCSANOW | TCSAFLUSH, &old_tty);

		cursor_unlock(fd);

		if (cursor__y_start < 1)
			options->cursor = 0;
	}

#endif				/* HAVE_IPC */

	close(fd);
}


/*
 * Output a single-line update, moving the cursor to the correct position to
 * do so.
 */
void cursor_update(opts_t options, char *str)
{
	char pos[32];			 /* RATS: ignore (checked OK) */
	int y;

#ifdef HAVE_IPC
	cursor_ipccount();
#endif				/* HAVE_IPC */

	y = cursor__y_start;

#ifdef HAVE_IPC
	/*
	 * If the screen has scrolled, or is about to scroll, due to
	 * multiple `pv' instances taking us near the bottom of the screen,
	 * scroll the screen (only if we're the first `pv'), and then move
	 * our initial Y co-ordinate up.
	 */
	if ((cursor__y_start + cursor__pvmax) > options->height) {
		int offs;

		offs =
		    ((cursor__y_start + cursor__pvmax) - options->height);

		cursor__y_start -= offs;
		if (cursor__y_start < 1)
			cursor__y_start = 1;

		if (cursor__y_offset == 0) {
			cursor_lock(STDERR_FILENO);

			sprintf(pos, "\033[%d;1H", options->height);
			write(STDERR_FILENO, pos, strlen(pos));
			for (; offs > 0; offs--) {
				write(STDERR_FILENO, "\n", 1);
			}

			cursor_unlock(STDERR_FILENO);
		}
	}

	y = cursor__y_start + cursor__y_offset;
#endif				/* HAVE_IPC */

	cursor_lock(STDERR_FILENO);

	sprintf(pos, "\033[%d;1H", y);

	write(STDERR_FILENO, pos, strlen(pos));	/* RATS: ignore */
	write(STDERR_FILENO, str, strlen(str));	/* RATS: ignore */

	cursor_unlock(STDERR_FILENO);
}


/*
 * Reposition the cursor to a final position.
 */
void cursor_fini(opts_t options)
{
	char pos[128];			 /* RATS: ignore (checked OK) */
	int y;

	cursor_lock(STDERR_FILENO);

	y = cursor__y_start;

#ifdef HAVE_IPC
	if (cursor__pvmax > 0)
		y += cursor__pvmax - 1;
#endif				/* HAVE_IPC */

	if (y > options->height)
		y = options->height;

	sprintf(pos, "\033[%d;1H\n", y);    /* RATS: ignore */
	write(STDERR_FILENO, pos, strlen(pos));	/* RATS: ignore */

	cursor_unlock(STDERR_FILENO);

#ifdef HAVE_IPC
	cursor_lock(STDERR_FILENO);

	cursor_ipccount();
	shmdt(cursor__y_top);

	/*
	 * If we are the last instance detaching from the shared memory,
	 * delete it so it's not left lying around.
	 */
	if (cursor__pvcount < 2)
		shmctl(cursor__shmid, IPC_RMID, 0);

	cursor_unlock(STDERR_FILENO);
#endif				/* HAVE_IPC */
}

/* EOF */
