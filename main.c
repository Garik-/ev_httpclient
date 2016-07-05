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

static void
main_loop(options_t *options) {
   
}

int
main(int argc, char** argv) {

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

    if ((options.file.in = open(argv[optind], O_RDWR)) > 0) {

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


    return status;
}

