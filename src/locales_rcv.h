#ifndef _LOCALES_RCV_H
#define _LOCALES_RCV_H

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
    .filename = "/stdout",
    .verbose  = 0,
};

#endif /* _LOCALES_RCV_H */
