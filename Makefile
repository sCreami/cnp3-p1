CC      = gcc
CFLAGS  = -Wall    \
		  -Werror  \
		  -Wshadow \
		  -Wextra  \
		  -g
LFLAGS  = 

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

all: receiver sender

receiver: src/receiver.o
	$(CC) $(LFLAGS) $< -o $@

sender: src/sender.o
	$(CC) $(LFLAGS) $< -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJ)
