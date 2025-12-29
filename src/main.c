#include <string.h>

#include <ncurses.h>

static WINDOW *wheader = NULL;
static WINDOW *wcontent = NULL;

void draw_header() {
    if (wheader == NULL) {
        int starty = 0;
        int startx = 0;
        int height = 4;
        int width = COLS;
        wheader = newwin(height, width, starty, startx);
    }
    
    mvwprintw(wheader, 0, 0, "0.0.1");
    
    mvwprintw(wheader, 0, COLS-17, "     _           ");
    mvwprintw(wheader, 1, COLS-17, " ___| |_ _ _ ___ ");
    mvwprintw(wheader, 2, COLS-17, "|  _|   | | | -_|");
    mvwprintw(wheader, 3, COLS-17, "|___|_|_|___|___|");

    wnoutrefresh(wheader);
}

void draw_content() {
    if (wcontent == NULL) {
        int starty = 4;
        int startx = 0;
        int height = LINES - 4;
        int width = COLS;
        wcontent = newwin(height, width, starty, startx);
    }
    
    /* title */
    wattrset(wcontent, COLOR_PAIR(1));
    box(wcontent, 0, 0);
    wattrset(wcontent, A_NORMAL);

    char title[COLS - 2];
    sprintf(title, "%s(%s)[%d]", "lights", "all", 0);
    mvwprintw(wcontent, 0, ((getmaxx(wcontent) - strlen(title)) / 2), " %s ", title);

    /* table header */
    char *header[] = {"NAME", "STATE", "PALETTE"};
    int header_count = 3;
    int header_width = (getmaxx(wcontent) - 2) / header_count;

    wmove(wcontent, 1, 1);
    for (int i = 0; i < header_count; i++) {
        wprintw(wcontent, "%-*s", header_width, header[i]);
    }

    wnoutrefresh(wcontent);
}

void draw() {
    refresh();
    draw_header();
    draw_content();

    doupdate();
}

void loop() {
    draw();

    int ch;
    while ((ch = getch()) != ERR) {
        switch (ch) {
            case 'q':
                return;
            case 'h':
                break;
            case 'j':
                break;
            case 'k':
                break;
            case 'l':
                break;
            case KEY_RESIZE:
                break;
            default:
                break;
        }

        draw();
    }
}


int ncurses_init() {
    initscr();
    raw();
    noecho();
    curs_set(0);

    if (!has_colors()) {
        fprintf(stderr, "colors not supported");
        return 1;
    }
    
    if (!can_change_color()) {
        fprintf(stderr, "changing colors not supported");
        return 1;
    }

    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);

    return 0;
}

void ncurses_end() {
    if (wheader != NULL) {
        delwin(wheader);
    }

    endwin();
}

int main() {
    int error = ncurses_init();
    if (error != 0) {
        ncurses_end();
        return error;
    }

    loop();
    ncurses_end();

    return 0;
}
