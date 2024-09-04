
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -ggdb3
LDLIBS=-lm

OBJ_FILES=main.c.o expr.c.o env.c.o lambda.c.o util.c.o read.c.o lexer.c.o parser.c.o eval.c.o primitives.c.o
OBJS=$(addprefix obj/, $(OBJ_FILES))

BIN=sl
INSTALL_DIR=/usr/local/bin

#-------------------------------------------------------------------------------

.PHONY: all clean install

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

install: $(BIN)
	mkdir -p $(INSTALL_DIR)
	install -m 755 $^ $(INSTALL_DIR)

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
