#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../src/receiver.h"
#include "../src/sender.h"

int main(void)
{
    CU_pSuite rSuite = NULL,
              sSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add suites to the registry */
    rSuite = CU_add_suite("receiver_suite", NULL, NULL);
    sSuite = CU_add_suite("sender_suite"  , NULL, NULL);

    if (!rSuite || !sSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }


    /* add the tests to the suite */
    if ((NULL == CU_add_test(rSuite, "test_a", test_a)) ||
        (NULL == CU_add_test(rSuite, "test_b", test_b)) ||
        (NULL == CU_add_test(rSuite, "test_c", test_c))
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
