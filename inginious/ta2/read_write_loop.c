#include "read_write_loop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(const int sfd)
{
    fd_set fds;
    char buffer[1024];
    struct timeval tv;
    ssize_t rsize, wsize;

    tv = (struct timeval) {
        .tv_sec  = 5,
        .tv_usec = 0,
    };

    for (;;) {

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(sfd, &fds);

        if (select(FD_SETSIZE, &fds, NULL, NULL, &tv) == -1) {
            perror("select");
            return;
        }

        if (FD_ISSET(sfd, &fds)) {

            if ((rsize = read(sfd, buffer, 1024)) > 0) {

                if (write(STDOUT_FILENO, buffer, rsize) < 0) {
                    perror("write sfd");
                    return;
                }

                fflush(stdout);
                bzero(buffer, sizeof(buffer));

            } else {
                perror("read sfd");
                return;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {

            if ((wsize = read(STDIN_FILENO, buffer, 1024)) > 0) {

                if (write(sfd, buffer, wsize) < 0) {
                    perror("write stdin");
                    return;
                }

                bzero(buffer, sizeof(buffer));

            } else if (wsize == 0) {
                //EOF
                return;
                
            } else {
                perror("read stdin");
                return;
            }
        }
    }
}
