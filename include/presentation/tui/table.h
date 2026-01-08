#ifndef CHUE_PRESENTATION_TUI_TABLE_H
#define CHUE_PRESENTATION_TUI_TABLE_H

#include <stdlib.h>

#include <ncurses.h>

typedef struct TableBehavior {
    size_t (*cols)(void *data);
    size_t (*rows)(void *data);
    void (*header)(void *data, size_t col, char *buf, size_t size);
    void (*text)(void *data, size_t col, size_t row, char *buf, size_t size);
    void (*select)(void *data, size_t row);
} TableBehavior;

TableBehavior *tablebehavior_create();

void tablebehavior_free(TableBehavior *b);

typedef struct Table {
    void *data;
    TableBehavior *behavior;

    size_t sindex;
    size_t windex;
    size_t wsize;
} Table;

Table *table_create(TableBehavior *b, void *data);

void table_free(Table *t);

void table_draw(WINDOW *w, Table *t);

void table_resize(WINDOW *w, Table *t);

void table_up(Table *t);

void table_down(Table *t);

void table_top(Table *t);

void table_bottom(Table *t);

void table_select(Table *t);

#endif  // CHUE_PRESENTATION_TUI_TABLE_H
