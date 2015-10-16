#include "packet.h"

/*BUILDERS & DESTROYER*/

pkt_t *pkt_build(ptypes_t t, uint8_t w, uint8_t s, uint16_t l, char *payload)
{
    pkt_t *packet;

    packet =  (pkt_t *)malloc(sizeof(pkt_t));

    if (!packet)
        return NULL;

    *packet = (pkt_t)
    {
        .type   = t,
        .window = w,
        .seqnum = s,
        .length = l
    };

    if (!payload)
        packet->payload = NULL;
    else
        pkt_set_payload(packet, payload, l);

    return packet;
}

pkt_t *pkt_new()
{
    return pkt_build(PTYPE_DATA, 0, 0, 0, NULL);
}

void pkt_del(pkt_t *pkt)
{
    if (!pkt)
        return;

    free(pkt->payload);
    free(pkt);
}

/*DECODER & ENCODER*/

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    uint8_t type;
    uint32_t expected_crc, data_crc;

    if (len < 4)
        return E_NOHEADER;

    /*decoding header*/

    memcpy(pkt, data, 4);
    pkt->length = be16toh(pkt->length);

    type = pkt_get_type(pkt);

    if (type != PTYPE_DATA &&
        type != PTYPE_ACK  &&
        type != PTYPE_NACK)
        return E_TYPE;

    if (pkt->length > MAX_PAYLOAD_SIZE)
        return E_LENGTH;
    else if (pkt_get_window(pkt) > MAX_WINDOW_BIT_SIZE)
        return E_WINDOW;

    /*decoding crc*/

    expected_crc = crc32(0, (const Bytef *)data, len - 4);
    memcpy(&data_crc, data + len - 4, 4);
    data_crc = be32toh(data_crc);

    if (expected_crc != data_crc)
        return E_CRC;

    /*decoding payload*/

    if (pkt_get_type(pkt) != PTYPE_DATA)
    {
        if (len == 8)
            return PKT_OK;
        else
            return E_LENGTH;
    }

    if (pkt_get_length(pkt))
    {
        if (len < (size_t)(pkt_get_length(pkt) + 8))
        {
            if (len == 8)
                return E_NOPAYLOAD;
            else
                return E_LENGTH;
        }
        else
        {
            pkt_set_payload(pkt, data + 4, pkt_get_length(pkt));
        }
    }
    else
    {
        return E_NOPAYLOAD;
    }

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len)
{
    /*encoding header*/

    memcpy(buf, pkt, 2);

    uint16_t length_int = htobe16(pkt->length);
    memcpy(buf + 2, &length_int, 2);
    *len = 4;

    /*encoding payload*/

    char *payload = (char *)pkt_get_payload(pkt);

    int l = 0;
    if (payload != NULL)
    {
        l = pkt_get_length(pkt);
        if (l % 4)
            l += 4 - l % 4;
        memcpy(buf + *len, payload, l);
    }

    *len += l;

    /*encoding crc*/
    uint32_t crc = crc32(0, (Bytef*)buf, *len);
    uint32_t crc_int = htobe32(crc);
    memcpy(buf + *len, &crc_int, 4);
    *len += 4;

    return PKT_OK;
}

/*SETTERS*/

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if (type != PTYPE_DATA &&
        type != PTYPE_ACK  &&
        type != PTYPE_NACK)
        return E_TYPE;

    pkt->type = type;

    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if (window > MAX_WINDOW_BIT_SIZE)
        return E_WINDOW;

    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if (length > MAX_PAYLOAD_SIZE)
        return E_LENGTH;

    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *d, const uint16_t len)
{
    char PADDING_CHAR = 0b00000000;

    if (len > MAX_PAYLOAD_SIZE)
        return E_NOMEM;

    free(pkt->payload);

    int length = len;
    if (length % 4)
        length += 4 - length % 4;

    pkt->payload = (char *)malloc(length);

    if (!pkt->payload)
        return E_NOMEM;

    int i;
    for (i = len; i < length; i++)
        pkt->payload[i] = PADDING_CHAR;

    memcpy(pkt->payload, d, len);

    return pkt_set_length(pkt, len);
}

/*GETTERS*/

ptypes_t pkt_get_type(const pkt_t *pkt)
{
    return pkt->type;
}

uint8_t pkt_get_window(const pkt_t *pkt)
{
    return pkt->window;
}

uint8_t pkt_get_seqnum(const pkt_t *pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t *pkt)
{
    return pkt->length;
}

const char* pkt_get_payload(const pkt_t *pkt)
{
    return pkt->payload;
}

/*PRINT*/

void pkt_print(pkt_t *pkt)
{
    if (pkt == NULL)
        return;

    printf("<HEADER type=%d,"
           "window=%d,"
           "seqnum=%d,"
           "length=%d/>\n",
            (int)pkt_get_type(pkt),
            (int)pkt_get_window(pkt),
            (int)pkt->seqnum,
            (int)pkt->length);

    printf("<PAYLOAD>\n\t%s\n</PAYLOAD>\n", pkt->payload);
}

void buffer_print(char * buffer, int length)
{
    for (int i = 0; i < length; i++)
        printf("%u ", (unsigned char)buffer[i]);
    printf("\n");
}
