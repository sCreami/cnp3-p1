CC      = gcc
CFLAGS  = -Wall -Werror -Wshadow -Wextra \
          -std=gnu99 \
          -g
LFLAGS  = -lz
CCFLAGS = -I$(HOME)/local/include
CLFLAGS = -L$(HOME)/local/lib -lcunit

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

.PHONY: test clean

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

sender: src/sender.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

test:
	@tests/test.sh

cunit: all tests/t.c src/packet.o
	$(CC) $(CCFLAGS) -c tests/t.c -o tests/t.o
	$(CC) tests/t.o src/packet.o -o test $(CLFLAGS) $(LFLAGS)

clean:
	rm -f $(OBJ)
