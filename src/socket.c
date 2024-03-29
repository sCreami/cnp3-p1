/* socket.c */

#include "socket.h"

/* Retrieve the connectable address from a given string in parameter.
 * Can also be used to determine if a string is an address or not setting the
 * parameter flag to 1. */
int real_address(const char *address, struct sockaddr_in6 *rval, int flag)
{
    struct addrinfo hints, *res, *p;
    int status;

    bzero(&hints, sizeof(hints));

    hints = (struct addrinfo) {
        .ai_family    = AF_INET6,
        .ai_socktype  = SOCK_DGRAM,
        .ai_flags     = 0,
        .ai_protocol  = 0,
    };

    status = getaddrinfo(address, NULL, &hints, &res);

    // used by argpars to check
    // for address in args
    if (flag) {
        if (!status) freeaddrinfo(res);
        return !status;
    }

    if (status) {
        if (locales.verbose)
            fprintf(stderr, "%s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next)
        memcpy(rval, p->ai_addr, sizeof(*rval));

    freeaddrinfo(res);

    return 0;
}


/* Clean the socket buffer from the garbage sent by an unauthorized remote
 * host. */
int clean_buffer(int sockfd)
{
    int ret;
    char buffer[4096];

    // read it until it bleeds
    while ((ret = read(sockfd, buffer, sizeof(buffer))) > 0);

    return ret;
}


/* Get a socket, set a timeout of 30 seconds on this socket and if called by :
 *   sender   : connect the socket to the given address retrieved by
 *              real_address().
 *   receiver : bind the socket to a local interface and wait from an incoming
 *              connection to retrieve the address from the remote host and
 *              connect to it. If the receiver is not in a passive state, check
 *              if the incoming host is the one attended. */
int connect_socket(void)
{
    char dull;
    int sockfd, match;
    struct timeval tv;
    socklen_t addr_len, sin6_len;
    struct sockaddr_in6 addr, rmot;

    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

    // stored in locales for facility
    locales.sockfd = sockfd;

    if (sockfd == -1) {
        perror("socket");
        return 0;
    }

    tv = (struct timeval) {
        .tv_sec  = 30,
        .tv_usec = 0,
    };

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (void *) &tv, sizeof(tv)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return 0;
    }

    // ensure cleanliness
    bzero(&addr, sizeof(addr));

    if (real_address(locales.addr, &addr, 0)) {
        perror("real_address");
        close(sockfd);
        return 0;
    }

    addr.sin6_port = htons(locales.port);

    if (locales.idef) {
        
        // remember remote hosts
        memcpy(&rmot, &addr, sizeof(addr));

        // listen to all interfaces
        addr.sin6_addr = in6addr_any;

        if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            perror("bind");
            close(sockfd);
            return 0;
        }

        addr_len = sizeof(addr);
        sin6_len = sizeof(addr.sin6_addr);

        if (locales.verbose)
            fprintf(stderr, "["KBLU" info "KNRM"] Waiting for sender\n");

        do {
            if (recvfrom(sockfd, &dull, 1, MSG_PEEK,
                         (struct sockaddr *) &addr, &addr_len) == -1) {
                perror("recvfrom");
                close(sockfd);
                return 0;
            }

            if (locales.passive) {
                match = 1;
            } else {
                match = !memcmp(&rmot.sin6_addr, &addr.sin6_addr, sizeof(sin6_len));

                if (!match && !clean_buffer(sockfd))
                    perror("clean_buffer");
            }
        } while (!match);
    }

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, "["KGRN"  ok  "KNRM"] Connected\n");

    return sockfd;
}
