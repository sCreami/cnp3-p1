#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "locales.h"
#include "socket.h"

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */

/* This structure will store the shared parameters of all functions. Its
 * definition can be found in locales.h */
struct config locales = {
    .idef     = 1,
    .addr     = "localhost",
    .port     = 8080,
    .filename = NULL,
    .verbose  = 0,
    .passive  = 0,
};

/* In verbose mode, it prints the meta datas contained inside locales like the
 * address, the port, the filename, or the open socket ... */
void print_locales(void)
{
    printf(KCYN "---- args ----    \n"
           KCYN "Address "KNRM": %s\n"
           KCYN "Port    "KNRM": %d\n"
           KCYN "File    "KNRM": %s\n"
           KCYN "Passive "KNRM": %d\n"
           KCYN "--------------    \n",
           locales.addr, locales.port,
           (locales.filename ? locales.filename : "stdout"),
           locales.passive);
}

int main(int argc, char **argv)
{
    int ok = 1; // starting okay

    arguments_parser(argc, argv);

    if (locales.verbose)
        print_locales();

    ok = connect_socket();

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
