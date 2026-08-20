#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <ncurses/ncurses.h>
#include <ncurses/panel.h>

#include "view.h"

#define VIEW_HEADER_HEIGHT 4
#define VIEW_FOOTER_HEIGHT 1

static view_t view;

bool view_get_focused_grouped_light(view_t *view, hue_grouped_light_t **grouped_light) {
    if (view->focused_row >= view->rooms_count) {
        return false;
    }

    hue_room_t *room = &view->rooms[view->focused_row];

    for (size_t i = 0; i < view->grouped_lights_count; i++) {
        if (strcmp(room->id, view->grouped_lights[i].owner_id) == 0) {
            *grouped_light = &view->grouped_lights[i];
            return true;
        }
    }

    return false;
}

void view_move_up(view_t *view) {
    switch (view->window_content_type) {
    case VIEW_CONTENT_BRIDGES:
        break;
    case VIEW_CONTENT_ROOMS:
        if (view->focused_row > 0) {
            view->focused_row--;
        }
        break;
    case VIEW_CONTENT_LIGHTS:
        break;
    };
}

void view_move_down(view_t *view) {
    switch (view->window_content_type) {
    case VIEW_CONTENT_BRIDGES:
        break;
    case VIEW_CONTENT_ROOMS:
        if (view->rooms_count > 0 && (view->focused_row + 1) < view->rooms_count) {
            view->focused_row++;
        }
        break;
    case VIEW_CONTENT_LIGHTS:
        break;
    };
}

void view_move_right(view_t *view) {
    if (view->window_content_type < 2) {
        view->window_content_type += 1;
    }
}

void view_move_left(view_t *view) {
    if (view->window_content_type > 0) {
        view->window_content_type -= 1;
    }
}

void view_toggle_panel(view_t *view) {
    view->panel_visible = !view->panel_visible;
}

void render_header(view_t *view) {
    werase(view->window_header);

    int maxx = getmaxx(view->window_header);

    mvwprintw(view->window_header, 0, 0, "chue 0.0.1");
    mvwprintw(view->window_header, 1, 0, "bridge %s", view->bridges[0].id);

    mvwprintw(view->window_header, 0, (maxx - 17), "     _           ");
    mvwprintw(view->window_header, 1, (maxx - 17), " ___| |_ _ _ ___ ");
    mvwprintw(view->window_header, 2, (maxx - 17), "|  _|   | | | -_|");
    mvwprintw(view->window_header, 3, (maxx - 17), "|___|_|_|___|___|");

    wnoutrefresh(view->window_header);
}

void render_footer(view_t *view) {
    werase(view->window_footer);
    wnoutrefresh(view->window_footer);
}

void render_content_bridges(view_t *view) {
    werase(view->window_content_bridges);

    int maxy = getmaxy(view->window_content_bridges);
    int maxx = getmaxx(view->window_content_bridges);

    int table_header_y = 1;
    int table_header_x = 1;
    int table_content_y = table_header_y + 1;
    int table_content_x = table_header_x;

    box(view->window_content_bridges, 0, 0);
    char *title = " bridges(all)[1] ";
    mvwaddstr(
        view->window_content_bridges,
        0,
        ((maxx - strlen(title)) / 2),
        title
    );

    mvwprintw(
        view->window_content_bridges,
        table_header_y,
        table_header_x,
        "%-*s %-*s",
        HUE_BRIDGE_ID_LEN,
        "ID",
        HUE_NAME_LEN,
        "NAME"
    );

    for (size_t i = 0; i < view->bridges_count; i++) {
        if ((table_content_y + i) > (size_t)maxy) {
            break;
        }

        mvwprintw(
            view->window_content_bridges,
            (table_content_y + i),
            table_content_x,
            "%-*s %-*s",
            HUE_BRIDGE_ID_LEN,
            view->bridges[i].id,
            HUE_NAME_LEN,
            view->bridges[i].name
        );
    }

    wnoutrefresh(view->window_content_bridges);
}

