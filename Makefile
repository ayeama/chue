CC = gcc
CFLAGS = -Wall -Wextra -std=c23 -Isrc $(shell pkg-config --cflags libcjson)
LDFLAGS = $(shell pkg-config --libs libcjson)

BUILD = build
TARGET = $(BUILD)/chue

SRC = src/main.c src/config.c

all:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

install:
	sudo dnf install -y cjson-devel

clean:
	rm -rf $(BUILD)
