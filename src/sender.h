#ifndef _SENDER_H
#define _SENDER_H

// Saves the local configuration for
// the current instance.
struct snd_config {
    char *addr;
    int   port;
    int   verbose;
};

#endif /* _SENDER_H */
