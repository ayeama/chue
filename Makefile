CC      := cc
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
BIN     := build/chue

CFLAGS  := -Wall -Wextra -std=c23
LDFLAGS := -lncurses -lcurl -lcjson #-lsqlite3

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

run:
	./build/chue

clean:
	rm -rf build
