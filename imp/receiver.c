#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <zlib.h>

#include <netdb.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "socket/real_address.h"
#include "socket/create_socket.h"

#include "packet/packet_interface.h"

#define NUL 0b00000000
#define DLE 0b00100000
#define STX 0b00000010
#define ETX 0b00000011

in_port_t get_port(char * s)
{
    in_port_t port = 0;

    while (s[0] != '\0')
    {
        char c[2] = {s[0], '\0'};

        if (atoi(c))
            port = port * 10 + atoi(c);
        else
            return 0;

        s++;
    }

    return port;
}

char * find_next_pkt(char * buf, size_t len)
{
    int i;
    for (i = 0; i < len - 1; i++)
        if (buf[i] == DLE && buf[i + 1] == STX)
            return buf + i;
    return NULL;
}

bool read_data(int fd, char * buf, size_t * len)
{
    int read_status = read(fd, buf, *len);

    if (read_status < 0)
        fprintf(stderr, ">> met error while reading data\n");
    else
        *len = read_status;

    return read_status > 0;
}

int main(int argc, char * argv[])
{
    char * address = "::";
    in_port_t port = 24601;
    char * filename = NULL;

    int i;
    for (i = 0; i < argc - 1; i++)
        if (!strcmp("--filename", argv[i]) || !strcmp("-f", argv[i]))
            filename = argv[++i];
        else if (get_port(argv[i]))
            port = get_port(argv[i]);
        else
            address = argv[i];

    fprintf(stderr, ">> %s ; %d ; %s\n", address, port, filename);

    struct sockaddr_in6 addr;

    const char * addr_error = real_address(address, &addr);
    if (addr_error)
    {
        fprintf(stderr, ">> %s\n", addr_error);
        return EXIT_FAILURE;
    }

    int socket_fd = create_socket(NULL, -1, &addr, port);

    if (socket_fd == -1)
    {
        fprintf(stderr, ">> couldn't create socket\n");
        return EXIT_FAILURE;
    }

    int out_fd;
    if (filename)
        out_fd = open(filename, O_WRONLY, S_IWRITE);
    else
        out_fd = fileno(stdin);

    fd_set fds;
    struct timeval tv =
    {
        .tv_sec  = 5,
        .tv_usec = 0,
    };

    while (true)
    {
        FD_ZERO(&fds);
        FD_SET(socket_fd, &fds);

        if (select(FD_SETSIZE, &fds, NULL, NULL, &tv) > 0)
        {
            fprintf(stderr, ">> met error executing 'select'\n");
            return EXIT_FAILURE;
        }

        if (FD_ISSET(socket_fd, &fds))
        {
            //get and write incoming data
        }
    }

    return EXIT_SUCCESS;
}
