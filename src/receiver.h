#ifndef _RECEIVER_H
#define _RECEIVER_H

// Saves the local configuration for
// the current instance.
struct rcv_config {
	char *addr;
	int   port;
	char *filename;
	int   verbose;
};

#endif /* _RECEIVER_H */
