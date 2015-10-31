CFLAGS  = -Wall -Werror -Wshadow -Wextra -std=gnu99
LDFLAGS = -lz -lcunit

.PHONY: tests clean

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LDFLAGS)

sender: src/sender.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LDFLAGS)

tests:
	@$(MAKE) cunit > /dev/null
	./test
	@$(MAKE) all > /dev/null
	@tests/test.sh

cunit: tests/t.o src/packet.o
	$(CC) $^ -o test $(LDFLAGS)

clean:
	@rm -f $(foreach ssrc, src tests tests/simlink, $(wildcard $(ssrc)/*.o))