void render_content_rooms(view_t *view) {
    werase(view->window_content_rooms);

    int maxx = getmaxx(view->window_content_rooms);
    int maxy = getmaxy(view->window_content_rooms);

    int table_header_y = 1;
    int table_header_x = 1;
    int table_content_y = table_header_y + 1;
    int table_content_x = table_header_x;

    box(view->window_content_rooms, 0, 0);
    char *title = " rooms(all)[6] ";
    mvwaddstr(
        view->window_content_rooms,
        0,
        ((maxx - strlen(title)) / 2),
        title
    );

    mvwprintw(
        view->window_content_rooms,
        table_header_y,
        table_header_x,
        "%-*s %s %s",
        HUE_NAME_LEN,
        "NAME",
        "STATE",
        "BRIGHTNESS"
    );

    for (size_t i = 0; i < view->rooms_count; i++) {
        if ((table_content_y + i) > (size_t)maxy) {
            break;
        }
        
        for (size_t j = 0; j < view->grouped_lights_count; j++) {
            if (strcmp(view->rooms[i].id, view->grouped_lights[j].owner_id) != 0) {
                continue;
            }

            if (i == view->focused_row) {
                wattrset(view->window_content_rooms, A_REVERSE);
            }

            mvwprintw(
                view->window_content_rooms,
                table_content_y + i,
                table_content_x,
                "%-*s %-5s %10.f",
                HUE_NAME_LEN,
                view->rooms[i].name,
                view->grouped_lights[j].on ? "on" : "off",
                (float)(view->grouped_lights[j].brightness / 100)
            );

            if (i == view->focused_row) {
                wattrset(view->window_content_rooms, A_NORMAL);
            }
        }
    }

    wnoutrefresh(view->window_content_rooms);
}

void render_content_lights(view_t *view) {
    werase(view->window_content_lights);

    int maxx = getmaxx(stdscr);

    box(view->window_content_lights, 0, 0);
    char *title = " lights(all)[0] ";
    mvwaddstr(
        view->window_content_lights,
        0,
        ((maxx - strlen(title)) / 2),
        title
    );

    wnoutrefresh(view->window_content_lights);
}

void render_content(view_t *view) {
    switch (view->window_content_type) {
    case VIEW_CONTENT_BRIDGES:
        render_content_bridges(view);
        break;
    case VIEW_CONTENT_ROOMS:
        render_content_rooms(view);
        break;
    case VIEW_CONTENT_LIGHTS:
        render_content_lights(view);
        break;
    default:
        break;
    }
}

void render_panel_discover(view_t *view) {
    werase(view->window_discover);

    int maxy = getmaxy(view->window_discover);
    int maxx = getmaxx(view->window_discover);

    box(view->window_discover, 0, 0);
    char *title = " discover ";
    mvwaddstr(
        view->window_discover,
        0,
        ((maxx - strlen(title)) / 2),
        title
    );

    mvwprintw(
        view->window_discover,
        1,
        1,
        "%-*s %-*s",
        HUE_BRIDGE_ID_LEN,
        "ID",
        HUE_NAME_LEN,
        "NAME"
    );

    for (size_t i = 0; i < view->bridges_count; i++) {
        if ((1 + i) > (size_t)maxy) {
            break;
        }

        mvwprintw(
            view->window_discover,
            (2 + i),
            1,
            "%-*s %-*s",
            HUE_BRIDGE_ID_LEN,
            view->bridges[i].id,
            HUE_NAME_LEN,
            view->bridges[i].name
        );
    }
}

void render_panel(view_t *view) {
    if (!view->panel_visible) {
        hide_panel(view->panel_discover);
        return;
    }

    show_panel(view->panel_discover);
    render_panel_discover(view);
}

void view_render(view_t *view) {
    render_header(view);
    render_footer(view);
    render_content(view);
    
    render_panel(view);
    update_panels();
    doupdate();
}

