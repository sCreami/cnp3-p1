CC      = gcc
CFLAGS  = -Wall -Werror -Wshadow -Wextra \
          -std=gnu99
LFLAGS  = -lz -lcunit

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

.PHONY: tests clean

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

sender: src/sender.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

tests:
	@$(MAKE) cunit > /dev/null
	./test
	@$(MAKE) all > /dev/null
	@tests/test.sh

cunit: tests/t.c src/packet.o
	$(CC) -c tests/t.c -o tests/t.o
	$(CC) tests/t.o src/packet.o -o test $(LFLAGS)

clean:
	rm -f $(OBJ)
