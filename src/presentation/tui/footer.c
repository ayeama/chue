#include <stdlib.h>

#include <presentation/tui/footer.h>

Footer *footer_create() {
    Footer *f = calloc(1, sizeof(Footer));
    if (f == NULL) {
        return NULL;
    }

    return f;
}

void footer_free(Footer *f) {
    free(f);
}

void footer_draw(WINDOW *w, Footer *f) {
    (void)f;

    mvwprintw(w, 0, 0, ":quit");
}

void footer_resize(WINDOW *w, Footer *f) {
    (void)w;
    (void)f;
}