void create_header(view_t *view) {
    int height = VIEW_HEADER_HEIGHT;
    int width = getmaxx(stdscr);
    int y = 0;
    int x = 0;

    view->window_header = newwin(height, width, y, x);
    if (view->window_header == NULL) {
        return; // TODO handle error
    }
}

void create_footer(view_t *view) {
    int height = VIEW_FOOTER_HEIGHT;
    int width = getmaxx(stdscr);
    int y = getmaxy(stdscr) - height;
    int x = 0;

    view->window_footer = newwin(height, width, y, x);
    if (view->window_footer == NULL) {
        return; // TODO handle error
    }
}

void create_content_bridges(view_t *view) {
    int height = getmaxy(stdscr) - VIEW_HEADER_HEIGHT - VIEW_FOOTER_HEIGHT;
    int width = getmaxx(stdscr);
    int y = VIEW_HEADER_HEIGHT;
    int x = 0;

    view->window_content_bridges = newwin(height, width, y, x);
    if (view->window_content_bridges == NULL) {
        return; // TODO handle error
    }
}

void create_content_rooms(view_t *view) {
    int height = getmaxy(stdscr) - VIEW_HEADER_HEIGHT - VIEW_FOOTER_HEIGHT;
    int width = getmaxx(stdscr);
    int y = VIEW_HEADER_HEIGHT;
    int x = 0;

    view->window_content_rooms = newwin(height, width, y, x);
    if (view->window_content_rooms == NULL) {
        return; // TODO handle error
    }
}

void create_content_lights(view_t *view) {
    int height = getmaxy(stdscr) - VIEW_HEADER_HEIGHT - VIEW_FOOTER_HEIGHT;
    int width = getmaxx(stdscr);
    int y = VIEW_HEADER_HEIGHT;
    int x = 0;

    view->window_content_lights = newwin(height, width, y, x);
    if (view->window_content_lights == NULL) {
        return; // TODO handle error
    }
}

void create_content(view_t *view) {
    create_content_bridges(view);
    create_content_rooms(view);
    create_content_lights(view);
}

void create_panel_discover(view_t *view) {
    int maxy = getmaxy(stdscr);
    int maxx = getmaxx(stdscr);

    int height = (maxy / 4) * 3;
    int width = (maxx / 4) * 3;
    int y = (maxy - height) / 2;
    int x = (maxx - width) / 2;

    view->window_discover = newwin(height, width, y, x);
    if (view->window_discover == NULL) {
        return; // TODO handle error
    }

    view->panel_discover = new_panel(view->window_discover);
    if (view->panel_discover == NULL) {
        return; // TODO handle error
    }
}

void create_panel(view_t *view) {
    create_panel_discover(view);
}

view_t *view_create() {
    view = (view_t){0};
    view.focused_row = 0;
    
    view.window_content_type = VIEW_CONTENT_ROOMS;

    view.panel_type = VIEW_PANEL_DISCOVER;
    view.panel_visible = false;

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(10);

    create_header(&view);
    create_footer(&view);
    create_content(&view);

    create_panel(&view);

    return &view;
}

void destroy_header(view_t *view) {
    delwin(view->window_header);
}

void destroy_footer(view_t *view) {
    delwin(view->window_footer);
}

void destroy_content_bridges(view_t *view) {
    delwin(view->window_content_bridges);
}

void destroy_content_rooms(view_t *view) {
    delwin(view->window_content_rooms);
}

void destroy_content_lights(view_t *view) {
    delwin(view->window_content_lights);
}

void destroy_content(view_t *view) {
    destroy_content_bridges(view);
    destroy_content_rooms(view);
    destroy_content_lights(view);
}

void destroy_panel_discover(view_t *view) {
    delwin(view->window_discover);
}

void destroy_panel(view_t *view) {
    destroy_panel_discover(view);
    del_panel(view->panel_discover);
}

void view_destroy(view_t *view) {
    destroy_header(view);
    destroy_footer(view);
    destroy_content(view);

    destroy_panel(view);

    endwin();
}
