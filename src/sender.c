#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h> /* inet_pton */
#include <string.h>

#include "sender.h"

static struct snd_config locales = {
	.addr     = "localhost",
	.port     = 8080,
	.verbose  = 0,
};

void arguments_parser(int argc, char **argv)
{
	char *opt, *endptr;
	int port;

	for (int i = 1; i < argc; i++) {
		opt = argv[i];

		if (strstr(opt, ":") || !strncmp(opt, "localhost", 9)) {
			// assuming address
			locales.addr = opt;

			// followed by port if presents
			if ((opt = argv[++i])) {
				port = atoi(opt);
				locales.port = (port ? port : 8080);
			}
		}

		else if (!strncmp(opt, "-v", 2) || !strncmp(opt, "--verbose", 9)) {
			// define verbosity
			locales.verbose = 1;
		}

		else {
			// looking for aliens
			strtol(opt, &endptr, 10);
			if (*endptr != '\0')
				fprintf(stderr, "unknown argument: %s\n", opt);
		}
	}
}

int main(int argc, char **argv)
{
	arguments_parser(argc, argv);

	printf("Addresse:\t %s\n"
		   "Port:\t\t %d\n"
		   "Verbeux:\t %d\n",
		   locales.addr,
		   locales.port,
		   locales.verbose);


	/* code */
	return EXIT_SUCCESS;
}
