#include <stdlib.h>

#include <presentation/tui/header.h>

Header *header_create() {
    Header *h = calloc(1, sizeof(Header));
    if (h == NULL) {
        return NULL;
    }

    return h;
}

void header_free(Header *h) {
    free(h);
}

void header_draw(WINDOW *w, Header *h) {
    (void)h;

    size_t maxx = getmaxx(w);

    mvwprintw(w, 0, 0, "chue 0.0.1");

    mvwprintw(w, 0, (maxx - 17), "     _           ");
    mvwprintw(w, 1, (maxx - 17), " ___| |_ _ _ ___ ");
    mvwprintw(w, 2, (maxx - 17), "|  _|   | | | -_|");
    mvwprintw(w, 3, (maxx - 17), "|___|_|_|___|___|");
}

void header_resize(WINDOW *w, Header *h) {
    (void)w;
    (void)h;
}
