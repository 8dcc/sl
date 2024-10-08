
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -ggdb3
LDLIBS=-lm

SRCS=main.c expr.c env.c lambda.c util.c debug.c read.c lexer.c parser.c eval.c prim_special.c prim_general.c prim_logic.c prim_type.c prim_list.c prim_string.c prim_arith.c prim_bitwise.c prim_io.c
OBJS=$(addprefix obj/, $(addsuffix .o, $(SRCS)))

BIN=sl
LIB=stdlib.lisp

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib/sl

#-------------------------------------------------------------------------------

.PHONY: all clean install install-bin install-lib doc

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

install: install-bin install-lib

install-bin: $(BIN)
	mkdir -p $(BINDIR)
	install -m 755 $^ $(BINDIR)

install-lib: $(LIB)
	mkdir -p $(LIBDIR)
	install -m 644 $^ $(LIBDIR)

doc:
	make --directory=doc clean all

#-------------------------------------------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<
