/* receiver.c */

#include "receiver.h"
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
           (locales.filename ? locales.filename : "stdout"),
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
