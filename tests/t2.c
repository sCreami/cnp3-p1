#include "t2.h"

int setup(void)
{
	return 0;
}

int teardown(void)
{
	return 0;
}

/*
 * Only the result of encode for valid input is tested here.
 */
void test_encode()
{
    pkt_t *pkt;
    uint32_t crc;
    size_t length;
    char * payload;
    char buffer[520];
    pkt_status_code status;

    length = 520;
    pkt = pkt_new();
    payload  = "12345";

    char expected_result[12] = {49, 42, 0, 5, 49, 50, 51, 52, 53, 0, 0, 0};
    uint32_t expected_crc = crc32(0, expected_result, 12);

    CU_ASSERT_NOT_EQUAL(pkt, NULL);

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_window(pkt, 17);
    pkt_set_seqnum(pkt, 42);
    pkt_set_payload(pkt, payload, strlen(payload));

    status = pkt_encode(pkt, buffer, &length);

    CU_ASSERT_EQUAL(status, PKT_OK);

    CU_ASSERT_EQUAL(length, 16);

    CU_ASSERT_EQUAL(0, strncmp(buffer, expected_result, 12));

    memcpy(&crc, buffer + 12, 4);
    crc = be32toh(crc);

    CU_ASSERT_EQUAL(expected_crc, crc);

    pkt_del(pkt);
}

/*
 * done : valid input
 * soon : bad crc, incoherent length, no payload, large window, wtf type
 */
void test_decode()
{
    pkt_t *pkt;
    size_t length;
    char * payload;
    pkt_status_code status;

    char input[16] = {49, 42, 0, 5, 49, 50, 51, 52, 53, 0, 0, 0, 0, 0, 0, 0};
    uint32_t crc = crc32(0, input, 12);
    crc = htobe32(crc);

    memcpy(input + 12, &crc, 4);

    length = 16;
    pkt = pkt_new();
    payload  = "12345\0\0\0";

    CU_ASSERT_NOT_EQUAL(pkt, NULL);

    status = pkt_decode(input, length, pkt);

    CU_ASSERT_EQUAL(status, PKT_OK);

    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
    CU_ASSERT_EQUAL(pkt_get_window(pkt), 17);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 42);
    CU_ASSERT_EQUAL(pkt_get_length(pkt), 5);Only the result of encode for valid input is tested here.
    CU_ASSERT_EQUAL(strncmp(pkt_get_payload(pkt), payload, 8), 0);

}

/*int main()
{
    int initialization_status = CU_initialize_registry();

    if (initialization_status != CUE_SUCCESS)
	   return CU_get_error();

    CU_pSuite pSuite = CU_add_suite("pkt tests", setup, teardown);

    if (!pSuite)
    {
        CU_cleanup_registry();
		return CU_get_error();
    }

    if
    (
        !CU_add_test(pSuite, "test encode", test_encode) ||
        !CU_add_test(pSuite, "test decode", test_decode)
    )
    {
        CU_cleanup_registry();
		return CU_get_error();
    }

    CU_basic_run_tests();
    CU_cleanup_registry();

    return EXIT_SUCCESS;
}*/
