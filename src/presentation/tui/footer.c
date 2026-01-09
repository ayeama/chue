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

    if (f->command != NULL) {
        mvwprintw(w, 0, 0, ":%s", f->command);
    }
}

void footer_resize(WINDOW *w, Footer *f) {
    (void)w;
    (void)f;
}

// TODO tmp wrong location
void footer_command_clear(WINDOW *w, Footer *f) {
    free(f->command);
    f->command = NULL;
    wclear(w);
}