#ifndef _LOCALES_SND_H
#define _LOCALES_SND_H

// Saves the local configuration for
// the current instance.
struct config {
    
    // Arguments
    char *addr;
    int   port;
    char *filename;
    int   verbose;

    // CPU time
    clock_t start;
    clock_t stop;

    // Socket
    int sockfd;
};

// Global locales
static struct config locales = {
    .addr     = "localhost",
    .port     = 8080,
    .filename = "/stdin",
    .verbose  = 0,
};

#endif /* _LOCALES_SND_H */
