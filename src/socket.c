#include "socket.h"

int real_address(const char *address, struct sockaddr_in6 *rval)
{
    struct addrinfo hints, *res, *p;
    int status;

    bzero(&hints, sizeof(struct addrinfo));

    hints = (struct addrinfo) {
        .ai_family    = AF_INET6,
        .ai_socktype  = SOCK_DGRAM,
        .ai_flags     = 0,
        .ai_protocol  = 0,
    };

    status = getaddrinfo(address, NULL, &hints, &res);

    if (status) {
        perror(gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next)
        memcpy(rval, p->ai_addr, sizeof(struct sockaddr_in6));

    return 0;
}

int connect_socket(void)
{
    int sockfd;
    struct timeval tv;
    struct sockaddr_in6 addr;

    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

    // for debug purpose
    locales.sockfd = sockfd;

    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (void *) &tv, sizeof(struct timeval)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    tv = (struct timeval) {
        .tv_sec  = 2,
        .tv_usec = 0,
    };

    // ensure cleaniness
    bzero(&addr, sizeof(struct sockaddr_in6));

    if (real_address(locales.addr, &addr)) {
        perror("real_address");
        close(sockfd);
        return -1;
    }

    addr.sin6_port = htons(locales.port);

    if (connect(sockfd, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_in6)) == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
