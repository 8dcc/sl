
CC=gcc
CPPFLAGS=-DSL_DEBUG_MAX_CALLSTACK=10000

# The project uses the 'c11' standard because it uses '_Static_assert', but it
# can be compiled with 'gnu99', or the 'SL_STATIC_ASSERT' macro could be
# modified.
#
# We don't include '-Wpedantic' because it warns about passing a function
# pointer to a "%p" format specifier.
CFLAGS=-std=c11 -Wall -Wextra -Wshadow -ggdb3

LDLIBS=-lm

SRC=main.c \
    env.c expr.c expr_pool.c lambda.c \
    util.c memory.c garbage_collector.c error.c debug.c \
    cmdargs.c read.c lexer.c parser.c eval.c \
    prim_special.c prim_general.c prim_logic.c prim_type.c prim_list.c \
    prim_string.c prim_arith.c prim_bitwise.c prim_io.c
OBJ=$(addprefix obj/, $(addsuffix .o, $(SRC)))

BIN=sl
LIB=stdlib.lisp

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib/sl

#-------------------------------------------------------------------------------

.PHONY: all clean install install-bin install-lib doc

all: $(BIN)

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

install: install-bin install-lib

install-bin: $(BIN)
	install -D -m 755 $^ -t $(DESTDIR)$(BINDIR)

install-lib: $(LIB)
	install -D -m 644 $^ -t $(DESTDIR)$(LIBDIR)

doc:
	make --directory=doc clean all

#-------------------------------------------------------------------------------

$(BIN): $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
