#ifndef _LOCALES_H
#define _LOCALES_H

// Term colors
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

/* Saves the local configuration for
 * the current instance. */
struct config {
    
    // ID
    int idef; /* identify recv or snd */

    // Arguments
    char *addr;
    int   port;
    char *filename;
    int   verbose;

    // Socket
    int sockfd;
    int passive; /* connect or bind */
};

extern struct config locales;

#endif /* _LOCALES_H */
