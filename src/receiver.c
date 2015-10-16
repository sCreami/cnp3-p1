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
           KCYN "Passive "KNRM": %d\n"
           KCYN "--------------    \n" KNRM,
           locales.addr, locales.port,
           (locales.filename ? locales.filename : "stdout"),
           locales.passive);
}

/*int process_received_data(char * buffer, size_t length, int expected_seqnum)
{
    pkt_t *pkt = pkt_new();
    int decode_status = pkt_decode(buffer, length, pkt);

    if (decode_status != PKT_OK)
    {
        //send ack nack
    }
    else
    {
        if (expected_seqnum < pkt_get_seqnum);
    }

    return expected_seqnum;
}*/

/*void receive_loop(int in_fd, int out_fd)
{
    pkt_t pkt_buffer[32];
    uint32_t pkt_buffer_status = 0;
    int expected_seqnum;

    size_t initial_length = 4 + 512 + 4;
    sizet_t current_length = initial_length;
    char * data_buffer[length]

    while (1)
    {
        pkt_t * = pkt_new();


        pkt_decode()
    }
}*/

int main(int argc, char **argv)
{
    int ok = 1; // starting okay

    arguments_parser(argc, argv);

    if (locales.verbose)
        print_locales();

    ok = connect_socket();

    if (!ok)
        return EXIT_FAILURE;

    locales.in_fd = locales.sockfd;

    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
