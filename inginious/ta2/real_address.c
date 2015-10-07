#include "real_address.h"

#include <arpa/inet.h> /* inet_ntop */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* bzero */

/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval)
{
	struct addrinfo hints, *res, *p;
	int status;

	bzero(&hints, sizeof(struct addrinfo));

	hints = (struct addrinfo) {
		.ai_family    = AF_INET6,
		.ai_socktype  = SOCK_DGRAM,
		.ai_flags     = 0,
		.ai_protocol  = 0,
	};

	status = getaddrinfo(address, NULL, &hints, &res);

	if (status)
		return gai_strerror(status);

	for (p = res; p != NULL; p = p->ai_next) {
		rval = (struct sockaddr_in6 *) p->ai_addr;
	}

	return NULL;
}

// int main(int argc, char *argv[])
// {
// 	argc++; //FY

// 	struct sockaddr_in6 rval;
// 	const char *s = real_address(argv[1], &rval);

// 	if (s) {
// 		printf("%s\n", s);
// 		return 1;
// 	}

// 	char buf[30];
// 	printf("%s\n", inet_ntop(AF_INET6, &rval.sin6_addr, buf, sizeof(buf)));

// 	return 0;
// }
