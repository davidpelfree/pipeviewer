/*
 * Functions for opening and closing files.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "options.h"

#include <stdio.h>
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
void main_getsize(opts_t options)
{
	struct stat64 sb;
	int i, fd;

	options->size = 0;

	if (options->argc < 1) {
		if (fstat64(STDIN_FILENO, &sb) == 0)
			options->size = sb.st_size;
		return;
	}

	for (i = 0; i < options->argc; i++) {
		if (strcmp(options->argv[i], "-") == 0) {
			if (fstat64(STDIN_FILENO, &sb) == 0) {
				options->size = sb.st_size;
			} else {
				options->size = 0;
				return;
			}
		}

		if (stat64(options->argv[i], &sb) == 0) {
			if (S_ISBLK(sb.st_mode)) {
				fd = open64(options->argv[i], O_RDONLY);
				if (fd >= 0) {
					options->size += lseek64(fd, 0, SEEK_END);
					close(fd);
				}
			} else {
				options->size += sb.st_size;
			}
		}
	}
}


/*
 * Close the given file descriptor and open the next one, whose number in
 * the list is "filenum", returning the new file descriptor (or negative on
 * error).
 */
int main_nextfd(opts_t options, int filenum, int oldfd)
{
	struct stat64 isb;
	struct stat64 osb;
	int fd;

	if (oldfd > 0) {
		if (close(oldfd)) {
			fprintf(stderr, "%s: %s: %s\n",
				options->program_name,
				_("failed to close file"),
				strerror(errno));
			return -1;
		}
	}

	if (filenum >= options->argc)
		return -1;

	if (strcmp(options->argv[filenum], "-") == 0) {
		return STDIN_FILENO;
	}

	fd = open64(options->argv[filenum], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: %s: %s: %s\n",
			options->program_name,
			_("failed to read file"),
			options->argv[filenum],
			strerror(errno));
		return -1;
	}

	if (fstat64(fd, &isb)) {
		fprintf(stderr, "%s: %s: %s: %s\n",
			options->program_name,
			_("failed to stat file"),
			options->argv[filenum],
			strerror(errno));
		close(fd);
		return -1;
	}

	if (fstat64(STDOUT_FILENO, &osb)) {
		fprintf(stderr, "%s: %s: %s\n",
			options->program_name,
			_("failed to stat output file"),
			strerror(errno));
		close(fd);
		return -1;
	}

	if (isb.st_dev != osb.st_dev)
		return fd;
	if (isb.st_ino != osb.st_ino)
		return fd;

	fprintf(stderr, "%s: %s: %s\n",
		options->program_name,
		_("input file is output file"),
		options->argv[filenum]);
	close(fd);
	return -1;
}

/* EOF */
