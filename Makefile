
CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=-lm

OBJ_FILES=main.c.o util.c.o lexer.c.o parser.c.o
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
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

