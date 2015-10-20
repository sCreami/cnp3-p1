#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "locales.h"
#include "socket.h"

#include <sys/time.h>
#include <sys/types.h>

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
    .window   = 31,
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
                    KCYN "Passive "KNRM": %d\n"
                    KCYN "--------------    \n" KNRM,
                    locales.addr, locales.port,
                    (locales.filename ? locales.filename : "stdout"),
                    locales.passive);
}

pkt_t * withdraw_pkt(pkt_t *buffer[32], int i)
{
    if (buffer[i] == NULL)
        return NULL; // and print something ?

    pkt_t * result = buffer[i];
    locales.window++;
    buffer[i] = NULL;
    return result;
}

int store_pkt(pkt_t *buffer[32], pkt_t *pkt)
{
    int seqnum_diff;

    seqnum_diff = pkt->seqnum - locales.seqnum;

    if (seqnum_diff < 0 || seqnum_diff > 31)
    {
        pkt_del(pkt);
        return 0;
    }

    if (buffer[seqnum_diff] != NULL)
        return 1;

    locales.window--;
    buffer[seqnum_diff] = pkt;

    return 0;
}

int write_in_seq_pkt(int fd, pkt_t *buffer[32])
{
    pkt_t *pkt;
    int i;

    for (i = 0; i < 32; i++)
    {
        pkt = withdraw_pkt(buffer, i);

        if (pkt && pkt->seqnum == locales.seqnum)
        {
            locales.seqnum = (locales.seqnum + 1) % 256;

            if (write(fd, pkt->payload, pkt->length) == -1) {
                perror("write");
                pkt_del(pkt);
                return 1;
            }
        }

        pkt_del(pkt);
    }

    /*
     * if more than 1 pkt has been sent, then content of the buffer is moved to
     * the left.
     */

    if (locales.window < 31)
    {
        memcpy(buffer, &buffer[i], 32 - i);
        memset(&buffer[i], 0, i);
    }

    return 0;
}

int send_control_pkt(ptypes_t type)
{
    char buffer[8];
    size_t length;
    pkt_t * pkt;

    pkt = pkt_build(type, locales.window, locales.seqnum, 0, NULL);
    length = 8;

    if (!pkt) {
        perror("pkt_build");
        return 1;
    }

    if (pkt_encode(pkt, buffer, &length) != PKT_OK) {
        perror("pkt_encode");
        pkt_del(pkt);
        return 1;
    }

    if (send(locales.sockfd, buffer, length, 0) == -1) {
        perror("send");
        pkt_del(pkt);
        return 1;
    }

    pkt_del(pkt);

    return 0;
}

#define LENGTH 4 + 512 + 4

int receive_data(void)
{
    pkt_t *pkt;
    fd_set rfds;
    struct timeval tv;
    int ofd, read_size;
    char buffer[LENGTH];
    pkt_t *reception_window[32];
    pkt_status_code decode_status;

    ofd = (locales.filename ? open(locales.filename, O_WRONLY | O_CREAT |
           O_TRUNC, 0644) : fileno(stdout));
    bzero(reception_window, 32 * sizeof(pkt_t *));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    for (;;)
    {
        tv = (struct timeval) {
            .tv_sec  = 2, // should be enough to
            .tv_usec = 0, // retrieve all packets
        };

        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) == -1) {
            perror("select");
            close(ofd);
            return 0;
        }

        if (FD_ISSET(locales.sockfd, &rfds))
        {
            read_size  = recv(locales.sockfd, buffer,
                              sizeof(buffer), MSG_DONTWAIT);

            if (read_size == -1) {
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

            decode_status = pkt_decode(buffer, (size_t) read_size, pkt);

            if (decode_status == PKT_OK)
            {
                if (store_pkt(reception_window, pkt)) {
                    perror("store_pkt");
                    close(ofd);
                    return 0;
                }

                if (write_in_seq_pkt(ofd, reception_window)) {
                    perror("write_in_seq_pkt");
                    close(ofd);
                    return 0;
                }

                send_control_pkt(PTYPE_ACK);
            }
            else
            {
                send_control_pkt(PTYPE_NACK);
            }
        }
        else
        {
            // assuming -1 is EOF after 2 sec of waiting packets
            if (recv(locales.sockfd, buffer,
                sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == -1) {
            close(ofd);
            return 1;
            }
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
        ok = receive_data();

    close(locales.sockfd);

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] All done\n");

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
