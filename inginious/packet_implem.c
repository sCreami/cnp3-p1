#include "packet_interface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#include <fcntl.h>
#include <unistd.h>

#define TYPE_SIZE 3
#define WINDOW_SIZE 5
#define SEQNUM_SIZE 8
#define LENGTH_SIZE 16
#define HEADER_SIZE 32
#define PAYLOAD_SIZE 4096
#define CRC_SIZE 32

struct __attribute__((__packed__)) pkt
{
	/*header*/

	uint8_t meta;
	uint8_t seqnum;
	uint16_t length;

	/*content*/

	char * payload;
	uint32_t crc;
};

/* PROTOTYPES */

pkt_t * pkt_build(ptypes_t t, uint8_t w, uint8_t s, char * payload);

void pkt_print(pkt_t * pkt);
void buffer_print(char * buffer, int length);

/*ADDS*/

pkt_t * pkt_build(ptypes_t t, uint8_t w, uint8_t s, char * payload)
{
	pkt_t * result;
	result = (pkt_t *)malloc(sizeof(pkt_t));

	if (result == NULL)
		return NULL;

	result->meta = 0;
	result->seqnum = s;
	result->length = 0;

	pkt_set_type(result, t);
	pkt_set_window(result, w);

	if (payload == NULL)
		result->payload = NULL;
	else
		pkt_set_payload(result, payload, strlen(payload));

	result->crc = 0; /*improve*/

	return result;
}


/*BUILDER & DESTROYER*/

pkt_t* pkt_new()
{
	pkt_t * result = pkt_build(PTYPE_DATA, 0, 0, NULL);

	if (result == NULL)
		return NULL;

	return result;
}

void pkt_del(pkt_t *pkt)
{
	if (pkt == NULL)
		return;

	free(pkt->payload);
	free(pkt);
}

/*DECODER & ENCODER*/

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	if (len < HEADER_SIZE / 8)
		return E_NOHEADER;

	/*decoding header*/

	pkt->meta = (uint8_t)data[0];
	pkt->seqnum = (uint8_t)data[1];

	char * length = (char *)(data + 2);
	pkt->length = *(le16toh(uint16_t *)length);

	uint8_t type = pkt_get_type(pkt);

	if (type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK)
		return E_TYPE;
	else if (pkt->length > MAX_PAYLOAD_SIZE)
		return E_LENGTH;
	else if (pkt_get_window(pkt) > MAX_WINDOW_SIZE)
		return E_WINDOW;

	/*decoding crc*/

	uint32_t rec_crc = crc32(0, (const Bytef *)data, (size_t)(len - CRC_SIZE / 8));
	char * crc = (char *)(data + len - CRC_SIZE / 8);
	pkt->crc = *(le32toh(uint32_t *)crc);

	if (rec_crc != pkt->crc)
		return E_CRC;

	/*decoding payload*/

	if (pkt_get_type(pkt) != PTYPE_DATA)
	{
		if (len == (HEADER_SIZE + CRC_SIZE) / 8)
			return PKT_OK;
		else
			return E_LENGTH;
	}

	if (pkt_get_length(pkt))
	{
		if (len < (size_t)(pkt_get_length(pkt) + (HEADER_SIZE + CRC_SIZE) / 8))
		{
			if (len == (HEADER_SIZE + CRC_SIZE) / 8)
				return E_NOPAYLOAD;
			else
				return E_LENGTH;
		}
		else
		{
			pkt_set_payload(pkt, data + HEADER_SIZE / 8, pkt_get_length(pkt));
		}
	}
	else
	{
		return E_NOPAYLOAD;
	}

	if (strlen(pkt_get_payload(pkt)) % 4)
		return E_PADDING;
	else
		return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	/*encoding header*/

	buf[0] = pkt->meta;
	buf[1] = pkt->seqnum;

	char * length = (char *)&(htole16(pkt->length));
	buf[2] = length[0];
	buf[3] = length[1];
	*len = 4;

	/*encoding payload*/

	char * payload = (char *)pkt_get_payload(pkt);

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

	char * crc = (char *)&(htole32(pkt->crc));
	buf[*len + 0] = crc[0];
	buf[*len + 1] = crc[1];
	buf[*len + 2] = crc[2];
	buf[*len + 3] = crc[3];
	*len += 4;

	return PKT_OK;
}

/*SETTERS*/

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if (type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK)
		return E_TYPE;

	pkt->meta = pkt->meta & 0b00011111;
	pkt->meta += (type << 5);

	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if (window > MAX_WINDOW_SIZE)
		return E_WINDOW;

	pkt->meta = pkt->meta & 0b11100000;
	pkt->meta += window;

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

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
	/*not checking anything here*/

	pkt->crc = crc;
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

	if (pkt->payload == NULL)
		return E_NOMEM;

	int i;
	for (i = len; i < length; i++)
		pkt->payload[i] = PADDING_CHAR;

	memcpy(pkt->payload, d, len);

	return pkt_set_length(pkt, len);
}

/*GETTERS*/

ptypes_t pkt_get_type(const pkt_t* pkt)
{
	return (pkt->meta >> 5);
}

uint8_t pkt_get_window(const pkt_t* pkt)
{
	return (pkt->meta & 0b00011111);
}

uint8_t pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return pkt->length;
}

uint32_t pkt_get_crc(const pkt_t* pkt)
{
	return pkt->crc;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
	return pkt->payload;
}

/*PRINT*/

void pkt_print(pkt_t * pkt)
{
	if (pkt == NULL)
		return;

	int type = pkt_get_type(pkt);
	int window = pkt_get_window(pkt);

	printf("<HEADER type=%d window=%d, seqnum=%d, length=%d/>\n", type, window,
	(int)pkt->seqnum, (int)pkt->length);

	printf("<PAYLOAD>\n");
	printf("\t%s\n", pkt->payload);

	printf("</PAYLOAD>\n<CRC crc32=%ld/>\n", (unsigned long)pkt->crc);
}

void buffer_print(char * buffer, int length)
{
	int i;
	for (i = 0; i < length; i++)
		printf("%d ", (int)buffer[i]);
	printf("\n");
}

/*MAIN*/

int main()
{
	pkt_t * pkt = pkt_build(PTYPE_DATA, 17, 42, "12345");

	char buffer[2048];
	size_t length = 2048;

	pkt_encode(pkt, buffer, &length);
	uint32_t crc = crc32(0, (const Bytef *)buffer, (size_t)(length - CRC_SIZE / 8));
	pkt_set_crc(pkt, crc);
	//pkt_set_crc(pkt, 96);
	pkt_print(pkt);

	length = 2048;
	pkt_encode(pkt, buffer, &length);
	buffer_print(buffer, length);

	pkt_t * new_pkt = pkt_new();
	pkt_decode(buffer, length, new_pkt);
	pkt_print(new_pkt);

	return EXIT_SUCCESS;
}
