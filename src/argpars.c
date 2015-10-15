/*
 * THIS FILE IS A TEMPLATE INCLUDED IN SENDER.C
 * AND RECEIVER.C TO AVOID REDUNDANCY. DO NOT
 * DEFINE ANY MACRO IN THIS FILE !
 */

/* Fill the local configuration following the parameters given in arguments.
 * It reads argc and argv to fill the static structure locales with valid data. */
void arguments_parser(int argc, char **argv)
{
    char *opt, *endptr;
    int port;

    for (int i = 1; i < argc; i++) {
        opt = argv[i];

        recycle:
        if (!strcmp(opt, "-f") || !strcmp(opt, "--filename")) {
            // followed by filename if presents
            locales.filename = argv[++i];
        }

        else if (strstr(opt, ":") || !strcmp(opt, "localhost")) {

            if (locales.idef && !strcmp(opt, "::")) {
                // listen to all interfaces
                locales.addr = opt;
                locales.passive = 1;
            }

            else {
                // assuming address
                locales.addr = opt;
            }

            // followed by port if presents
            if ((opt = argv[++i])) {
                port = atoi(opt);
                locales.port = (port ? port : 8080);
                if (!port) goto recycle;
            }
        }

        else if (!strcmp(opt, "-v") || !strcmp(opt, "--verbose")) {
            // define verbosity
            locales.verbose = 1;
        }

        else {
            // looking for aliens
            strtol(opt, &endptr, 10);
            if (*endptr != '\0')
                fprintf(stderr, "illegal option: %s\n", opt);
        }
    }
}
