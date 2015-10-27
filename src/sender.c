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
#include "packet.h" /* packet related functions and structures */

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */

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

/*TO PRINT DATA*/

void print_warning(char * type, int value)
{
    if (locales.verbose)
        fprintf(stderr, "["KYEL" warn "KNRM"] %s\t%d\n", type, value);
}

void print_pkt_buffer(pkt_t *buffer[32])
{
    int i;

    if (!locales.verbose)
        return;

    for (i = 0; i < 32; i++)
    {
        if (i != 0 && !(i % 8))
            fprintf(stderr, "\n");

        if (buffer[i])
            fprintf(stderr, "%d\t", buffer[i]->seqnum);
        else
            fprintf(stderr, ".\t");
    }

    printf("\n");
}

/*UTIL*/

void clean_seqnum(int *seqnum)
{
    if (*seqnum < 0)
        *seqnum += 256;
}

long update_timeval(struct timeval *time)
{
    struct timeval current;
    long sec, usec;

    gettimeofday(&current, NULL);

    sec  = current.tv_sec  - time->tv_sec;
    usec = current.tv_usec - time->tv_usec;

    memcpy(time, &current, sizeof(struct timeval));

    return sec * 1000 * 1000 + usec;
}

/*FOR PKT_BUFFER USE*/

int store_pkt(pkt_t *buffer[32], pkt_t *pkt)
{
    for (int i = 0; i < 32; i++)
        if (!buffer[i]) {
            locales.window--;
            buffer[i] = pkt;
            return 0;
        }

    return 1;
}

pkt_t *peek_pkt(pkt_t *buffer[32], int seqnum)
{
    clean_seqnum(&seqnum);

    for (int i = 0; i < 32; i++)
        if (buffer[i] && buffer[i]->seqnum == seqnum)
            return buffer[i];

    return NULL;
}

int remove_pkt(pkt_t *buffer[32], int seqnum)
{
    clean_seqnum(&seqnum);

    for (int i = 0; i < 32; i++)
        if (buffer[i] && buffer[i]->seqnum == seqnum) {
            locales.window++;
            pkt_del(buffer[i]);
            buffer[i] = NULL;
            return 1;
        }

    return 0;
}

void drop_pkt(pkt_t *buffer[32], int seqnum)
{
    if (remove_pkt(buffer, seqnum))
        drop_pkt(buffer, seqnum - 1);
}

void free_pkt_buffer(pkt_t *buffer[32])
{
    for (int i = 0; i < 32; i++)
        pkt_del(buffer[i]);

    bzero(buffer, 32 * sizeof(pkt_t *));
}

int is_buffer_empty(pkt_t *buffer[32])
{
    for (int i = 0; i < 32; i++)
        if (buffer[i])
            return 0;

    return 1;
}

/*FOR TRANSMISSION*/

int send_pkt(pkt_t *buffer[32], int seqnum)
{
    static char buf[520];
    size_t length;
    pkt_t *pkt;

    clean_seqnum(&seqnum);

    pkt = peek_pkt(buffer, seqnum);
    length = 520;

    if (!pkt) {
        print_pkt_buffer(buffer);
        print_warning("NSP", seqnum);
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

int receive_pkt(pkt_t *buffer[32], struct timeval *time, uint8_t *last_ack)
{
    static char buf[520];
    pkt_t *pkt;

    if (recv(locales.sockfd, buf, 8, MSG_DONTWAIT) == -1) {
        perror("recv");
        return 1;
    }

    pkt = pkt_new();

    if (pkt_decode(buf, 8, pkt) != PKT_OK)
    {
        print_warning("CORR", -1);
    }
    else
    {
        update_timeval(time);

        switch (pkt->type)
        {
            case PTYPE_ACK :
                print_warning("ACK", pkt->seqnum);
                drop_pkt(buffer, pkt->seqnum - 1);
                *last_ack = pkt->seqnum;
                break;

            case PTYPE_NACK :
                print_warning("NACK", pkt->seqnum);
                send_pkt(buffer, pkt->seqnum);
                break;

            default :
                print_warning("INV", pkt->seqnum);
                break;
        }
    }

    pkt_del(pkt);

    return 0;
}

int perform_transfer(void)
{
    int ofd;
    pkt_t *pkt;
    fd_set rfds;
    double delay;
    char buf[512];
    uint8_t last_ack;
    ssize_t read_size;
    pkt_t *pkt_buffer[32];
    struct timeval time, d_time;

    ofd = (locales.filename ? open(locales.filename, O_RDONLY) : fileno(stdin));
    bzero(pkt_buffer, 32 * sizeof(pkt_t *));
    update_timeval(&time);
    read_size = 1;
    last_ack = 0;
    delay = 0;

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    for (;;)
    {
        if (locales.window && read_size)
        {
            read_size = read(ofd, buf, 512);

            if (read_size == -1) {
                free_pkt_buffer(pkt_buffer);
                perror("read");
                close(ofd);
                return 0;
            }

            pkt = pkt_build(PTYPE_DATA, locales.window, locales.seqnum,
                            (size_t)read_size, buf);

            store_pkt(pkt_buffer, pkt);

            if (send_pkt(pkt_buffer, locales.seqnum)) {
                free_pkt_buffer(pkt_buffer);
                close(ofd);
                return 0;
            }

            locales.seqnum = (locales.seqnum + 1) % 256;
        }

        if (!read_size && is_buffer_empty(pkt_buffer))
            break;

        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        if (delay)
            delay = 0.5 * delay + 0.5 * update_timeval(&time);
        else
            delay = update_timeval(&time);

        d_time = (struct timeval) {
            .tv_sec  = 0,
            .tv_usec = (long) delay
        };

        select(FD_SETSIZE, &rfds, NULL, NULL, &d_time);

        if (FD_ISSET(locales.sockfd, &rfds))
        {
            if (receive_pkt(pkt_buffer, &time, &last_ack)) {
                free_pkt_buffer(pkt_buffer);
                close(ofd);
                return 0;
            }
        }
        else
        {
            send_pkt(pkt_buffer, last_ack);
            print_warning("TIME", (int)delay);
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
