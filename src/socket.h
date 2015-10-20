#ifndef _SOCKET_H
#define _SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> /* inet_pton */

#if __APPLE__
    #include "osdep/endian.h"
#endif

#include "locales.h"

int connect_socket(void);

int real_address(const char *address, struct sockaddr_in6 *rval);

#endif /* _SOCKET_H */
