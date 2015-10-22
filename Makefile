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
	@$(MAKE) cunit > /dev/null
	LD_LIBRARY_PATH=$(HOME)/local/lib ./test
	@$(MAKE) all > /dev/null
	@tests/test.sh

cunit: tests/t.c src/packet.o
	$(CC) $(CCFLAGS) -c tests/t.c -o tests/t.o
	$(CC) tests/t.o src/packet.o -o test $(CLFLAGS) $(LFLAGS)

clean:
	rm -f $(OBJ)
