CC=gcc
CFLAGS= -g -Wall -Wextra -Wpedantic
OBJS= chip8.c
LINKER_FLAGS = -lSDL2 -lncurses
OBJ_NAME = chip8 

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LINKER_FLAGS) -o $(OBJ_NAME)

clean:
	rm $(OBJ_NAME)