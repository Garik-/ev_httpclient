/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 1.0
 */

#include "main.h"
#include <syslog.h>  /* for syslog() */


static void
err_doit(int errnoflag, const char *fmt, va_list ap) {
    int errno_save, n;
    char buf[MAXLINE];

    errno_save = errno; /* value caller might want printed */
#ifdef HAVE_VSNPRINTF
    vsnprintf(buf, sizeof (buf), fmt, ap); /* this is safe */
#else
    vsprintf(buf, fmt, ap); /* this is not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, sizeof (buf) - n, "%s:%d - %s", __FILE__, __LINE__, strerror(errno_save));
    strcat(buf, "\n");


    fflush(stdout); /* in case stdout and stderr are the same */
    fputs(buf, stderr);
    fflush(stderr);

    return;
}

/* Nonfatal error related to a system call.
 * Print a message and return. */

void
err_ret(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    return;
}

void
err_sys(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    exit(1);
}

void
err_quit(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}