/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 0.1
 *
 */

#include "main.h"

static const char http_get[] = "GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Connection: close\r\n\
Accept: text/plain,text/html;q=0.9\r\n\
User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.106 Safari/537.36\r\n\
\r\n";

static const char search_path[][256] = {"/includes/init.php", "/modules/clauses/clauses.sitemap.php?start=1", "/plugins/kcaptcha/kcaptcha.php?kcaptcha=1"};

static void
follow_location(domain_t * domain, const char *location, size_t len);

static void
timeout_handler(struct ev_loop *loop, struct ev_timer *watcher, int events) {
    // connection timeout occurred: close the associated socket and bail


    domain_t * domain = (domain_t *) (((char *) watcher) - offsetof(domain_t, tw)); // C magic

    debug("- timeout_handler %s", domain->domain);

    ev_io_stop(loop, &domain->io);
    ev_io_set(&domain->io, -1, 0);

    free_domain(domain);
    errno = ETIMEDOUT;
}

static void
success_checked(const domain_t *domain) {
    char buffer[MAXLINE];
    size_t len;

    debug("%s FOUND\n", domain->domain);
    __sync_fetch_and_add(&domain->options->counters.found, 1);

    len = snprintf(buffer, sizeof (buffer),
            "%s%s;\n", domain->domain, search_path[domain->index_search]);

    writen(domain->options->file.out, buffer, len);
}

static void
error_parse(const domain_t *domain) {
    char buffer[MAXLINE];
    size_t len;

    debug("error_parse %s", domain->domain);

    /*len = snprintf(buffer, sizeof (buffer),
            "%s;\n", domain->domain);

    writen(domain->options->file.parse, buffer, len);*/
}

static int
parse_response(domain_t *domain) {



    int pret = -1, minor_version, status, i;
    size_t msg_len, buflen = domain->data.len, prevbuflen = 0, num_headers;
    ssize_t rret;
    const char *msg;

    num_headers = sizeof (domain->http.headers) / sizeof (domain->http.headers[0]);

    while (1) {
        pret = phr_parse_response(domain->data.buffer, buflen, &minor_version, &status, &msg, &msg_len, domain->http.headers, &num_headers, prevbuflen);

        domain->http.num_headers = num_headers;
        domain->http.status = status;

        if (pret > 0)
            break;

        else if (pret == -1) { // ParseError;
            break;
        }

        assert(pret == -2);
        if (buflen == sizeof (domain->data.buffer)) { // RequestIsTooLongError
            break;
        }
    }

    return pret;
}

static bool 
parse_body(const char *buf, size_t len) {
    bool status = false;
    
    if (NULL != memmem(buf, len, "diafan", 6) || NULL != memmem(buf, len, "DIAFAN", 6)) {
        status = true;
    }
    
    return status;
}

static void
recv_handler(struct ev_loop *loop, struct ev_io *watcher, int events) {
    //debug("recv_handler");

    int i;

    domain_t *domain = (domain_t *) watcher;
    ev_io_stop(loop, &domain->io);
    ev_timer_stop(loop, &domain->tw);


    //debug("recv_header %s -- data buffer:%p; data len: %d", domain->domain, domain->data.buffer + domain->data.len, domain->data.len);


    ssize_t len = readn(domain->io.fd, domain->data.buffer + domain->data.len, sizeof (domain->data.buffer) - domain->data.len);

    if (len <= 0) {
        if (EAGAIN == errno) { // сокет занят буфер кончился и прочее
            //err_ret("error read socket %s: ", domain->domain);
            ev_io_start(loop, &domain->io);
            ev_timer_start(loop, &domain->tw);
            return;
        } else { // жесткая ошибка
            err_ret("error read socket %s: ", domain->domain);
            free_domain(domain);
            return;
        }
    } else {

        domain->data.len += len;

        int pret = parse_response(domain);

        debug("parse_response %s:%d", domain->domain, pret);

        if (pret > 0) {

            switch (domain->http.status) {
                case 301:
                case 302:
                {

                    for (i = 0; i != domain->http.num_headers; ++i) {
                        if (NULL != memmem(domain->http.headers[i].name, domain->http.headers[i].name_len, "Location", 8)) {
                            follow_location(domain, domain->http.headers[i].value, domain->http.headers[i].value_len);
                            return;
                            //break;
                        }
                    }
                    break;
                }

                case 200:
                {
                    if (true == parse_body(&domain->data.buffer[pret], domain->data.len - pret)) {
                        success_checked(domain);
                    } else {

                        if (++domain->index_search < (sizeof (search_path) / sizeof (search_path[0]))) {
                            //ares_gethostbyname(domain->options->ares.channel, domain->domain, AF_INET, ev_ares_dns_callback, (void *) domain);
                            http_request(domain);
                            return;
                        }
                    }
                    break;
                }


            }
        } else {
            error_parse(domain);
        }
    }


    debug("-- %s %d checked", domain->domain, domain->http.status);
    free_domain(domain);
}

