#ifndef CHUE_PRESENTATION_TUI_HEADER_h
#define CHUE_PRESENTATION_TUI_HEADER_h

#include <ncurses.h>

typedef struct HeaderBehavior {} HeaderBehavior;

typedef struct Header {
    void *data;
    HeaderBehavior *behavior;
} Header;

Header *header_create();

void header_free(Header *h);

void header_draw(WINDOW *w, Header *h);

void header_resize(WINDOW *w, Header *h);

#endif // CHUE_PRESENTATION_TUI_HEADER_H
