#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "locales.h"
#include "socket.h"
#include "packet.h" /* packet related functions and structures */

#include "argpars.c" /* void arguments_parser(int argc, char **argv) */

#define WINDOW_SIZE   32
#define PKT_BUF_SIZE  520
#define SEQNUM_AMOUNT 256

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
static inline void print_locales(void)
{
    fprintf(stderr,
    KCYN "---- args ----    \n"
    KCYN "Address "KNRM": %s\n"
    KCYN "Port    "KNRM": %d\n"
    KCYN "File    "KNRM": %s\n"
    KCYN "Passive "KNRM": %d\n"
    KCYN "--------------    \n" KNRM,
    locales.addr, locales.port, (locales.filename ? locales.filename:"stdout"),
    locales.passive);
}

/* Prints an event's code and an associated value, such as 'NACK 17' */
static inline void print_warning(char * type, int value)
{
    if (locales.verbose)
        fprintf(stderr, "["KYEL" warn "KNRM"] %s\t%d\n", type, value);
}

/* Operations on seqnum can turn them into negative integers, this function does
 * fix that. */
static inline void clean_seqnum(int *seqnum)
{
    if (*seqnum < 0)
        *seqnum += SEQNUM_AMOUNT;
}

/* Extracts the packet with the indicated seqnum from the buffer. */
pkt_t *withdraw_pkt(pkt_t *buffer[WINDOW_SIZE], int seqnum)
{
    pkt_t *pkt;

    clean_seqnum(&seqnum);

    for (int i = 0; i < WINDOW_SIZE; i++)
        if (buffer[i] && buffer[i]->seqnum == seqnum) {
            locales.window++;
            pkt = buffer [i];
            buffer[i] = NULL;
            return pkt;
        }

    return NULL;
}

/* Stores a packet into the buffer, if there's room for it and if it's
 * appropriate to do so */
int store_pkt(pkt_t *buffer[WINDOW_SIZE], pkt_t *pkt)
{
    int diff;

    diff = pkt->seqnum - locales.seqnum;

    // is it inappropriate to store the packet ?
    if (diff > WINDOW_SIZE || diff < 0)
    {
        pkt_del(pkt);
        return 0;
    }

    // is the packet already in the buffer ?
    for (int i = 0; i < WINDOW_SIZE; i++)
        if (buffer[i] && buffer[i]->seqnum == pkt->seqnum) {
            pkt_del(pkt);
            return 0;
        }

    // thus store it
    for (int i = 0; i < WINDOW_SIZE; i++)
        if (!buffer[i]) {
            locales.window--;
            buffer[i] = pkt;
            return 0;
        }

    return 1;
}

/* Frees the content of the buffer, mostly useful if the transfer has to be
 * interrupted */
void free_pkt_buffer(pkt_t *buffer[WINDOW_SIZE])
{
    for (int i = 0; i < WINDOW_SIZE; i++)
        pkt_del(buffer[i]);

    bzero(buffer, WINDOW_SIZE * sizeof(pkt_t *));
}

/* Writes all packets that are in sequence and stored into the buffer on stdout
 * or the output file. ex: packet 123 to 127 if packet 128 is missing*/
int write_in_seq_pkt(int fd, pkt_t *buffer[WINDOW_SIZE])
{
    int i;
    pkt_t *pkt;

    for (i = 0; i < WINDOW_SIZE; i++)
    {
        pkt = withdraw_pkt(buffer, locales.seqnum);

        // if the withdrawed packet from buffer is invalid
        if (!pkt || pkt->seqnum - locales.seqnum) {
            pkt_del(pkt);
            return 0;
        }

        locales.seqnum = (locales.seqnum + 1) % SEQNUM_AMOUNT;

        // if it's the closing connection packet
        // then delete it and return
        if (!pkt->length && !pkt->payload) {
            pkt_del(pkt);
            return 1;
        }

        // else append the packet content to output
        if (write(fd, pkt->payload, pkt->length) == -1) {
            perror("write");
            pkt_del(pkt);
            return -1;
        }

        pkt_del(pkt);
    }

    if (locales.window < WINDOW_SIZE - 1) {
        memcpy(buffer, &buffer[i], WINDOW_SIZE - i);
        bzero(&buffer[i], i);
    }

    return 0;
}

/* Sends an ACK or NACK to the remote host */
int send_control_pkt(ptypes_t type, uint8_t seqnum)
{
    pkt_t *pkt;
    size_t length;
    static char buf[8];

    pkt = pkt_build(type, locales.window, seqnum, 0, NULL);
    if (!pkt) return 1;

    length = 8;

    pkt_encode(pkt, buf, &length);
    send(locales.sockfd, buf, length, 0);
    pkt_del(pkt);

    return 0;
}

/* Main loop's function, handles the transfer */
int receive_data(void)
{
    pkt_t *pkt;
    fd_set rfds;
    ssize_t recv_size;
    int ofd, write_status;
    struct timeval s_time;
    char buf[PKT_BUF_SIZE];
    pkt_t *pkt_buffer[WINDOW_SIZE];

    bzero(pkt_buffer, WINDOW_SIZE * sizeof(pkt));
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
        // seek data on socket
        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        s_time = (struct timeval) {
            .tv_sec  = 1,
            .tv_usec = 0,
        };

        select(FD_SETSIZE, &rfds, NULL, NULL, &s_time);

        if (FD_ISSET(locales.sockfd, &rfds))
        {
            recv_size = recv(locales.sockfd, buf, PKT_BUF_SIZE, MSG_DONTWAIT);

            pkt = pkt_new();
            locales.pkt_cnt++;

            // decoding, storing in a buffer and
            // writing data if the packet is valid
            if (pkt_decode(buf, (size_t) recv_size, pkt) == PKT_OK) {

                print_warning("RECV", pkt->seqnum);

                store_pkt(pkt_buffer, pkt);

                write_status = write_in_seq_pkt(ofd, pkt_buffer);

                if (write_status == -1) {
                    free_pkt_buffer(pkt_buffer);
                    return 0;
                }

                // send ACK
                send_control_pkt(PTYPE_ACK, locales.seqnum);

                if (write_status)
                    break;
            }

            // else send NACK to remote host
            // if the packet in inconsistent
            else {
                send_control_pkt(PTYPE_NACK, pkt->seqnum);
                print_warning("NACK", pkt->seqnum);
                pkt_del(pkt);
            }
        }
    }
    
    // reporting perfomances, closing file and
    // shutdowning I/O operation on the socket

    if (locales.verbose) {
        fprintf(stderr,
        "["KGRN"  ok  "KNRM"] Transfered\n"
        "["KBLU" info "KNRM"] %d packets received (around %d kB)\n",
        locales.pkt_cnt, (locales.pkt_cnt * 512) / 1000);
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

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
