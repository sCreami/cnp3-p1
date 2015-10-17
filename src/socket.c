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

int clean_buffer(int sockfd)
{
    int ret;
    char buffer[4096];

    // read it until it bleeds
    while ((ret = read(sockfd, buffer, sizeof(buffer))) > 0);

    return ret;
}

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

    // ensure cleaniness
    bzero(&addr, sizeof(addr));

    if (real_address(locales.addr, &addr)) {
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
            fprintf(stderr, KRED"[socket]"KNRM" Waiting for sender\n");

        match = 0;
        while (!match)
        {
            if (recvfrom(sockfd, &dull, 1, MSG_PEEK,
                         (struct sockaddr *) &addr, &addr_len) == -1) {
                perror("recvfrom");
                close(sockfd);
                return 0;
            }

            match = !memcmp(&rmot.sin6_addr, &addr.sin6_addr, sizeof(sin6_len));

            if (!match && !clean_buffer(sockfd))
                perror("clean_buffer");
        }
    }

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        return 0;
    }

    if (locales.verbose)
        fprintf(stderr, KRED"[socket]"KNRM" Connected\n");

    return sockfd;
}
