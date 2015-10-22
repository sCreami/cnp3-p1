#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../src/packet.h"

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
 * valid input, bad crc, incoherent length, cut, wrong type
 */
void test_decode()
{
    pkt_t *pkt;
    size_t length;
    char * payload;
    pkt_status_code status;

    /*building encoded packet*/

    char input[16] = {49, 42, 0, 5, 49, 50, 51, 52, 53, 0, 0, 0, 0, 0, 0, 0};
    uint32_t crc = crc32(0, input, 12);
    crc = htobe32(crc);

    memcpy(input + 12, &crc, 4);

    /*building ressources*/

    length = 16;
    pkt = pkt_new();
    payload  = "12345\0\0\0";

    CU_ASSERT_NOT_EQUAL(pkt, NULL);

    /*testing result for valid input*/

    status = pkt_decode(input, length, pkt);

    CU_ASSERT_EQUAL(status, PKT_OK);

    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
    CU_ASSERT_EQUAL(pkt_get_window(pkt), 17);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 42);
    CU_ASSERT_EQUAL(pkt_get_length(pkt), 5);
    CU_ASSERT_EQUAL(strncmp(pkt_get_payload(pkt), payload, 8), 0);

    /*testing result for invalid crc*/

    input[14]++;
    crc = crc32(0, input, 12);
    crc = htobe32(crc);
    status = pkt_decode(input, length, pkt);

    CU_ASSERT_NOT_EQUAL(status, PKT_OK);

    /*testing result for incoherent length*/

    input[14]--;
    input[3] = 0b0011;
    crc = crc32(0, input, 12);
    crc = htobe32(crc);
    status = pkt_decode(input, length, pkt);

    CU_ASSERT_NOT_EQUAL(status, PKT_OK);

    /*testing result for cut pkt (no payload nor crc)*/

    input[3] = 0b0101;
    crc = crc32(0, input, 12);
    crc = htobe32(crc);
    status = pkt_decode(input, 4, pkt);

    CU_ASSERT_NOT_EQUAL(status, PKT_OK);

    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
    CU_ASSERT_EQUAL(pkt_get_window(pkt), 17);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 42);
    CU_ASSERT_EQUAL(pkt_get_length(pkt), 5);

    /*testing for invalid type*/

    input[0] = 0b11110001;
    crc = crc32(0, input, 12);
    crc = htobe32(crc);
    status = pkt_decode(input, 4, pkt);

    CU_ASSERT_NOT_EQUAL(status, PKT_OK);

    /*testing for PTYPE_ACK with payload*/

    input[0] = 0b01010001;
    crc = crc32(0, input, 12);
    crc = htobe32(crc);
    status = pkt_decode(input, 4, pkt);

    CU_ASSERT_NOT_EQUAL(status, PKT_OK);

    pkt_del(pkt);
}

int main(void)
{
    CU_pSuite paktSuite;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add suite to the registry */
    paktSuite = CU_add_suite("packet_suite", NULL, NULL);

    if (!paktSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    if (!(CU_add_test(paktSuite, "test_encode", test_encode)) ||
        !(CU_add_test(paktSuite, "test_decode", test_decode)))
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
