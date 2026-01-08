CC := cc

SRC := $(shell find src -type f -name '*.c')
OBJ := $(SRC:src/%.c=build/%.o)

BIN := build/chue

CFLAGS  := -Wall -Wextra -std=c23 -Iinclude
LDFLAGS := -lncurses -lcurl -lcjson #-lsqlite3

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run:
	./$(BIN)

clean:
	rm -rf build
