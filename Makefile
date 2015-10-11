CC      = gcc
EXEC    = sender receiver
CFLAGS  = -Wall    \
		  -Werror  \
		  -Wshadow \
		  -Wextra  \
		  -g
LFLAGS  = 

SRC := $(foreach ssrc, src, $(wildcard $(ssrc)/*.c))
OBJ := $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(LFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJ)
