/* socket.c */

#include "socket.h"

int real_address(const char *address, struct sockaddr_in6 *rval)
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

    if (status) {
        if (locales.verbose)
            fprintf(stderr, "%s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next)
        memcpy(rval, p->ai_addr, sizeof(*rval));

    return 0;
}

int connect_socket(void)
{
    int sockfd;
    struct timeval tv;
    socklen_t addr_len;
    struct sockaddr_in6 addr;

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

    // ensure cleaniness
    bzero(&addr, sizeof(addr));

    if (real_address(locales.addr, &addr)) {
        perror("real_address");
        close(sockfd);
        return 0;
    }

    addr.sin6_port = htons(locales.port);

    if (locales.passive) {
        if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            perror("bind");
            close(sockfd);
            return 0;
        }

        addr_len = sizeof(addr);

        // filling the same structure as bind may leads
        // to bugs. I'm not sure, let's discover !
        if (recvfrom(sockfd, NULL, 1, MSG_PEEK,
                     (struct sockaddr *) &addr, &addr_len) == -1) {
            perror("recvfrom");
            close(sockfd);
            return 0;
        }
    }

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        return 0;
    }

    return sockfd;
}
