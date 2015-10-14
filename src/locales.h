#ifndef _LOCALES_H
#define _LOCALES_H

/* Saves the local configuration for
 * the current instance. */
struct config {
    
    // Arguments
    char *addr;
    int   port;
    char *filename;
    int   verbose;

    // Socket
    int sockfd;
};

extern struct config locales;

#endif /* _LOCALES_H */
