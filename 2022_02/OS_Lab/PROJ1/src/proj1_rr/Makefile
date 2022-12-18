# A Makefile that I normally use to build single directory projects.

# redefine all and clean
.PHONY: all clean
# set object file directory
OBJ_DIR = obj
# set source code directory
SRC_DIR = ./

# define gcc as CC.
CC = gcc
# set CFLAGS, this will automatically tell gcc to look up for directories with depth of 1.
CFLAGS = -O2 -Wall -std=gnu99 $(addprefix -I,$(shell find $(SRC_DIR) -maxdepth 1 -type d))
# set LDFLAGS
LDFLAGS = -lpthread

# set project name as proj1
PROG = proj1

# look for *.c files in source directories, the save it as SRC.
SRC = $(shell find ./ -maxdepth 1 -name '*.c')
# with that SRC, replace *.c into *.o without directory names.
OBJ = $(patsubst %.c, %.o, $(notdir $(SRC)))

# get all *.o files into obj directory.
vpath %.o $(OBJ_DIR)

# recipe for make all.
all: $(PROG)

# recipe for debug, basically add -DDEBUG option when compiling.
debug: CFLAGS += -DDEBUG
debug: $(PROG)

# $(PROG) will need all $(OBJ) files as prerequisites
$(PROG): $(addprefix $(OBJ_DIR)/, $(OBJ))
	# Compile $(PROG) with all *.o files.
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile all obj/*.o using *.c
$(OBJ_DIR)/%.o: %.c
	# Make directory for obj
	mkdir -p $(@D)
	# Compile object file using *.c file.
	$(CC) $(CFLAGS) -o $@ -c $<

# recipe for clean
clean:
	rm -rf $(PROG) $(OBJ_DIR)