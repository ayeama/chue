CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc $(shell pkg-config --cflags ncurses panel libcurl libcjson)
LDFLAGS = $(shell pkg-config --libs ncurses panel libcurl libcjson)

BUILD = build
TARGET = $(BUILD)/chue

SRC = src/chue.c src/config.c src/hue.c src/view.c

all:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

install:
	sudo dnf install -y ncurses-devel libcurl-devel cjson-devel

clean:
	rm -rf $(BUILD)

run:
	$(TARGET)
