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

#define WINDOW_SIZE   32
#define MAX_DELAY     50000
#define PKT_BUF_SIZE  520
#define READ_BUF_SIZE 512
#define SEQNUM_AMOUNT 256

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
    KCYN "--------------    \n" KNRM,
    locales.addr, locales.port, (locales.filename ? locales.filename:"stdin"));
}

/* Prints an event's  code and an associated value, such as 'NACK   17'*/
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

/* Prints the seqnums of the content of the pkt_buffer */
void print_pkt_buffer(pkt_t *buffer[WINDOW_SIZE])
{
    if (!locales.verbose)
        return;

    for (int i = 0; i < WINDOW_SIZE; i++)
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

/* Updates timeval t to current time, returns the
 * time elapsed between now and the content of t */
long update_timeval(struct timeval *t)
{
    long sec, usec;
    struct timeval current;

    gettimeofday(&current, NULL);

    sec  = current.tv_sec  - t->tv_sec;
    usec = current.tv_usec - t->tv_usec;

    memcpy(t, &current, sizeof(struct timeval));

    return sec * 1000 * 1000 + usec;
}

/* Stores a packet into the buffer, thus reducing locales.window of one unity */
int store_pkt(pkt_t *buffer[WINDOW_SIZE], pkt_t *pkt)
{
    for (int i = 0; i < WINDOW_SIZE; i++)
        if (!buffer[i]) {
            locales.window--;
            buffer[i] = pkt;
            return 0;
        }

    return 1;
}

/* Returns the packet with indicated seqnum if there is such packet, returns
 * NULL otherwise */
pkt_t *peek_pkt(pkt_t *buffer[WINDOW_SIZE], int seqnum)
{
    clean_seqnum(&seqnum);

    for (int i = 0; i < WINDOW_SIZE; i++)
        if (buffer[i] && buffer[i]->seqnum == seqnum)
            return buffer[i];

    return NULL;
}

/* Removes a packet from the buffer and frees the memory allocated to it */
int remove_pkt(pkt_t *buffer[WINDOW_SIZE], int seqnum)
{
    clean_seqnum(&seqnum);

    for (int i = 0; i < WINDOW_SIZE; i++)
        if (buffer[i] && buffer[i]->seqnum == seqnum) {
            locales.window++;
            pkt_del(buffer[i]);
            buffer[i] = NULL;
            return 1;
        }

    return 0;
}

/* Removes packets preceding and equals to the indicated seqnum */
void drop_pkt(pkt_t *buffer[WINDOW_SIZE], int seqnum)
{
    if (remove_pkt(buffer, seqnum))
        drop_pkt(buffer, seqnum - 1);
}

/* Frees the content of the buffer, mostly useful if the transfer has to be
 * interrupted */
void free_pkt_buffer(pkt_t *buffer[WINDOW_SIZE])
{
    for (int i = 0; i < WINDOW_SIZE; i++)
        pkt_del(buffer[i]);

    bzero(buffer, WINDOW_SIZE * sizeof(pkt_t *));
}

/* Returns 1 if the buffer is empty, 0 otherwise */
int is_buffer_empty(pkt_t *buffer[WINDOW_SIZE])
{
    for (int i = 0; i < WINDOW_SIZE; i++)
        if (buffer[i])
            return 0;

    return 1;
}

/* Sends a packet of the buffer on the socket */
int send_pkt(pkt_t *buffer[WINDOW_SIZE], int seqnum)
{
    pkt_t *pkt;
    size_t length;
    static char buf[PKT_BUF_SIZE];

    clean_seqnum(&seqnum);

    // ask for a clean packet
    pkt = peek_pkt(buffer, seqnum);
    length = PKT_BUF_SIZE;

    if (!pkt) {
        print_pkt_buffer(buffer);
        print_warning("NSP", seqnum);
        return 0;
    }

    // encoding packet
    if (pkt_encode(pkt, buf, &length) != PKT_OK) {
        perror("pkt_encode");
        return 1;
    }

    // sending packet 
    if (send(locales.sockfd, buf, length, 0) == -1) {
        perror("send");
        return 1;
    }

    return 0;
}

/* Receives a packet ACK / NACK sent by the receiver */
int receive_pkt(pkt_t *buffer[WINDOW_SIZE],
                struct timeval *t,
                uint8_t *last_ack)
{
    pkt_t *pkt;
    static char buf[PKT_BUF_SIZE];

    if (recv(locales.sockfd, buf, 8, MSG_DONTWAIT) == -1) {
        perror("recv");
        return 1;
    }

    pkt = pkt_new();

    if (pkt_decode(buf, 8, pkt) != PKT_OK) {
        // if the packet is unconsistent
        print_warning("CORR", -1);
    }

    else {
        update_timeval(t);

        switch (pkt->type)
        {
            case PTYPE_ACK :
                print_warning("ACK", pkt->seqnum);
                drop_pkt(buffer, pkt->seqnum - 1);
                *last_ack = pkt->seqnum;
                break;

            case PTYPE_NACK :
                print_warning("NACK", pkt->seqnum);
                locales.pkt_cnt++;
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

/* Main loop, handles the transfer */
int perform_transfer(void)
{
    int ofd;
    pkt_t *pkt;
    fd_set rfds;
    double delay;
    uint8_t last_ack;
    ssize_t read_size;
    char buf[READ_BUF_SIZE];
    pkt_t *pkt_buffer[WINDOW_SIZE];
    struct timeval s_time, d_time;

    ofd = (locales.filename ? open(locales.filename, O_RDONLY) : fileno(stdin));
    
    // init
    read_size = 1;
    last_ack  = 0;
    delay     = 0;
    bzero(pkt_buffer, WINDOW_SIZE * sizeof(pkt));
    update_timeval(&s_time);

    if (ofd == -1) {
        perror("open");
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KBLU" info "KNRM"] Starting transfer\n");

    for (;;)
    {
        // reading data on stdin / input file if there is data and that there's
        // enough room to store them then sends the data

        if (locales.window && read_size)
        {
            read_size = read(ofd, buf, READ_BUF_SIZE);

            if (read_size == -1) {
                free_pkt_buffer(pkt_buffer);
                perror("read");
                close(ofd);
                return 0;
            }

            pkt = pkt_build(PTYPE_DATA, locales.window, locales.seqnum,
                            (size_t)read_size, buf);

            locales.pkt_cnt++;
            store_pkt(pkt_buffer, pkt);

            if (send_pkt(pkt_buffer, locales.seqnum)) {
                free_pkt_buffer(pkt_buffer);
                close(ofd);
                return 0;
            }

            locales.seqnum = (locales.seqnum + 1) % SEQNUM_AMOUNT;
        }

        // only if the transfer is completed, stop the loop
        if (!read_size && is_buffer_empty(pkt_buffer))
            break;

        // updating delay
        if (delay)
            delay = 0.5 * delay + 0.5 * update_timeval(&s_time);
        else
            delay = update_timeval(&s_time);

        d_time = (struct timeval) {
            .tv_sec  = 0,
            .tv_usec = (long) delay
        };

        FD_ZERO(&rfds);
        FD_SET(locales.sockfd, &rfds);

        // seek data on socket
        select(FD_SETSIZE, &rfds, NULL, NULL, &d_time);

        // there is data
        if (FD_ISSET(locales.sockfd, &rfds)) {
            if (receive_pkt(pkt_buffer, &s_time, &last_ack)) {
                free_pkt_buffer(pkt_buffer);
                close(ofd);
                return 0;
            }
        }

        // no data, unexpected, sending data again
        else {
            if (delay > MAX_DELAY) {
                free_pkt_buffer(pkt_buffer);
                break;
            }

            locales.pkt_cnt++;
            send_pkt(pkt_buffer, last_ack);
            print_warning("TIME", (int)delay);
        }
    }

    // reporting perfomances, closing file and
    // shutdowning I/O operation on the socket

    if (locales.verbose && delay > MAX_DELAY) {
        fprintf(stderr, "["KRED"  ko  "KNRM"] Interrupted\n");
    }

    else if (locales.verbose) {
        fprintf(stderr,
        "["KGRN"  ok  "KNRM"] Transfered\n"
        "["KBLU" info "KNRM"] %d packets sent (around %d kB)\n",
        locales.pkt_cnt, (locales.pkt_cnt * 512) / 1000);
    }

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

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
