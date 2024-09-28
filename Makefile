
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -ggdb3
LDLIBS=-lm

SRCS=main.c expr.c env.c lambda.c util.c read.c lexer.c parser.c eval.c prim_special.c prim_general.c prim_type.c prim_list.c prim_string.c prim_arith.c prim_bitwise.c prim_logic.c
OBJS=$(addprefix obj/, $(addsuffix .o, $(SRCS)))

BIN=sl
INSTALL_DIR=/usr/local/bin

#-------------------------------------------------------------------------------

.PHONY: all clean install doc

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

install: $(BIN)
	mkdir -p $(INSTALL_DIR)
	install -m 755 $^ $(INSTALL_DIR)

doc:
	make --directory=doc clean all

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
