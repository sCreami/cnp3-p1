#include "wait_for_client.h"

#include <stdio.h>
#include <arpa/inet.h>

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the
 * message, and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd)
{
    char buffer[1024];
    struct sockaddr_in6 fr;
    socklen_t fr_len;

    fr_len = sizeof(struct sockaddr_in6);

    if (recvfrom(sfd, buffer, sizeof(buffer), MSG_PEEK,
                 (struct sockaddr *)&fr, &fr_len) != -1) {

        if (connect(sfd, (struct sockaddr *)&fr, fr_len) != -1) {
            return 0;
        }

        perror("connect");
        return -1;
    }

    perror("recvfrom");
    return -1;
}
