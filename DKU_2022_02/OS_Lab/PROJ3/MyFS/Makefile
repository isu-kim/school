.PHONY: all clean  # redefine all and clean

# set object file directory
OBJ_DIR = obj
# set source code directory
SRC_DIR = ./

CC = gcc  # define gcc as CC.
CFLAGS = -O2 -Wall -std=gnu99 $(addprefix -I,$(shell find $(SRC_DIR) -maxdepth 1 -type d)) # set CFLAGS, this will include directories.
LDFLAGS = -lpthread # set LDFLAGS

PROG = MyFS  # set program name as stats_monitor

SRC = $(shell find ./ -maxdepth 1 -name '*.c')  # look for *.c files in source directories, the save it as SRC.
OBJ = $(patsubst %.c, %.o, $(notdir $(SRC)))  # with that SRC, replace *.c into *.o without directory names.

vpath %.o $(OBJ_DIR)  # get all *.o files into obj directory.

all: $(PROG)  # recipe for make all.

debug: CFLAGS += -DDEBUG  # recipe for debug
debug: $(PROG)

$(PROG): $(addprefix $(OBJ_DIR)/, $(OBJ))  # $(PROG) will need all $(OBJ) files as prerequisites
	$(CC) -o $@ $^ $(LDFLAGS)  # Compile $(PROG) with all *.o files.

$(OBJ_DIR)/%.o: %.c  # Generate all obj/*.o using *.c
	mkdir -p $(@D)  # Make directory for obj
	$(CC) $(CFLAGS) -o $@ -c $<  # Compile object file using *.c file.

clean:  # recipe for clean
	rm -rf $(PROG) $(OBJ_DIR)