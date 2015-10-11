#ifndef _RECEIVER_H
#define _RECEIVER_H

// Saves the local configuration for
// the current instance.
struct rcv_config {

    // Arguments
    char *addr;
    int   port;
    char *filename;
    int   verbose;

    // CPU time
    clock_t start;
    clock_t stop;
};

#endif /* _RECEIVER_H */
