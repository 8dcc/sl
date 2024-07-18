
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -ggdb3
LDFLAGS=-lm

OBJ_FILES=main.c.o util.c.o lexer.c.o parser.c.o expr.c.o eval.c.o primitives.c.o env.c.o
OBJS=$(addprefix obj/, $(OBJ_FILES))

BIN=sl

#-------------------------------------------------------------------------------

.PHONY: clean all

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
