CC      = gcc
CFLAGS  = -Wall      \
          -Werror    \
          -Wshadow   \
          -Wextra    \
          -std=gnu99 \
          -g
LFLAGS  = -lz

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

sender: src/sender.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test: src/socket.o src/packet.o
	$(CC) $(CFLAGS) -c tests/t1.c -o tests/t1.o -I$(HOME)/local/include
	$(CC) tests/t1.o src/socket.o -o test -L$(HOME)/local/lib -lcunit

clean:
	rm -f $(OBJ)
