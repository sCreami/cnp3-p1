#ifndef _SENDER_H
#define _SENDER_H

// Saves the local configuration for
// the current instance.
struct snd_config {
    
    // Arguments
    char *addr;
    int   port;
    int   verbose;

    // CPU time
    clock_t start;
    clock_t stop;
};

#endif /* _SENDER_H */
