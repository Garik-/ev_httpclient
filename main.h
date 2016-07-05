/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 0.1
 *
 */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ev.h>
 

#define OUT_DEFAULT STDOUT_FILENO
#define MAXLINE  4096
#define MAXPENDING 10

    typedef struct {
        
        struct ev_loop * loop;

        struct {
            int in;
            int out;
            size_t len;
        } file;

        int pending_requests;

    } options_t;

    void
    err_ret(const char *fmt, ...);

    void
    err_quit(const char *fmt, ...);

    void
    err_sys(const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

