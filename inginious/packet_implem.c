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

pkt_status_code pkt_header_decode(const char * data,
	 						      const size_t len,
								  pkt_header_t *h);
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

	if (payload == NULL)
		result->payload = NULL;
	else
		pkt_set_payload(result, payload, strlen(payload));

	result->crc = 0; /*improve*/

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

pkt_status_code pkt_header_decode(const char * data,
	 						      const size_t len,
								  pkt_header_t *h)
{
	if (len < HEADER_SIZE / 8)
		return E_NOHEADER;

	h->meta = (uint8_t)data[0];
	h->seqnum = (uint8_t)data[1];

	char * length = (char *)(data + 2);
	h->length = *(le16toh(uint16_t *)length);

	uint8_t type = pkt_header_get_type(h);

	if (type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK)
		return E_TYPE;
	else if (h->length > MAX_PAYLOAD_SIZE)
		return E_LENGTH;
	else if (pkt_header_get_window(h) > MAX_WINDOW_SIZE)
		return E_WINDOW;

	return PKT_OK;
}

pkt_status_code pkt_header_encode(pkt_header_t *h, char * buf, size_t * len)
{
	buf[0] = h->meta;
	buf[1] = h->seqnum;

	char * length = (char *)&(htole16(h->length));
	buf[2] = length[0];
	buf[3] = length[1];
	*len = 4;

	return PKT_OK;
}

pkt_status_code pkt_header_set_type(pkt_header_t *h, const ptypes_t type)
{
	if (type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK)
		return E_TYPE;

	h->meta = h->meta & 0b00011111;
	h->meta += (type << 5);

	return PKT_OK;
}

pkt_status_code pkt_header_set_window(pkt_header_t *h, const uint8_t window)
{
	if (window > MAX_WINDOW_SIZE)
		return E_WINDOW;

	h->meta = h->meta & 0b11100000;
	h->meta += window;

	return PKT_OK;
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
	if (len < HEADER_SIZE / 8)
		return E_NOHEADER;

	/*decoding header*/

	pkt_status_code code = pkt_header_decode(data, len, pkt->header);

	if (code != PKT_OK)
		return code;

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
	if (*len < (size_t)((HEADER_SIZE + CRC_SIZE) / 8 + pkt_get_length(pkt)))
		return E_NOMEM;

	/*encoding header*/

	pkt_header_encode(pkt->header, buf, len);

	/*encoding payload*/

	char * payload = (char *)pkt_get_payload(pkt);

	int l = 0;
	if (payload != NULL)
	{
		l = pkt_get_length(pkt);
		if (l % 4)
		 	l += 4 - l % 4;
		strncpy(buf + *len, payload, l);
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
	return pkt_header_set_type(pkt->header, type);
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	return pkt_header_set_window(pkt->header, window);
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	pkt->header->seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if (length > MAX_PAYLOAD_SIZE)
		return E_LENGTH;

	pkt->header->length = length;
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

	strncpy(pkt->payload, d, len);

	return pkt_set_length(pkt, len);
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
	(int)pkt->header->seqnum, (int)pkt->header->length);

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
	pkt_t * pkt = pkt_build(pkt_header_build(PTYPE_DATA, 17, 42), "12345");

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
	
	pkt_t * new_pkt = pkt_build(pkt_header_build(PTYPE_ACK, 0, 0), NULL);
	pkt_decode(buffer, length, new_pkt);
	pkt_print(new_pkt);

	return EXIT_SUCCESS;
}
