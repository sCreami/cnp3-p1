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
	FILE* fp;
	int status;
	fd_set fds;
	char buffer[2048];
	struct timeval tv;

	fp = fdopen(sfd, "rw");

	tv = (struct timeval) {
		.tv_sec  = 2,
		.tv_usec = 0,
	};

	for (;;) {

		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		FD_SET(sfd, &fds);

		status = select(FD_SETSIZE, &fds, &fds, NULL, &tv);

		if (status == -1) {
			perror("select");
			return;
		}

		if (FD_ISSET(sfd, &fds)) {

			if (fread(buffer, sizeof(char), sizeof(buffer), fp) > 0) {

				if (fwrite(buffer, sizeof(char), sizeof(buffer), stdout) > 0) {
					perror("fwrite sfd");
					return;
				}

				bzero(buffer, sizeof(buffer));

			} else {
				perror("fread sfd");
				return;
			}
		}

		if (FD_ISSET(STDIN_FILENO, &fds)) {

			if (feof(stdin))
				return;
			
			if (fread(buffer, sizeof(char), sizeof(buffer), stdin) > 0) {

				if (fwrite(buffer, sizeof(char), sizeof(buffer), fp) > 0) {
					perror("fwrite stdin");
					return;
				}

				bzero(buffer, sizeof(buffer));

			} else {
				perror("fread stdin");
				return;
			}
		}
	}
}

// int main(int argc, char *argv[])
// {return 0;}
