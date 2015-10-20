CC      = gcc
CFLAGS  = -Wall      \
          -Werror    \
          -Wshadow   \
          -Wextra    \
          -std=gnu99 \
          -g         \
          -I$(HOME)/local/include
LFLAGS  = -lz -L$(HOME)/local/lib -lcunit

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

all: receiver sender

receiver: src/receiver.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

sender: src/sender.o src/socket.o src/packet.o
	$(CC) $^ -o $@ $(LFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test: tests/t_main.c tests/t1.c tests/t2.c src/socket.o src/packet.o
	$(CC) $(CFLAGS) -c tests/t1.c -o tests/t1.o -I$(HOME)/local/include
	$(CC) $(CFLAGS) -c tests/t2.c -o tests/t2.o -I$(HOME)/local/include
	$(CC) $(CFLAGS) -c tests/t_main.c -o tests/t_main.o -I$(HOME)/local/include
	$(CC) tests/t1.o tests/t2.o tests/t_main.o src/socket.o src/packet.o -o test $(LFLAGS)

clean:
	rm -f $(OBJ)
