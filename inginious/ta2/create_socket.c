#include "create_socket.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define handle(x)                                 \
    if (x == -1) {                                \
        fprintf(stderr, "%s\n", strerror(errno)); \
        return -1;                                \
    }
                    

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr,
                  int src_port,
                  struct sockaddr_in6 *dest_addr,
                  int dst_port)
{
    int status, sfd;
    socklen_t dest_len;

    source_addr->sin6_port = htons(src_port);
    dest_addr->sin6_port   = htons(dst_port);

    sfd = socket(AF_INET6, SOCK_STREAM, 0);
    handle(sfd);

    status = bind(sfd, (struct sockaddr *) source_addr,
             sizeof(source_addr));
    handle(status);

    status = listen(sfd, 50);
    handle(status);

    dest_len = sizeof(dest_addr);
    status = accept(sfd, (struct sockaddr *) dest_addr,
             &dest_len);
    handle(status);

    return sfd;
}

//int main(int argc, char **argv){argc--;argv++;return 0;}
