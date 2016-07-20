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
#include <stddef.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ev.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#include "picohttpparser.h"


#define OUT_DEFAULT STDOUT_FILENO
#define MAXLINE  4096
#define MAXPENDING 10
#define MAGIC 0xDEADBEEF

#define MAXCONTIME 3.0 // in seconds
#define MAXRECVTIME 3.0 // in seconds

#define DEBUG

#ifdef DEBUG
#define debug(fmt, ...)   do{ \
  fprintf(stderr, "[DEBUG] %s:%d: ", __FILE__, __LINE__); \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        if (fmt[strlen(fmt) - 1] != 0x0a) { fprintf(stderr, "\n"); } \
        } while(0)
#else
#define debug(fmt, ...) do {} while(0);
#endif

    typedef struct {
        struct ev_loop * loop;

        struct {
            int in;
            int out;
            off_t len;
        } file;

        struct {
            ssize_t domains;
            ssize_t found;
            ssize_t follow;
        } counters;

        int pending_requests;

    } options_t;

#pragma pack(push,1)

    typedef struct {
        unsigned int magic;
        unsigned int data_len;
    } file_record;

    typedef struct {
        unsigned int len;
        char * buf;
    } lsp;

#pragma pack(pop)

    typedef struct {
        //size_t domain_len;

        struct ev_io io;
        struct ev_timer tw;
        options_t * options;
        char *domain;

        struct {
            char buffer[MAXLINE];
            size_t len;
        } data;

        struct {
            struct phr_header headers[16];
            size_t num_headers;
            unsigned int status;
        } http;
        struct sockaddr_in servaddr;
        //unsigned int keep_alive;
        unsigned int index_search;
    } domain_t;

    void
    err_ret(const char *fmt, ...);

    void
    err_quit(const char *fmt, ...);

    void
    err_sys(const char *fmt, ...);

    long
    mtime(void);

    size_t /* Read "n" bytes from a descriptor. */
    readn(int fd, void *vptr, size_t n);

    size_t /* Write "n" bytes to a descriptor. */
    writen(int fd, const void *vptr, size_t n);


    int
    Socket(int family, int type, int protocol);

    int
    Fcntl(int fd, int cmd, int arg);

    void
    free_domain(domain_t *domain);

    int
    http_request(domain_t * domain);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

