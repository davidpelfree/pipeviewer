/*
 * Functions for opening and closing files.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


/*
 * Try to work out the total size of all data by adding up the sizes of all
 * input files. If any of the input files are of indeterminate size (i.e.
 * they are a pipe), the total size is set to zero.
 */
void main_getsize(opts_t opts)
{
	struct stat64 sb;
	int i, fd;

	opts->size = 0;

	if (opts->argc < 1) {
		if (fstat64(STDIN_FILENO, &sb) == 0)
			opts->size = sb.st_size;
		return;
	}

	for (i = 0; i < opts->argc; i++) {
		if (strcmp(opts->argv[i], "-") == 0) {
			if (fstat64(STDIN_FILENO, &sb) == 0) {
				opts->size = sb.st_size;
			} else {
				opts->size = 0;
				return;
			}
		}

		if (stat64(opts->argv[i], &sb) == 0) {
			if (S_ISBLK(sb.st_mode)) {
				/*
				 * Get the size of block devices by opening
				 * them and seeking to the end.
				 */
				fd = open64(opts->argv[i], O_RDONLY);
				if (fd >= 0) {
					opts->size +=
					    lseek64(fd, 0, SEEK_END);
					close(fd);
				}
			} else {
				opts->size += sb.st_size;
			}
		}
	}
}


/*
 * Close the given file descriptor and open the next one, whose number in
 * the list is "filenum", returning the new file descriptor (or negative on
 * error). It is an error if the next input file is the same as the file
 * stdout is pointing to.
 */
int main_nextfd(opts_t opts, int filenum, int oldfd)
{
	struct stat64 isb;
	struct stat64 osb;
	int fd;

	if (oldfd > 0) {
		if (close(oldfd)) {
			fprintf(stderr, "%s: %s: %s\n",
				opts->program_name,
				_("failed to close file"),
				strerror(errno));
			return -1;
		}
	}

	if (filenum >= opts->argc)
		return -1;

	if (filenum < 0)
		return -1;

	if (strcmp(opts->argv[filenum], "-") == 0) {
		fd = STDIN_FILENO;
	} else {
		fd = open64(opts->argv[filenum], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "%s: %s: %s: %s\n",
				opts->program_name,
				_("failed to read file"),
				opts->argv[filenum], strerror(errno));
			return -1;
		}
	}

	if (fstat64(fd, &isb)) {
		fprintf(stderr, "%s: %s: %s: %s\n",
			opts->program_name,
			_("failed to stat file"),
			opts->argv[filenum], strerror(errno));
		close(fd);
		return -1;
	}

	if (fstat64(STDOUT_FILENO, &osb)) {
		fprintf(stderr, "%s: %s: %s\n",
			opts->program_name,
			_("failed to stat output file"), strerror(errno));
		close(fd);
		return -1;
	}

	/*
	 * Check that this new input file is not the same as stdout's
	 * destination.
	 */
	if (isb.st_dev != osb.st_dev)
		return fd;
	if (isb.st_ino != osb.st_ino)
		return fd;

	fprintf(stderr, "%s: %s: %s\n",
		opts->program_name,
		_("input file is output file"), opts->argv[filenum]);
	close(fd);
	return -1;
}

/* EOF */
