/*
 * Signal handling functions.
 *
 * Copyright 2004 Andrew Wood, distributed under the Artistic License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>


static int sig__old_stderr;		 /* see sig__ttou() */
static struct timeval sig__tstp_time;	 /* see sig__tstp() / __cont() */
struct timeval sig__toffset;		 /* total time spent stopped */
sig_atomic_t sig__newsize = 0;		 /* whether we need to get term size again */


/*
 * Handle SIGTTOU (tty output for background process) by redirecting stderr
 * to /dev/null, so that we can be stopped and backgrounded without messing
 * up the terminal. We store the old stderr file descriptor so that on a
 * subsequent SIGCONT we can try writing to the terminal again, in case we
 * get backgrounded and later get foregrounded again.
 */
void sig__ttou(int s)
{
	int fd;

	fd = open("/dev/null", O_RDWR);
	if (fd < 0)
		return;

	if (sig__old_stderr == -1)
		sig__old_stderr = dup(STDERR_FILENO);

	dup2(fd, STDERR_FILENO);
	close(fd);
}


/*
 * Handle SIGTSTP (stop typed at tty) by storing the time the signal
 * happened for later use by sig__cont(), and then stopping the process.
 */
void sig__tstp(int s)
{
	gettimeofday(&sig__tstp_time, NULL);
	raise(SIGSTOP);
}


/*
 * Handle SIGCONT (continue if stopped) by adding the elapsed time since the
 * last SIGTSTP to the elapsed time offset, and by trying to write to the
 * terminal again (by replacing the /dev/null stderr with the old stderr).
 */
void sig__cont(int s)
{
	struct timeval tv;
	struct termios t;

	sig__newsize = 1;

	if (sig__tstp_time.tv_sec == 0) {
		tcgetattr(STDERR_FILENO, &t);
		t.c_lflag |= TOSTOP;
		tcsetattr(STDERR_FILENO, TCSANOW, &t);
		return;
	}

	gettimeofday(&tv, NULL);

	sig__toffset.tv_sec += (tv.tv_sec - sig__tstp_time.tv_sec);
	sig__toffset.tv_usec += (tv.tv_usec - sig__tstp_time.tv_usec);
	if (sig__toffset.tv_usec >= 1000000) {
		sig__toffset.tv_sec++;
		sig__toffset.tv_usec -= 1000000;
	}
	if (sig__toffset.tv_usec < 0) {
		sig__toffset.tv_sec--;
		sig__toffset.tv_usec += 1000000;
	}

	sig__tstp_time.tv_sec = 0;
	sig__tstp_time.tv_usec = 0;

	if (sig__old_stderr != -1) {
		dup2(sig__old_stderr, STDERR_FILENO);
		close(sig__old_stderr);
		sig__old_stderr = -1;
	}

	tcgetattr(STDERR_FILENO, &t);
	t.c_lflag |= TOSTOP;
	tcsetattr(STDERR_FILENO, TCSANOW, &t);
}


/*
 * Handle SIGWINCH (window size changed) by setting a flag.
 */
void sig__winch(int s)
{
	sig__newsize = 1;
}


/*
 * Initialise signal handling.
 */
void sig_init(void)
{
	struct sigaction sa;

	sig__old_stderr = -1;
	sig__tstp_time.tv_sec = 0;
	sig__tstp_time.tv_usec = 0;
	sig__toffset.tv_sec = 0;
	sig__toffset.tv_usec = 0;

	/*
	 * Ignore SIGPIPE
	 */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);

	/*
	 * Handle SIGTTOU by continuing with output switched off, so that we
	 * can be stopped and backgrounded without messing up the terminal
	 */
	sa.sa_handler = sig__ttou;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGTTOU, &sa, NULL);

	/*
	 * Handle SIGTSTP by storing the time the signal happened for later
	 * use by sig__cont(), and then stopping the process
	 */
	sa.sa_handler = sig__tstp;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGTSTP, &sa, NULL);

	/*
	 * Handle SIGCONT by adding the elapsed time since the last SIGTSTP
	 * to the elapsed time offset, and by trying to write to the
	 * terminal again
	 */
	sa.sa_handler = sig__cont;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGCONT, &sa, NULL);

	/*
	 * Handle SIGWINCH by setting a flag to let the main loop know it
	 * has to reread the terminal size.
	 */
	sa.sa_handler = sig__winch;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGWINCH, &sa, NULL);
}


/*
 * Stop reacting to SIGTSTP and SIGCONT.
 */
void sig_nopause(void)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGTSTP, &sa, NULL);

	sa.sa_handler = SIG_DFL;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGCONT, &sa, NULL);
}


/*
 * Start catching SIGTSTP and SIGCONT again.
 */
void sig_allowpause(void)
{
	struct sigaction sa;

	sa.sa_handler = sig__tstp;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGTSTP, &sa, NULL);

	sa.sa_handler = sig__cont;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGCONT, &sa, NULL);
}


/*
 * If we'd redirected stderr to /dev/null, check every second or so to see
 * whether we can write to the terminal again - this is so that if we get
 * backgrounded, then foregrounded again, we start writing to the terminal
 * again.
 */
void sig_checkbg(void)
{
	static time_t next_check = 0;
	struct termios t;

	if (time(NULL) < next_check)
		return;

	next_check = time(NULL) + 1;

	if (sig__old_stderr == -1)
		return;

	dup2(sig__old_stderr, STDERR_FILENO);
	close(sig__old_stderr);
	sig__old_stderr = -1;

	tcgetattr(STDERR_FILENO, &t);
	t.c_lflag |= TOSTOP;
	tcsetattr(STDERR_FILENO, TCSANOW, &t);
}

/* EOF */
