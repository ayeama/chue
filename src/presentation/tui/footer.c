#include <stdlib.h>
#include <string.h>

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
        
        size_t len = strlen(f->command);
        if (len > 0) {
            wattrset(w, A_DIM);
            if (strncmp(f->command, "light", len) == 0) {
                mvwprintw(w, 0, (1 + len), "%s", "light" + len);
            } else if (strncmp(f->command, "room", len) == 0) {
                mvwprintw(w, 0, (1 + len), "%s", "room" + len);
            }
            wattrset(w, A_NORMAL);
        }
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