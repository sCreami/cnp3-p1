#include "t1.h"

void test_real_address(void)
{
    int status;
    struct sockaddr_in6 rval;
    struct in6_addr IPv6;

    memset(&rval, 0, sizeof(rval));
    memset(&IPv6, 0, sizeof(IPv6));

    char loopback[] = "::1";

    if(inet_pton(AF_INET6, loopback, &IPv6) == 0)
        CU_FAIL("inet_pton");

    status = real_address("localhost", &rval); // 0 on success
    CU_ASSERT(!status);

    CU_ASSERT(!memcmp(&rval.sin6_addr, &IPv6, sizeof(IPv6)));
}

void test_connect_socket(void)
{
    int sockfd;
    int wsize, rsize;
    char helloing[] = "Hello World!";
    char worlding[13];

    // connect_socket() nécessite le contenu
    // de locales pour fonctionner
    // NB : test comme sender
    sockfd = connect_socket();

    CU_ASSERT(sockfd > 0);

    /* Testing socket readability-writability */
    wsize = write(sockfd, helloing, sizeof(helloing));
    CU_ASSERT(wsize > 0);

    rsize = read(sockfd, worlding, wsize);
    CU_ASSERT(rsize = wsize);

    close(sockfd);
}
