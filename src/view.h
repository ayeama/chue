#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>

#include <ncurses/ncurses.h>
#include <ncurses/panel.h>

#include "hue.h"

typedef enum {
    VIEW_CONTENT_ROOMS,
    VIEW_CONTENT_LIGHTS,
} view_content_t;

typedef enum {
    VIEW_PANEL_DISCOVER,
} view_panel_t;

typedef struct {
    hue_bridge_t bridges[4];
    size_t bridges_count;

    hue_room_t rooms[16];
    size_t rooms_count;

    hue_grouped_light_t grouped_lights[16];
    size_t grouped_lights_count;

    size_t selected_row_index;
    size_t selected_row_grouped_light_index;

    WINDOW *window_header;
    WINDOW *window_footer;
    WINDOW *window_content_rooms;
    WINDOW *window_content_lights;
    view_content_t window_content_type;
    
    WINDOW *window_discover;
    PANEL *panel_discover;
    view_panel_t panel_type;
    bool panel_visible;
} view_t;

void view_render(view_t *view);
view_t *view_create();
void view_destroy(view_t *view);

#endif // VIEW_H
