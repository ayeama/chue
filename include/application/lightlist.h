#ifndef CHUE_APPLICATION_LIGHTLIST_H
#define CHUE_APPLICATION_LIGHTLIST_H

#include <stdlib.h>

#include <domain/light.h>

typedef struct LightList {
    Light *items;
    size_t count;
} LightList;

size_t lightlist_cols(void *data);

size_t lightlist_rows(void *data);

void lightlist_header(void *data, size_t vol, char *buf, size_t size);

void lightlist_text(void *data, size_t col, size_t row, char *buf, size_t size);

void lightlist_select(void *data, size_t row);

#endif  // CHUE_APPLICATION_LIGHTLIST_H
