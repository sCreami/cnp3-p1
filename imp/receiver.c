#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <netdb.h>

#include "socket/real_address.h"
#include "socket/create_socket.h"

#include "packet/packet_interface.h"

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

    char * addr_error = real_address(address, &addr);
    if (addr_error)
    {
        fprintf(stderr, ">> %s\n", addr_error);
        return EXIT_FAILURE;
    }

    socket_fd = create_socket(null, -1, addr, port);

    if (socket_fd == -1)
    {
        fprintf(stderr, ">> couldn't create socket\n");
        return EXIT_FAILURE;
    }

    while (true)
    {
        /*let's just ...*/
        return 1;
        /*for now ...*/
    }

    return EXIT_SUCCESS;
}
