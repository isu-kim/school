.PHONY: all clean

CC = gcc
CFLAGS = -O2 -Wall -std=gnu99

PROG = mipsim

SRC = $(shell find . -maxdepth 1 -name '*.c')
OBJ = $(patsubst %.c, %.o, $(SRC))

all: $(PROG)

debug: CFLAGS += -DDEBUG
debug: $(PROG)

$(PROG): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(PROG) $(OBJ)
	rm -rf mipsim