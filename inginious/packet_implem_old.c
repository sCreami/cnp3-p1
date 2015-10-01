#include "packet_interface.h"

/*
 * creer cle ssh github
 */

#define TYPE_SIZE 3
#define WINDOW_SIZE 5
#define SEQNUM_SIZE 8
#define LENGTH_SIZE 16
#define HEADER_SIZE 32
#define PAYLOAD_SIZE 4096
#define CRC_SIZE 32

/* Extra #includes */
/* Your code will be inserted here */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

typedef struct pkt_header
{
	ptypes_t type;
	uint8_t window;
	uint8_t seqnum;
	uint16_t length;
} pkt_header_t;

struct __attribute__((__packed__)) pkt
{
	struct pkt_header * header;
	char * payload;
	uint32_t crc;
};

void buffer_print(char * buffer, int length);

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
	pkt_t * result = (pkt_t *)malloc(sizeof(pkt_t));

	if (result == NULL)
		return NULL;

	result->header = (pkt_header_t *)malloc(sizeof(pkt_header_t));

	if (result->header == NULL)
	{
		free(result);
		return NULL;
	}

	result->header->type = PTYPE_DATA;
	result->header->window = 0;
	result->header->seqnum = 0;
	result->header->length = 0;

	result->payload = NULL;
	result->crc = 0;

	return result;
}

void pkt_del(pkt_t *pkt)
{
	if (pkt == NULL)
		return;

	free(pkt->header);
    free(pkt->payload);
	free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	/*TODO : MANAGE INVALID PKT AND CHECK CRC*/

	uint32_t crc = (uint32_t)*(data + len - CRC_SIZE / 8);

	/*extracting header*/

	pkt_set_type(pkt, (uint8_t)(data[0]) >> 5);
	pkt_set_window(pkt, (uint8_t)(data[0] & 0b00011111));
	pkt_set_seqnum(pkt, (uint8_t)data[1]);

	uint16_t length = (uint16_t)data[2];
	length = (length << 8) + (uint16_t)data[3];
	pkt_set_length(pkt, length);

	/*extracting payload*/

	if (pkt_get_length(pkt) > 0)
		pkt_set_payload(pkt, data + HEADER_SIZE / 8, pkt_get_length(pkt));

	/*extracting crc*/

	pkt_set_crc(pkt, crc);

	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	/*TODO : MANAGE INVALID PKT*/

	/*building header*/

	buf[0] = (char)((pkt_get_type(pkt) << 5) + pkt_get_window(pkt));
	buf[1] = (char)pkt_get_seqnum(pkt);
	buf[2] = (char)((pkt_get_length(pkt) & 0b1111111100000000) >> 8);
	buf[3] = (char)(pkt_get_length(pkt) & 0b0000000011111111);

	/*building payload*/

	if (pkt->payload != NULL)
		strcpy(buf + HEADER_SIZE / 8, pkt_get_payload(pkt));

	/*building crc*/

	uint32_t crc = pkt_get_crc(pkt);

	if (pkt->payload != NULL)
		strncpy(buf + HEADER_SIZE + strlen(pkt->payload), (char *)&crc, CRC_SIZE);
	else
		strncpy(buf + HEADER_SIZE, (char *)&crc, CRC_SIZE);

	*len = HEADER_SIZE / 8 + pkt_get_length(pkt) + CRC_SIZE / 8;

	return PKT_OK;
}

/*SETTERS*/

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if (pkt == NULL)
		return E_UNCONSISTENT;
	else if (pkt->header == NULL)
		return E_NOHEADER;

	pkt->header->type = type;

	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if (pkt == NULL)
		return E_UNCONSISTENT;
	else if (pkt->header == NULL)
		return E_NOHEADER;

	pkt->header->window = window;

	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	if (pkt == NULL)
		return E_UNCONSISTENT;
	else if (pkt->header == NULL)
		return E_NOHEADER;

	pkt->header->seqnum = seqnum;

	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if (pkt == NULL)
		return E_UNCONSISTENT;
	else if (pkt->header == NULL)
		return E_NOHEADER;

	pkt->header->length = length;

	return PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
	if (pkt == NULL)
		return E_UNCONSISTENT;

	pkt->crc = crc;

	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
	printf("%s %d\n", data, (int)length);

	/*checking if pkt is valid*/

	if (pkt == NULL)
		return E_UNCONSISTENT;
	else if (length != pkt->header->length) /*or change length in header ???*/
		return E_LENGTH;
	else if (length > MAX_PAYLOAD_SIZE)
		return E_LENGTH;

	/*freeing old payload, getting memory for new payload*/

	free(pkt->payload);
	pkt->payload = (char *)malloc(length + length % 4 + 1);

	if (pkt->payload == NULL)
		return E_NOMEM;

	/*writing data into payload*/

	pkt->payload[length + length % 4] = '\0';
	strncpy(pkt->payload, data, length);

	/*adding padding bytes*/

	char PADDING_CHAR = '='; /*padding character*/

	int len = length;
	while (len % 4 != 0)
		pkt->payload[len++] = PADDING_CHAR;

	return PKT_OK;
}

/*GETTERS*/

/*pkt is assumed to be != NULL*/

ptypes_t pkt_get_type(const pkt_t* pkt)
{
	return pkt->header->type;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
	return pkt->header->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->header->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return pkt->header->length;
}

uint32_t pkt_get_crc(const pkt_t* pkt)
{
	return pkt->crc;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
	return (const char *)pkt->payload;
}

/*PRINT FUNCTIONS*/

void pkt_print(pkt_t * pkt)
{
	if (pkt == NULL)
		return;

	printf("<HEADER type=%d window=%d, seqnum=%d, length=%d/>\n",
	(int)pkt->header->type, (int)pkt->header->window, (int)pkt->header->seqnum,
	(int)pkt->header->length);

	printf("<PAYLOAD>\n");
	printf("\t%s\n", pkt->payload);

	printf("</PAYLOAD>\n<CRC crc32=%ld/>\n", (unsigned long)pkt->crc);
}

void buffer_print(char * buffer, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		printf("%d ", (int)buffer[i]);
	}
	printf("\n");
}

int main()
{
	pkt_t * pkt = pkt_new();

	if (pkt == NULL)
		return EXIT_FAILURE;

	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_window(pkt, 17);
	pkt_set_seqnum(pkt, 42);
	pkt_set_length(pkt, 9);

	pkt_set_payload(pkt, "12345678", 8);
	pkt_set_crc(pkt, 96);

	pkt_print(pkt);

	char buffer[2048];
	size_t length = 2047;

	pkt_encode(pkt, buffer, &length);

	buffer_print(buffer, length);

	pkt_del(pkt);
	pkt = pkt_new();

	pkt_decode(buffer, length, pkt);

	pkt_print(pkt);
}
