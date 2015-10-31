CFLAGS  = -Wall -Werror -Wshadow -Wextra -std=gnu99
LDFLAGS = -lz

.PHONY: tests clean

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o

sender: src/sender.o src/socket.o src/packet.o

%: src/%.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	@rm -f $(foreach ssrc, src tests tests/simlink, $(wildcard $(ssrc)/*.o))

# Don't want to install CUnit on OSX
ifneq ($(shell uname -s), Darwin)
LDFLAGS += -lcunit
tests:
	@$(MAKE) cunit > /dev/null
	./test
	@$(MAKE) all > /dev/null
	@tests/test.sh

cunit: tests/t.o src/packet.o
	$(CC) $^ -o test $(LDFLAGS)
endif
