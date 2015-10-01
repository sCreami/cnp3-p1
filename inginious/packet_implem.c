#include "packet_interface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#define TYPE_SIZE 3
#define WINDOW_SIZE 5
#define SEQNUM_SIZE 8
#define LENGTH_SIZE 16
#define HEADER_SIZE 32
#define PAYLOAD_SIZE 4096
#define CRC_SIZE 32

/*STRUCTURES*/

typedef struct pkt_header
{
	uint8_t meta;
	uint8_t seqnum;
	uint16_t length;
} pkt_header_t;

struct __attribute__((__packed__)) pkt
{
	struct pkt_header * header;
	char * payload;
	uint32_t crc;
};

/*PROTOTYPES*/

pkt_header_t * pkt_header_new();
pkt_header_t * pkt_header_build(ptypes_t t, uint8_t w, uint8_t s);
pkt_t * pkt_build(pkt_header_t *header, char * payload);
void pkt_header_del(pkt_header_t *header);

pkt_status_code pkt_header_encode(pkt_header_t *h, char * buf, size_t * len);

pkt_status_code pkt_header_set_type(pkt_header_t *h, const ptypes_t type);
pkt_status_code pkt_header_set_window(pkt_header_t *h, const uint8_t window);

ptypes_t pkt_header_get_type  (const pkt_header_t* h);
uint8_t  pkt_header_get_window(const pkt_header_t* h);

void pkt_print(pkt_t * pkt);
void buffer_print(char * buffer, int length);

/*ADDS*/

pkt_header_t * pkt_header_build(ptypes_t t, uint8_t w, uint8_t s)
{
	pkt_header_t * result;
	result = (pkt_header_t *)malloc(sizeof(pkt_header_t));

	if (result == NULL)
		return NULL;

	result->meta = 0;
	result->seqnum = s;
	result->length = 0;

	pkt_header_set_type(result, t);
	pkt_header_set_window(result, w);

	return result;
}

pkt_t * pkt_build(pkt_header_t * header, char * payload)
{
	if (header == NULL)
		return NULL;

	pkt_t * result;
	result = (pkt_t *)malloc(sizeof(pkt_t));

	if (result == NULL)
		return NULL;

	result->header = header;
	result->payload = payload;
	result->crc = 0; /*improve*/

	if (result->payload != NULL)
		pkt_set_length(result, strlen(result->payload));

	return result;
}

pkt_header_t * pkt_header_new()
{
	pkt_header_t * result = pkt_header_build(PTYPE_DATA, 0, 0);

	if (result == NULL)
		return NULL;

	return result;
}

void pkt_header_del(pkt_header_t * header)
{
	free(header);
}

pkt_status_code pkt_header_encode(pkt_header_t *h, char * buf, size_t * len)
{
	buf[0] = h->meta;
	buf[1] = h->seqnum;

	char * length = (char *)&(h->length); /*! endian*/
	buf[2] = length[1];
	buf[3] = length[0];
	*len = 4;

	return PKT_OK;
}

pkt_status_code pkt_header_set_type(pkt_header_t *h, const ptypes_t type)
{
	h->meta = h->meta & 0b00011111;
	h->meta += (type << 5);
}

pkt_status_code pkt_header_set_window(pkt_header_t *h, const uint8_t window)
{
	h->meta = h->meta & 0b11100000;
	h->meta += window;
}

ptypes_t pkt_header_get_type  (const pkt_header_t* h)
{
	return (h->meta >> 5);
}

uint8_t  pkt_header_get_window(const pkt_header_t* h)
{
	return (h->meta & 0b00011111);
}

/*BUILDERS*/

pkt_t* pkt_new()
{
	pkt_t * result = pkt_build(pkt_header_new(), NULL);

	if (result == NULL)
		return NULL;

	return result;
}

void pkt_del(pkt_t *pkt)
{
	if (pkt == NULL)
		return;

	pkt_header_del(pkt->header);
	free(pkt->payload);
	free(pkt);
}

/*ENCODE & DECODE*/

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	/*encoding header*/

	pkt_header_encode(pkt->header, buf, len);

	/*encoding payload*/

	char *new_buf = buf + *len;

	/*encodeing crc*/

	/*...*/

	return PKT_OK;
}

/*SETTERS*/

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	pkt_header_set_type(pkt->header, type);
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	pkt_header_set_window(pkt->header, window);
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	pkt->header->seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	pkt->header->length = length;
	return PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
	pkt->crc = crc;
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *d, const uint16_t len)
{
	free(pkt->payload);
	pkt->payload = (char *)malloc(len + 1);
	strncpy(pkt->payload, d, len);
	pkt->payload[len] = '\0';

	return PKT_OK;
}

/*GETTERS*/

ptypes_t pkt_get_type(const pkt_t* pkt)
{
	return pkt_header_get_type(pkt->header);
}

uint8_t pkt_get_window(const pkt_t* pkt)
{
	return pkt_header_get_window(pkt->header);
}

uint8_t pkt_get_seqnum(const pkt_t* pkt)
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
	char PADDING_CHAR = '=';

	int length = pkt_get_length(pkt);

	if (length % 4)
		length += 4 - length % 4;

	char * result = (char *)malloc(length + 1);
	result[length] = '\0';

	if (result == NULL)
		return NULL;

	int i;
	for (i = 0; i < length; i++)
		result[i] = PADDING_CHAR;

	strncpy(result, pkt->payload, pkt_get_length(pkt));

	return result;
}

/*PRINT*/

void pkt_print(pkt_t * pkt)
{
	if (pkt == NULL)
		return;

	int type = pkt_get_type(pkt);
	int window = pkt_get_window(pkt);

	printf("<HEADER type=%d window=%d, seqnum=%d, length=%d/>\n", type, window,
	(int)pkt->header->seqnum, (int)pkt->header->length);

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

/*MAIN*/

int main()
{
	pkt_t * pkt = pkt_build(pkt_header_build(PTYPE_DATA, 17, 42), "123456789");
	pkt_print(pkt);

	char buffer[2048];
	size_t length = 2048;

	pkt_header_encode(pkt->header, buffer, &length);
	buffer_print(buffer, length);
}
