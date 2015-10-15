CC      = gcc
CFLAGS  = -Wall      \
          -Werror    \
          -Wshadow   \
          -Wextra    \
          -std=gnu99 \
          -g
LFLAGS  = #-lz

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

all: receiver sender

receiver: src/receiver.o src/socket.o
	$(CC) $(LFLAGS) $^ -o $@

sender: src/sender.o src/socket.o
	$(CC) $(LFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(OBJ)
