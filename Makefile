CC = gcc
CFLAGS = -lpthread -lm

DIR_SRC = ./src
DIR_BUILD = ./bin

SRC = $(wildcard ${DIR_SRC}/*.c)
OBJ = $(patsubst %.c,${DIR_BUILD}/%.o,$(notdir ${SRC}))

TARGET = $(patsubst %.c,${DIR_BUILD}/% ,$(notdir ${SRC}))


.PHONY: all clean

#build programs
all: $(DIR_BUILD) $(TARGET)

$(DIR_BUILD):
	mkdir $(DIR_BUILD)

${DIR_BUILD}/% :${DIR_BUILD}/%.o
	$(CC) $(CFLAGS) $< -o $@

${DIR_BUILD}/%.o: ${DIR_SRC}/%.c
	$(CC) -c $< -o $@

# Clean up build products.
clean:
	rm -r -f $(DIR_BUILD) *.txt


