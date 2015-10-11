#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h> /* inet_pton */
#include <string.h>
#include <time.h>

#include "receiver.h"

static struct rcv_config locales = {
    .addr     = "localhost",
    .port     = 8080,
    .filename = "/stdout",
    .verbose  = 0,
};


/* Fill the local configuration following the parameters given in arguments.
 * It reads argc and argv to fill the static structure locales with valid data.
 */
void arguments_parser(int argc, char **argv)
{
    char *opt, *endptr;
    int port;

    for (int i = 1; i < argc; i++) {
        opt = argv[i];

        recycle:
        if (!strncmp(opt, "-f", 2) || !strncmp(opt, "--filename", 10)) {
            // followed by filename if presents
            opt = argv[++i];
            locales.filename = (opt ? opt : "/stdout");
        }

        else if (strstr(opt, ":") || !strncmp(opt, "localhost", 9)) {
            // assuming address
            locales.addr = opt;

            // followed by port if presents
            if ((opt = argv[++i])) {
                port = atoi(opt);
                locales.port = (port ? port : 8080);
                if (!port) goto recycle;
            }
        }

        else if (!strncmp(opt, "-v", 2) || !strncmp(opt, "--verbose", 9)) {
            // define verbosity
            locales.verbose = 1;
        }

        else {
            // looking for aliens
            strtol(opt, &endptr, 10);
            if (*endptr != '\0')
                fprintf(stderr, "illegal option: %s\n", opt);
        }
    }
}

/* In verbose mode, it prints the meta data contained inside locales like
 * the address, the port, ...
 */
void meta_print(void)
{
    printf("Address   : %s\n"
           "Port      : %d\n"
           "File      : %s\n"
           "CPU time  : %f\n",
           locales.addr, locales.port, locales.filename,
           (double)(locales.stop - locales.start)/CLOCKS_PER_SEC);
}

int main(int argc, char **argv)
{
    if (locales.verbose)
        locales.start = clock();


    arguments_parser(argc, argv);


    if (locales.verbose) {
        locales.stop = clock();
        meta_print();
    }

    return EXIT_SUCCESS;
}
