#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "locales.h"
#include "socket.h"

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */
#include "packet.h" /*packet related functions and structures*/

/* This structure will store the shared parameters of all functions. Its
 * definition can be found in locales.h */
struct config locales = {
    .idef     = 1,
    .addr     = "::1",
    .port     = 8080,
    .filename = NULL,
    .verbose  = 0,
    .passive  = 0,
    .window = 0,
    .seqnum = 0,
};

/* In verbose mode, it prints the meta datas contained inside locales like the
 * address, the port, the filename, or the open socket ... */
void print_locales(void)
{
    fprintf(stderr, KCYN "---- args ----    \n"
                    KCYN "Address "KNRM": %s\n"
                    KCYN "Port    "KNRM": %d\n"
                    KCYN "File    "KNRM": %s\n"
                    KCYN "Passive "KNRM": %d\n"
                    KCYN "--------------    \n" KNRM,
                    locales.addr, locales.port,
                    (locales.filename ? locales.filename : "stdout"),
                    locales.passive);
}

#define LENGTH 4 + 512 + 4

int receive_data(void)
{
    pkt_t *pkt;
    int ofd, read_size;
    char buffer[LENGTH];

    ofd = (locales.filename ? open(locales.filename, O_WRONLY | O_CREAT |
           O_TRUNC, 0644) : fileno(stdout));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    while ((read_size  = recv(locales.sockfd, buffer,
                              sizeof(buffer), MSG_DONTWAIT)) != -1)
    {
        pkt = pkt_new();

        if (!pkt) {
            perror("pkt_new");
            close(ofd);
            return 0;
        }

        if (pkt_decode(buffer, (size_t) read_size, pkt) != PKT_OK) {
            perror("pkt_decode");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        if (write(ofd, pkt->payload, pkt->length) == -1) {
            perror("write");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        pkt_del(pkt);
    }

    if (errno != EAGAIN) { // recv return -1 when the socket dies
        perror("recv");
        close(ofd);
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KGRN"  ok  "KNRM"] Transfered\n");

    if (locales.filename)
        close(ofd);

    if (shutdown(locales.sockfd, SHUT_RD) == -1) {
        perror("shutdown");
        return 0;
    }

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
        ok = receive_data();

    close(locales.sockfd);

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] All done\n");

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
