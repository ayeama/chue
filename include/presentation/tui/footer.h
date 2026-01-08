#ifndef CHUE_PRESENTATION_TUI_FOOTER_h
#define CHUE_PRESENTATION_TUI_FOOTER_h

#include <ncurses.h>

typedef struct FooterBehavior {} FooterBehavior;

typedef struct Footer {
    void *data;
    FooterBehavior *behavior;
} Footer;

Footer *footer_create();

void footer_free(Footer *f);

void footer_draw(WINDOW *w, Footer *f);

void footer_resize(WINDOW *w, Footer *f);

#endif // CHUE_PRESENTATION_TUI_FOOTER_H
