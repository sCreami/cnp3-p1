#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../src/socket.h"

struct config locales = {
    .idef     = 0,
    .addr     = "::1",
    .port     = 8080,
    .filename = NULL,
    .verbose  = 0,
    .window   = 31,
    .seqnum   = 0,
};

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

int main(void)
{
    CU_pSuite sockSuite;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add suites to the registry */
    sockSuite = CU_add_suite("socket_suite", NULL, NULL);

    if (!sockSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if (!(CU_add_test(sockSuite, "test_real_address", test_real_address)) ||
        !(CU_add_test(sockSuite, "test_connect_socket", test_connect_socket)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