static void
send_request(struct ev_loop *loop, domain_t * domain) {

    debug("send_request %s, search = %s", domain->domain, search_path[domain->index_search]);

    char buf[MAXLINE];

    ev_io_set(&domain->io, domain->io.fd, EV_READ);
    ev_set_cb(&domain->io, recv_handler);

    ev_timer_set(&domain->tw, MAXRECVTIME, 0);


    size_t len = snprintf(buf, sizeof (buf), http_get, search_path[domain->index_search], domain->domain); /* this is safe */



    if (len == writen(domain->io.fd, buf, len)) {

        //bzero(domain->data.buffer, sizeof(domain->data.buffer));
        domain->data.len = 0;

        ev_io_start(loop, &domain->io);
        ev_timer_start(loop, &domain->tw);

    } else {
        free_domain(domain);
    }
}

static void
connect_handler(struct ev_loop *loop, struct ev_io *watcher, int events) {

    domain_t * domain = (domain_t *) watcher;
    ev_io_stop(loop, &domain->io);
    ev_timer_stop(loop, &domain->tw);

    debug("-- connected %s", domain->domain);


    int error;
    socklen_t len = sizeof (error);
    if (getsockopt(domain->io.fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error) {
        free_domain(domain);
        errno = error;

    } else {
        send_request(loop, domain);
    }

}

int
http_request(domain_t * domain) {


    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    int n;

    int flags = Fcntl(sockfd, F_GETFL, 0);
    Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    ev_io_init(&domain->io, connect_handler, sockfd, EV_WRITE);
    ev_io_start(domain->options->loop, &domain->io);

    ev_timer_init(&domain->tw, timeout_handler, MAXCONTIME, 0);
    ev_timer_start(domain->options->loop, &domain->tw);

    if ((n = connect(sockfd, (struct sockaddr *) &domain->servaddr, sizeof (domain->servaddr))) < 0)
        if (errno != EINPROGRESS)
            return (-1);

    return 0;
}

static bool
is_valid_location(const domain_t *domain, const char *location, size_t len) {
    // "http://1000heads.comhttp://1000HEADS.RU/includes/init.php" 
    bool valid = false;
    char buffer[MAXLINE];
    size_t s = 0;


    if (NULL != location) {
        if (len > 0 && location[len - 1] != '/' && NULL != memmem(location, len, ".php", 4)) {

            //debug("-- follow location %.*s", 1, &location[len-1]);

            char *p = NULL;


            if (NULL != (p = memmem(location, len, "http://", 7))) {
                p = p + 7;
                if (NULL == memmem(p, len - (p - location), "http://", 7)) {
                    if (NULL == memmem(p, len - (p - location), "http/", 5)) {
                        if (NULL == memmem(p, len - (p - location), ".html", 5)) {
                            valid = true;
                        }
                    }
                }
            }

            if (true == valid) {
                valid = false;
                int i = 0;
                for (i = 0; i < sizeof (search_path) / sizeof (search_path[0]); i++) {
                    s = snprintf(buffer, sizeof (buffer),
                            "%s%s", domain->domain, search_path[i]);

                    //debug("-- %s --", buffer[s-1]);


                    if (NULL != memmem(location, len, buffer, s) && location[len - 1] == buffer[s - 1]) {
                        valid = true;
                        break;
                    }
                }
            }

        }
    }

    if (false == valid) {
        debug("- invalid location");
    }

    return valid;
}

static void
follow_location(domain_t * domain, const char *location, size_t len) {
    debug("-- follow location %.*s", (int) len, location);

    __sync_fetch_and_add(&domain->options->counters.follow, 1);

    char *host;
    char c;
    size_t host_len;



    if (phr_parse_host(location, len, (const char **)&host, &host_len) > 0) {

        free(domain->domain);

        c = host[host_len];
        host[host_len] = 0;

        domain->domain = strdup(host);

        host[host_len] = c;

        if (false == is_valid_location(domain, location, len)) {
            free_domain(domain);
            return;
        }

        http_request(domain);
    }

}