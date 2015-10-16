#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "locales.h"
#include "socket.h"

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */
#include "packet.h" /*packet related functions and structures*/

/* This structure will store the shared parameters of all functions. Its
 * definition can be found in locales.h */
struct config locales = {
    .idef     = 0,
    .addr     = "::1",
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
           KCYN "--------------    \n" KNRM,
           locales.addr, locales.port,
           (locales.filename ? locales.filename : "stdin"));
}

#define LENGTH 4 + 512 + 4

int perform_transfer(void)
{
    pkt_t *pkt;
    int ofd, read_size;
    char buffer[LENGTH];

    ofd = (locales.filename ? open(locales.filename, O_RDONLY) : fileno(stdin));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, KGRN"[transf]"KNRM" Performing\n");

    while ((read_size = read(ofd, buffer, 512)))
    {
        if (read_size < 0) {
            perror("read");
            close(ofd);
            return 0;
        }

        pkt = pkt_build(PTYPE_DATA, locales.window, locales.seqnum,
                        read_size, buffer);

        if (!pkt) {
            perror("pkt_build");
            close(ofd);
            return 0;
        }

        if (pkt_encode(pkt, buffer, (size_t *)&read_size) != PKT_OK) {
            perror("pkt_encode");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        if (write(locales.sockfd, buffer, read_size) == -1) {
            perror("write");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        pkt_del(pkt);
    }

    if (locales.filename)
        close(ofd);

    close(locales.sockfd);
    return 1;
}

#undef LENGTH

int main(int argc, char **argv)
{
    int ok = 1; // starting okay

    arguments_parser(argc, argv);

    if (locales.verbose)
        print_locales();

    ok = connect_socket();

    if (ok)
        ok = perform_transfer();

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
