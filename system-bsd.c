/* signal-safe read and write (for BSD slow devices). writeall() also
allows partial writes */

#include <errno.h>

#include "rc.h"
#include "jbwrap.h"

static char *safe_buf;
static size_t safe_remain;

extern void writeall(int fd, char *buf, size_t remain) {
	int i;

	safe_buf = buf;
	safe_remain = remain;
	for (i = 0; safe_remain > 0; buf += i, safe_remain -= i) {
		interrupt_happened = FALSE;
		if (!sigsetjmp(slowbuf.j, 1)) {
			slow = TRUE;
			if (interrupt_happened)
				break;
			else if ((i = write(fd, safe_buf, safe_remain)) <= 0)
				break; /* abort silently on errors in write() */
		} else
			break;
		slow = FALSE;
	}
	slow = FALSE;
	sigchk();
}

extern int rc_read(int fd, char *buf, size_t n) {
	ssize_t r;

	interrupt_happened = FALSE;
	if (!sigsetjmp(slowbuf.j, 1)) {
		slow = TRUE;
		if (!interrupt_happened)
			r = read(fd, buf, n);
		else
			r = -2;
	} else
		r = -2;
	slow = FALSE;
	if (r == -2) {
		errno = EINTR;
		r = -1;
	}
	return r;
}

static int r = -1;
extern pid_t rc_wait(int *stat) {
	interrupt_happened = FALSE;
	if (!sigsetjmp(slowbuf.j, 1)) {
		slow = TRUE;
		if (!interrupt_happened)
			r = wait(stat);
		else
			r = -2;
	} else
		r = -2;
	slow = FALSE;
	if (r == -2) {
		errno = EINTR;
		r = -1;
	}
	return r;
}

/* signal-safe readline wrapper */
#if READLINE
extern char *rc_readline(char *prompt) {
	char *r;

	interrupt_happened = FALSE;
	if (!sigsetjmp(slowbuf.j, 1)) {
		slow = TRUE;
		if (!interrupt_happened)
			r = readline(prompt);
		else
			r = NULL;
	} else
		r = NULL;
	slow = FALSE;
	if (r == NULL)
		errno = EINTR;
	sigchk();
	return r;
}
#endif /* READLINE */
