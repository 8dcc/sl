
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -ggdb3
LDLIBS=-lm

# TODO: Use addsuffix and SRCS
OBJ_FILES=main.c.o expr.c.o env.c.o lambda.c.o util.c.o read.c.o lexer.c.o parser.c.o eval.c.o prim_special.c.o prim_general.c.o prim_type.c.o prim_list.c.o prim_string.c.o prim_arith.c.o prim_bitwise.c.o prim_logic.c.o
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
