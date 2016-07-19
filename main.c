/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 0.1
 *
 */

#include "main.h"

static int sigterm;
#define CHECK_TERM() if(0 != sigterm) break;

static void
sig_handler(int signum) {
    sigterm = 1;
}

static void
print_usage(const char * name) {
    err_quit("Usage: %s [KEY]... IP-LIST\n\n\
\t-n\tnumber asynchronous requests, default %d\n\
\t-o\toutput file found domains, default stdout\n\n", name, MAXPENDING);
}

static int
print_stat(options_t *options, const long time) {
    char buf[MAXLINE];
    size_t len;


    len = snprintf(buf, sizeof (buf),
            "Crawler: %zu; found: %zu; \
pending requests: %d; time: %ld milliseconds\n",
            options->counters.domains,
            options->counters.found,
            options->pending_requests,
            time
            );

    return writen(OUT_DEFAULT, buf, len);
}

static domain_t *
create_domain(options_t *options, const char *buf, const size_t size) {

    domain_t * domain = (domain_t *) malloc(sizeof (domain_t));
    if (NULL != domain) {
        domain->options = options;

        off_t offset = 0;
        unsigned int *len = (unsigned int *) buf;
        offset += sizeof (unsigned int);
        domain->domain = (char *) malloc((*len) + 1);
        memcpy(domain->domain, &buf[offset], *len);
        domain->domain[*len] = 0; // null char
        offset += *len;

        len = (unsigned int *) &buf[offset];
        offset += sizeof (unsigned int);
        memcpy(&domain->servaddr.sin_addr, &buf[offset], *len);
        offset += *len;
        
        domain->servaddr.sin_family = AF_INET;
        domain->servaddr.sin_port = htons(80);
        domain->index_search = 0;
        
        __sync_fetch_and_add(&domain->options->counters.domains, 1);

        debug("%s: %s\n", domain->domain, inet_ntoa((struct in_addr) domain->servaddr.sin_addr));
        
        if(http_request(domain) < 0) {
            err_ret("http_request");
        }
    }

    return domain;
}

void
free_domain(domain_t *domain) {

    debug("-- free domain %s", domain->domain);

    if (NULL != domain->domain) {
        free(domain->domain);
    }
    
    if(domain->io.fd > 0) { /* close socket */
        close(domain->io.fd);
    }

    free(domain);
}

static void
main_loop(options_t *options) {
    off_t offset = 0;
    char buf[MAXLINE];
    size_t pending_requests = 0;


    while (offset < options->file.len) {

        file_record rec;
        ssize_t len = readn(options->file.in, &rec, sizeof (rec));

        if (sizeof (rec) == len) {
            offset += len;
            if (MAGIC == rec.magic && rec.data_len > 0) {

                len = readn(options->file.in, buf, rec.data_len);
                if (rec.data_len == len) {
                    offset += len;

                    domain_t * domain = create_domain(options, buf, rec.data_len);
                    if (NULL != domain) {
                        ++pending_requests;
                    }

                    if (MAXPENDING == pending_requests) {
                        debug("pending_requests: %zu", pending_requests);
                        ev_run(options->loop, 0);
                        pending_requests = 0;
                    }

                }

            } else {
                debug("error file_record");
                continue;
            }

        } else {
            err_ret("error read");
            break;
        }

        CHECK_TERM();
    }


    if (pending_requests > 0) {
        debug("pending_requests: %zu", pending_requests);
        ev_run(options->loop, 0);
        pending_requests = 0;
    }



}

int
main(int argc, char** argv) {

    const long time_start = mtime();

    options_t options;
    char *opts = "n:o:";
    int opt, status = EXIT_SUCCESS;

    if (1 == argc) {
        print_usage(argv[0]);
    }

    sigterm = 0;
    (void) signal(SIGHUP, sig_handler);
    (void) signal(SIGINT, sig_handler);
    (void) signal(SIGTERM, sig_handler);

    memset(&options, 0, sizeof (options_t));

    options.file.out = OUT_DEFAULT;
    options.loop = EV_DEFAULT;
    options.pending_requests = MAXPENDING;

    while ((opt = getopt(argc, argv, opts)) != -1) {
        switch (opt) {
            case 'n':
                options.pending_requests = atoi(optarg);
                break;
            case 'o':
                if ((options.file.out = open(optarg, O_APPEND | O_CREAT | O_WRONLY,
                        S_IRUSR | S_IWUSR)) < 0) {
                    err_sys("[E] open output file %s", optarg);
                }
                break;
            case '?':
                print_usage(argv[0]);
        }
    }

    if (argc == optind) {
        print_usage(argv[0]);
    }

    if ((options.file.in = open(argv[optind], O_RDONLY)) > 0) {

        struct stat statbuf;

        if (fstat(options.file.in, &statbuf) < 0 && S_ISREG(statbuf.st_mode)) {
            err_ret("fstat");
            status = EXIT_FAILURE;
        } else {
            options.file.len = statbuf.st_size;
            (void) main_loop(&options);
        }

        close(options.file.in);
    } else {
        err_ret("[E] domain list %s", argv[optind]);
        status = EXIT_FAILURE;
    }

    print_stat(&options, (mtime() - time_start));
    return status;
}

