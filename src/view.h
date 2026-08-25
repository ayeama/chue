#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>

#include <ncurses/ncurses.h>
#include <ncurses/panel.h>

#include "hue.h"

typedef enum {
    VIEW_CONTENT_BRIDGES,
    VIEW_CONTENT_ROOMS,
    VIEW_CONTENT_LIGHTS,
} view_content_t;

typedef enum {
    VIEW_PANEL_DISCOVER,
} view_panel_t;

typedef struct {
    hue_bridge_t bridges[HUE_MAX_BRIDGES];
    size_t bridges_count;

    hue_room_t rooms[HUE_MAX_ROOMS];
    size_t rooms_count;

    hue_grouped_light_t grouped_lights[HUE_MAX_GROUPED_LIGHTS];
    size_t grouped_lights_count;

    hue_light_t lights[HUE_MAX_LIGHTS];
    size_t lights_count;

    size_t focused_row;

    WINDOW *window_header;
    WINDOW *window_footer;
    WINDOW *window_content_bridges;
    WINDOW *window_content_rooms;
    WINDOW *window_content_lights;
    view_content_t window_content_type;
    
    WINDOW *window_discover;
    PANEL *panel_discover;
    view_panel_t panel_type;
    bool panel_visible;
} view_t;

bool view_get_focused_grouped_light(view_t *view, hue_grouped_light_t **grouped_light);

void view_move_up(view_t *view);
void view_move_down(view_t *view);
void view_move_right(view_t *view);
void view_move_left(view_t *view);
void view_toggle_panel(view_t *view);

void view_render(view_t *view);
view_t *view_create();
void view_destroy(view_t *view);

#endif // VIEW_H
