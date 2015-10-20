#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../src/locales.h"

#include "t1.h"
#include "t2.h"

struct config locales = {
    .idef     = 0,
    .addr     = "::1",
    .port     = 8080,
    .filename = NULL,
    .verbose  = 0,
    .window   = 31,
    .seqnum   = 0,
};

int main(void)
{
    CU_pSuite sockSuite;
    CU_pSuite paktSuite;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add suites to the registry */
    sockSuite = CU_add_suite("socket_suite", NULL, NULL);
    paktSuite = CU_add_suite("packet_suite", NULL, NULL);

    if (!sockSuite || !paktSuite) {
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

    if (0) // add T2 tests
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
