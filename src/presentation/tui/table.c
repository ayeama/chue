#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include <application/lightlist.h>
#include <presentation/tui/table.h>

TableBehavior *tablebehavior_create() {
    TableBehavior *b = calloc(1, sizeof(TableBehavior));
    if (b == NULL) {
        return NULL;
    }

    return b;
}

void tablebehavior_free(TableBehavior *b) {
    free(b);
}

Table *table_create(TableBehavior *b, void *data) {
    Table *t = calloc(1, sizeof(Table));
    if (t == NULL) {
        return NULL;
    }

    t->behavior = b;
    t->data = data;
    t->sindex = 0;
    t->windex = 0;
    t->wsize = 0;

    return t;
}

void table_free(Table *t) {
    free(t);
}

void table_draw(WINDOW *w, Table *t) {
    size_t cols = t->behavior->cols(t->data);
    size_t rows = t->behavior->rows(t->data);

    // size_t maxy = getmaxy(w);
    size_t maxx = getmaxx(w);

    size_t colwidth = (maxx - 2) / cols;

    box(w, 0, 0);

    char title[maxx - 2];
    t->behavior->title(t->data, title, sizeof title);
    // TODO append filter to title
    mvwprintw(w, 0, ((maxx - strlen(title)) / 2), " %s ", title);

    wmove(w, 1, 1);

    char buf[64];
    for (size_t col = 0; col < cols; col++) {
        t->behavior->header(t->data, col, buf, sizeof buf);
        wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
    }

    for (size_t row = 0; (row < rows) && (row < (t->wsize)); row++) {
        wmove(w, (row + 2), 1);
        
        if (row == t->sindex) {
            mvwhline(w, (row + 2), 1, ' ', (maxx - 2));
        }

        for (size_t col = 0; col < cols; col++) {
            t->behavior->text(t->data, col, (row + t->windex), buf, sizeof buf);
            
            if ((row + t->windex) == t->sindex) {
                wattrset(w, A_REVERSE);
                wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
                wattrset(w, A_NORMAL);
            } else {
                wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
            }
        }
    }
}

void table_clear(WINDOW *w, Table *t) {
    (void)t;

    wclear(w);
}

void table_resize(WINDOW *w, Table *t) {
    t->wsize = getmaxy(w) - 3;
}

void table_up(Table *t) {
    size_t wmargin = 4;
    size_t ls = ((LightList *)t->data)->count;

    if (t->windex > 0) {
        if (t->sindex > (t->windex + (wmargin - 1))) {
            t->sindex--;
        } else {
            t->sindex--;
            t->windex--;
        }
    } else {
        if (t->sindex > 0) {
            t->sindex--;
        } else {
            t->sindex = (ls - 1);

            if (t->wsize < ls) {
                t->windex = (ls - t->wsize);
            }
        }
    }
}

void table_down(Table *t) {
    size_t wmargin = 4;
    size_t ls = ((LightList *)t->data)->count;

    if ((t->windex + t->wsize) < ls) {
        if (t->sindex < (t->windex + t->wsize - wmargin)) {
            t->sindex++;
        } else {
            t->sindex++;
            t->windex++;
        }
    } else {
        if (t->sindex < (ls - 1)) {
            t->sindex++;
        } else {
            t->sindex = 0;
            t->windex = 0;
        }
    }
}

void table_top(Table *t) {
    t->sindex = 0;
    t->windex = 0;
}

void table_bottom(Table *t) {
    size_t ls = ((LightList *)t->data)->count;
    t->sindex = (ls - 1);
    if (ls > t->wsize) {
        t->windex = (ls - t->wsize);
    }
}

void table_select(Table *t) {
    t->behavior->select(t->data, t->sindex);
}
