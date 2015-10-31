#ifndef _PACKET_H
#define _PACKET_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stddef.h>
#include <stdint.h>

#include <zlib.h>
#include <fcntl.h>
#include <unistd.h>

#if __APPLE__
    #include "osdep/endian.h"
#endif

/*header size, a.k.a type, window, seqnum and length*/
#define HEADER_SIZE 4
/* Maximum allowed payload size, including padding bytes if any */
#define MAX_PAYLOAD_SIZE 512
/* Maximum allowed window size */
#define MAX_WINDOW_BIT_SIZE 31

typedef enum
{
	PTYPE_DATA = 1,
	PTYPE_ACK = 2,
	PTYPE_NACK = 4
} ptypes_t;

/* Function error code */
typedef enum {
	PKT_OK = 0,     /* The packet has been processed successfully */
	E_TYPE,         /* Invalid type */
	E_LENGTH,       /* Invalid length */
	E_CRC,          /* Invalid CRC */
	E_WINDOW,       /* Invalid window */
	E_SEQNUM,       /* Invalid sequence number */
	E_PADDING,      /* Invalid padding in the payload */
	E_NOMEM,        /* Not enough memory */
	E_NOPAYLOAD,    /* Packet has no payload */
	E_NOHEADER,     /* Packet is too short thus has no header */
	E_UNCONSISTENT, /* Packet is unconsistent */
} pkt_status_code;

typedef struct __attribute__((__packed__)) pkt
{
    /* Header */

    uint8_t  window  : 5;
    ptypes_t type    : 3;
    uint8_t  seqnum  : 8;
    uint16_t length  : 16;

    /* Content */

    char *   payload;
} pkt_t;

/* Creates a new struct pkt and allows to initialize it's fields
 * @return: A new struc pkt with it's fields initialized to specified values
            NULL if the system is out of memory */
pkt_t *pkt_build(ptypes_t t, uint8_t w, uint8_t s, uint16_t l, char *payload);

/* Create a new struct pkt, to be free'd by pkt_del
 * @return: An initialized struct pkt,
 * 			NULL if the system is out of memory */
pkt_t * pkt_new();
/* Free up all resources taken by the given struct pkt */
void pkt_del(pkt_t *);

/*
 * Decode data that has been received and create a new pkt_t from it.
 * This functions performs the following checks:
 * - Validates the CRC32 of the received data against the one present at the
 *   end of the data stream
 * - Validates the type of the packet
 * - Validates the length of the packet
 *
 * @data: The set of bytes that constitutes the packet
 * @len: The number of bytes received
 * @pkt: An allocated struct pkt, where the decoded data should be stored.
 *
 * @return: A status code indicating the success or the failure type.
 * 			Unless the error is E_NOHEADER, the packet has at least the
 * 			values of the header found in the data stream.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt);

/*
 * Convert a struct pkt into a set of bytes ready to be sent over the wires,
 * including the CRC32 of the header & payload of the packet
 *
 * @pkt: The struct that contains the info about the packet to send
 * @buf: A buffer to store the resulting set of bytes
 * @len: The number of bytes that can be written in buf.
 * @len-POST: The number of bytes written in the buffer by the function.
 * @return: A status code indicating the success or E_NOMEM if the buffer is
 * 		too small
 * */
pkt_status_code pkt_encode(const pkt_t *, char *buf, size_t *len);

/* Getters for the mandatory fields of the struct pkt */
ptypes_t pkt_get_type  (const pkt_t * pkt);
uint8_t  pkt_get_window(const pkt_t * pkt);
uint8_t  pkt_get_seqnum(const pkt_t * pkt);
uint16_t pkt_get_length(const pkt_t * pkt);

/* This function must return the full payload of the packet,
 * including the eventual padding bytes. */
const char* pkt_get_payload(const pkt_t *);

/* Setters for the mandatory fields of the struct pkt. If the value does not
 * fit in the appropriate ranges for the field, the functions must return
 * an appropriate error code */
pkt_status_code pkt_set_type  (pkt_t *, const ptypes_t type);
pkt_status_code pkt_set_window(pkt_t *, const uint8_t window);
pkt_status_code pkt_set_seqnum(pkt_t *, const uint8_t seqnum);
pkt_status_code pkt_set_length(pkt_t *, const uint16_t length);

/* Set the value of the payload field of the pkt,
 * @data: a byte array that represents the data to store as payload
 * @length: the number of bytes of that byte array. If length is not a multiple
 * of 4, a padding made of null bytes must be appended to the byte array.
 * @POST: length must be the new value of the Length field of the packet */
pkt_status_code pkt_set_payload(pkt_t *, const char *data,
                                const uint16_t length);

/* Prints the fields of a struct pkt in XML format*/
void pkt_print(pkt_t *pkt);
/* Prints length elements of an array of char as unsigned integers*/
void buffer_print(char *buffer, int length);

#endif /* _PACKET_H */
