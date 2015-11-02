#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "locales.h"
#include "socket.h"
#include "packet.h" /* packet related functions and structures us*/

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */

/* This structure will store the shared parameters of all functions. Its
 * definition can be found in locales.h */
struct config locales = {
    .idef     = 1,
    .addr     = "::1",
    .port     = 64341,
    .filename = NULL,
    .verbose  = 0,
    .passive  = 0,
    .window   = 31,
    .seqnum   = 0,
    .pkt_cnt  = 0,
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

/*TO PRINT DATA*/

void print_warning(char * type, int value)
{
    if (locales.verbose)
        fprintf(stderr, "["KYEL" warn "KNRM"] %s\t%d\n", type, value);
}

/*FOR PKT_BUFFER USE*/

pkt_t *withdraw_pkt(pkt_t *buffer[32], int i)
{
    pkt_t *pkt;

    if (!buffer[i])
        return NULL;

    pkt = buffer[i];
    locales.window++;
    buffer[i] = NULL;

    return pkt;
}

int store_pkt(pkt_t *buffer[32], pkt_t *pkt)
{
    int diff;

    diff = pkt->seqnum - locales.seqnum;

    if (diff < 0 || diff >= 32 || buffer[diff])
    {
        pkt_del(pkt);

        if (buffer[diff])
            return 1;
        else
            return 0;
    }

    locales.window--;
    buffer[diff] = pkt;

    return 0;
}

void free_pkt_buffer(pkt_t *buffer[32])
{
    for (int i = 0; i < 32; i++)
        pkt_del(buffer[i]);

    bzero(buffer, 32 * sizeof(pkt_t *));
}

/*FOR TRANSMISSION*/

int write_in_seq_pkt(int fd, pkt_t *buffer[32])
{
    int i;
    pkt_t *pkt;

    for (i = 0; i < 32; i++) {

        pkt = withdraw_pkt(buffer, i);

        if (!pkt || pkt->seqnum - locales.seqnum) {
            pkt_del(pkt);
            return 0;
        }

        locales.seqnum = (locales.seqnum + 1) % 256;

        if (!pkt->length && !pkt->payload) {
            pkt_del(pkt);
            return 1;
        }

        if (write(fd, pkt->payload, pkt->length) == -1) {
            perror("write");
            pkt_del(pkt);
            return -1;
        }

        pkt_del(pkt);
    }

    if (locales.window < 31) {
        memcpy(buffer, &buffer[i], 32 - i);
        memset(&buffer[i], 0, i);
    }

    return 0;
}

int send_control_pkt(ptypes_t type, uint8_t seqnum)
{
    static char buf[8];
    size_t length;
    pkt_t *pkt;

    pkt = pkt_build(type, locales.window, seqnum, 0, NULL);

    if (!pkt)
        return 1;

    length = 8;

    pkt_encode(pkt, buf, &length);
    send(locales.sockfd, buf, length, 0);
    pkt_del(pkt);

    return 0;
}

int receive_data(void)
{
    pkt_t *pkt;
    fd_set rfds;
    char buf[520];
    ssize_t recv_size;
    struct timeval c_time;
    pkt_t *pkt_buffer[32];
    int ofd, write_status;

    bzero(pkt_buffer, 32 * sizeof(pkt_t *));
    ofd = (locales.filename ? open(locales.filename,
            O_WRONLY | O_CREAT | O_TRUNC, 0644) : fileno(stdout));

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    for (;;)
    {
        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        c_time = (struct timeval) {
            .tv_sec  = 1,
            .tv_usec = 0,
        };

        select(FD_SETSIZE, &rfds, NULL, NULL, &c_time);

        if (FD_ISSET(locales.sockfd, &rfds))
        {
            recv_size = recv(locales.sockfd, buf, 520, MSG_DONTWAIT);

            pkt = pkt_new();

            if (pkt_decode(buf, (size_t)recv_size, pkt) == PKT_OK)
            {
                print_warning("REC", pkt->seqnum);

                store_pkt(pkt_buffer, pkt);
                locales.pkt_cnt++;

                write_status = write_in_seq_pkt(ofd, pkt_buffer);

                if (write_status == -1) {
                    free_pkt_buffer(pkt_buffer);
                    return 0;
                }

                send_control_pkt(PTYPE_ACK, locales.seqnum);

                if (write_status)
                    break;
            }
            else
            {
                send_control_pkt(PTYPE_NACK, pkt->seqnum);
                print_warning("NACK", pkt->seqnum);
                pkt_del(pkt);
            }
        }
    }

    if (locales.verbose)
    {
        fprintf(stderr, "["KGRN"  ok  "KNRM"] Transfered\n");

        fprintf(stderr, "["KBLU" info "KNRM"] %d kB received\n",
                (locales.pkt_cnt * 512) / 1000);
    }

    if (locales.filename)
        close(ofd);

    if (shutdown(locales.sockfd, SHUT_RDWR) == -1) {
        perror("shutdown");
        return 0;
    }

    free_pkt_buffer(pkt_buffer);
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
        ok = receive_data();

    close(locales.sockfd);

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] All done\n");

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
