#include <stdio.h>
#include <stdlib.h>

#include <application/roomlist.h>

size_t roomlist_cols(void *data) {
    (void)data;
    return 1;
}

size_t roomlist_rows(void *data) {
    return ((RoomList *)data)->count;
}

void roomlist_title(void *data, char *buf, size_t size) {
    snprintf(buf, size, "%s[%ld]", "rooms", ((RoomList *)data)->count);
}

void roomlist_header(void *data, size_t col, char *buf, size_t size) {
    (void)data;

    if (col == 0) {
        snprintf(buf, size, "%s", "NAME");
    }
}

void roomlist_text(void *data, size_t col, size_t row, char *buf, size_t size) {
    Room *room = &(((RoomList *)data)->items[row]);

    if (col == 0) {
        snprintf(buf, size, "%s", room->name);
    }
}

void roomlist_select(void *data, size_t row) {
    (void)data;
    (void)row;
}
