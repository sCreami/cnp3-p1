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
    .window   = 0,
    .seqnum   = 0,
};

/* In verbose mode, it prints the meta datas contained inside locales like the
 * address, the port, the filename, or the open socket ... */
void print_locales(void)
{
    fprintf(stderr, KCYN "---- args ----    \n"
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
    fd_set rfds;
    size_t length;
    struct timeval tv;
    char buffer[LENGTH];
    int ofd, read_size, recv_size;

    ofd = (locales.filename ? open(locales.filename, O_RDONLY) : fileno(stdin));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    tv = (struct timeval) {
        .tv_sec  = 1,
        .tv_usec = 0,
    };

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    while ((read_size = read(ofd, buffer, 512)))
    {
        if (read_size < 0) {
            perror("read");
            close(ofd);
            return 0;
        }

        pkt = pkt_build(PTYPE_DATA, locales.window, locales.seqnum,
                        read_size, buffer);
        locales.seqnum = (locales.seqnum + 1) % 256;

        if (!pkt) {
            perror("pkt_build");
            close(ofd);
            return 0;
        }

        length = read_size;
        if (pkt_encode(pkt, buffer, &length) != PKT_OK) {
            perror("pkt_encode");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        if (send(locales.sockfd, buffer, length, 0) == -1) {
            perror("send");
            pkt_del(pkt);
            close(ofd);
            return 0;
        }

        pkt_del(pkt);

        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) == -1) {
            perror("select");
            close(ofd);
            return 0;
        }

        if (FD_ISSET(locales.sockfd, &rfds))
        {
            recv_size  = recv(locales.sockfd, buffer, sizeof(buffer), 0);

            if (recv_size < 0) {
                perror("recv");
                close(ofd);
                return 0;
            }

            pkt = pkt_new();

            if (!pkt) {
                perror("pkt_new");
                close(ofd);
                return 0;
            }

            if (pkt_decode(buffer, (size_t) recv_size, pkt) == PKT_OK)
            {
                if (pkt->type == PTYPE_ACK)
                {
                    //get rid of pkt with pkt->seqnum
                    if (locales.verbose)
                        fprintf(stderr, "["KYEL" warn "KNRM"] received ack "
                                "for seq %d\n", (int) pkt->seqnum);
                }
                else if (pkt->type == PTYPE_NACK)
                {
                    //send pkt with pkt->seqnum again
                    if (locales.verbose)
                        fprintf(stderr, "["KYEL" warn "KNRM"] pkt %d has been"
                                "received damaged\n", (int) pkt->seqnum);
                }
                else
                {
                    if (locales.verbose)
                        fprintf(stderr, "["KYEL" warn "KNRM"] receiver"
                                "shouldn't send data\n");
                }
            }

            pkt_del(pkt);
        }
    }

    if (locales.verbose)
        fprintf(stderr, "["KGRN"  ok  "KNRM"] Transfered\n");

    if (locales.filename)
        close(ofd);

    if (shutdown(locales.sockfd, SHUT_RDWR) == -1) {
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
        ok = perform_transfer();

    close(locales.sockfd);

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] All done\n");

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
