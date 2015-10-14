/* sender.c */

#include "sender.h"
#include "argpars.c" /* void arguments_parser(int argc, char **argv) */

/* In verbose mode, it prints the meta data contained inside locales like
 * the address, the port, ...
 */
void meta_print(void)
{
    printf("Address   : %s\n"
           "Port      : %d\n"
           "File      : %s\n"
           "CPU time  : %f\n",
           locales.addr, locales.port,
           (locales.filename ? locales.filename : "stdin"),
           (double)(locales.stop - locales.start)/CLOCKS_PER_SEC);
}

int create_connection(void)
{
    int sockfd;

    sockfd = connect_socket();
    if (sockfd == -1)
        return -1;

    return 1;
}

int main(int argc, char **argv)
{
    int ok = 1; // starting okay

    if (locales.verbose)
        locales.start = clock();

    arguments_parser(argc, argv);

    ok = create_connection();

    if (locales.verbose) {
        locales.stop = clock();
        meta_print();
    }

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
