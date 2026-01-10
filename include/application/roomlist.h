#ifndef CHUE_APPLICATION_ROOMLIST_H
#define CHUE_APPLICATION_ROOMLIST_H

#include <stdio.h>

#include <domain/room.h>

typedef struct RoomList {
    Room *items;
    size_t count;
} RoomList;

size_t roomlist_cols(void *data);

size_t roomlist_rows(void *data);

void roomlist_title(void *data, char *buf, size_t size);

void roomlist_header(void *data, size_t col, char *buf, size_t size);

void roomlist_text(void *data, size_t col, size_t row, char *buf, size_t size);

void roomlist_select(void *data, size_t row);

#endif // CHUE_APPLICATION_ROOMLIST_H
