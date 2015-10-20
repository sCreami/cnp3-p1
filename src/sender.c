#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "locales.h"
#include "socket.h"

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */
#include "packet.h" /* packet related functions and structures */

/* This structure will store the shared parameters of all functions. Its
 * definition can be found in locales.h */
struct config locales = {
    .idef     = 0,
    .addr     = "::1",
    .port     = 64341,
    .filename = NULL,
    .verbose  = 0,
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
                    KCYN "--------------    \n" KNRM,
                    locales.addr, locales.port,
                    (locales.filename ? locales.filename : "stdin"));
}

pkt_t * withdraw_pkt(pkt_t *buffer[32], int seqnum)
{
    pkt_t *pkt;
    int i;

    for (i = 0; i < 32; i++)
        if (buffer[i] != NULL && buffer[i]->seqnum == seqnum)
        {
            locales.window++;
            pkt = buffer[i];
            buffer[i] = NULL;
            return pkt;
        }

    return NULL;
}

int store_pkt(pkt_t *buffer[32], pkt_t *pkt)
{
    int i;

    for (i = 0; i < 32; i++)
        if (buffer[i] ==  NULL)
        {
            locales.window--;
            buffer[i] = pkt;
            return 0;
        }

    return 1;
}

void free_pkt_buffer(pkt_t *buffer[32])
{
    int i;

    for  (i = 0; i < 32; i++)
        pkt_del(buffer[i]);

    locales.window = 31;
}

void drop_pkt(pkt_t *buffer[32], int seqnum)
{
    pkt_t * pkt;

    if (seqnum < 0)
        seqnum += 256;

    pkt = withdraw_pkt(buffer, seqnum);

    if (pkt) {
        drop_pkt(buffer, seqnum - 1);
        pkt_del(pkt);
    }
}

int is_buffer_empty(pkt_t *buffer[32])
{
    int i;
    for (i = 0; i < 32; i++)
        if (buffer[i])
            return 0;
    return 1;
}

int send_pkt(pkt_t *buffer[32], int seqnum)
{
    static char buf[520];
    size_t length;
    pkt_t *pkt;

    if (seqnum < 0)
        seqnum += 256;

    pkt = withdraw_pkt(buffer, seqnum);
    store_pkt(buffer, pkt);
    length = 520;

    if (!pkt) {
        if (locales.verbose)
            fprintf(stderr, "["KRED" error"KNRM"] pkt with seqnum %d doesn't "
                    "exist\n", seqnum);

        perror("withdraw_pkt");
        return 0;
    }

    if (pkt_encode(pkt, buf, &length) != PKT_OK) {
        perror("pkt_encode");
        return 1;
    }

    if (send(locales.sockfd, buf, length, 0) == -1) {
        perror("send");
        return 1;
    }

    return 0;
}

long get_elapsed_time(struct timeval *start, struct timeval *end)
{
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;

    long result = sec * 1000 + usec / 1000;

    return result;
}

#define LENGTH 4 + 512 + 4

int perform_transfer(void)
{
    pkt_t *pkt;
    fd_set rfds;
    char buffer[520];
    struct timeval tv;
    pkt_t *pkt_archives[32];
    int receiver_window, last_ack;
    int ofd, read_size, recv_size;

    ofd = (locales.filename ? open(locales.filename, O_RDONLY) : fileno(stdin));
    bzero(pkt_archives, 32 * sizeof(pkt_t *));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    tv = (struct timeval) {
        .tv_sec  = 0,
        .tv_usec = 500,
    };
    last_ack = 0;
    read_size = 1;
    receiver_window = 31;

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    for (;;)
    {
        if (locales.window && receiver_window && read_size)
        {
            read_size = read(ofd, buffer, 512);

            if (read_size < 0) {
                free_pkt_buffer(pkt_archives);
                perror("read");
                close(ofd);
                return 0;
            }
        }
        else if (!read_size && is_buffer_empty(pkt_archives))
        {
            break;
        }

        if (locales.window && receiver_window && read_size)
        {
            pkt = pkt_build(PTYPE_DATA, locales.window, locales.seqnum,
                            read_size, buffer);

            if (!pkt) {
                free_pkt_buffer(pkt_archives);
                perror("pkt_build");
                close(ofd);
                return 0;
            }
            gettimeofday(&pkt->tv, NULL);

            locales.seqnum = (locales.seqnum + 1) % 256;
            store_pkt(pkt_archives, pkt);

            if (send_pkt(pkt_archives, locales.seqnum - 1)) {
                free_pkt_buffer(pkt_archives);
                perror("send_pkt");
                close(ofd);
                return 0;
            }
        }

        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) == -1) {
            free_pkt_buffer(pkt_archives);
            perror("select");
            close(ofd);
            return 0;
        }

        if (FD_ISSET(locales.sockfd, &rfds)) {

            recv_size  = recv(locales.sockfd, buffer,
                              sizeof(buffer), MSG_DONTWAIT);

            if (recv_size < 0) {
                free_pkt_buffer(pkt_archives);
                perror("recv");
                close(ofd);
                return 0;
            }

            pkt = pkt_new();

            if (!pkt) {
                free_pkt_buffer(pkt_archives);
                perror("pkt_new");
                close(ofd);
                return 0;
            }

            if (pkt_decode(buffer, (size_t) recv_size, pkt) == PKT_OK)
            {
                receiver_window = pkt->window;

                switch (pkt->type)
                {
                    case PTYPE_ACK:
                        if (last_ack != pkt->seqnum)
                            last_ack = pkt->seqnum;
                        else
                            send_pkt(pkt_archives, pkt->seqnum);

                        drop_pkt(pkt_archives, pkt->seqnum - 1);
                        /*if (locales.verbose)
                            fprintf(stderr, "["KYEL" warn "KNRM"] received ACK "
                                    "for pkt %d\n", (int) pkt->seqnum);*/
                        break;

                    case PTYPE_NACK:
                        send_pkt(pkt_archives, pkt->seqnum);
                        if (locales.verbose)
                            fprintf(stderr, "["KYEL" warn "KNRM"] received NACK"
                                    " for pkt %d\n",
                                    (int) pkt->seqnum);
                        break;

                    default:
                        if (locales.verbose)
                            fprintf(stderr, "["KYEL" warn "KNRM"] receiver"
                                    "shouldn't send data\n");
                        break;
                }
            }

            pkt_del(pkt);
        }

        struct timeval current_time;
        gettimeofday(&current_time, NULL);

        for (int i = 0; i < 32; i++)
            if (pkt_archives[i] && get_elapsed_time(&pkt_archives[i]->tv, &current_time) > 500)
            {
                pkt_archives[i]->tv = current_time;
                send_pkt(pkt_archives, pkt_archives[i]->seqnum);
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
