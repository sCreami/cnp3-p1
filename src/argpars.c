/* Fill the local configuration following the parameters given in arguments.
 * It reads argc and argv to fill the static structure locales with valid data.
 */
void arguments_parser(int argc, char **argv)
{
    char *opt, *endptr;
    int port;

    for (int i = 1; i < argc; i++) {
        opt = argv[i];

        recycle:
        if (!strncmp(opt, "-f", 2) || !strncmp(opt, "--filename", 10)) {
            // followed by filename if presents
            locales.filename = argv[++i];
        }

        else if (strstr(opt, ":") || !strncmp(opt, "localhost", 9)) {
            // assuming address
            locales.addr = opt;

            // followed by port if presents
            if ((opt = argv[++i])) {
                port = atoi(opt);
                locales.port = (port ? port : 8080);
                if (!port) goto recycle;
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
                fprintf(stderr, "illegal option: %s\n", opt);
        }
    }
}
