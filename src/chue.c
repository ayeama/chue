#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <ncurses/ncurses.h>

#include "config.h"
#include "hue.h"
#include "view.h"

typedef enum {
    CHUE_STATE_READY,
    CHUE_STATE_BRIDGE_DISCOVERING,
} state_t;

typedef struct {
    view_t *view;
    state_t state;
    config_t *config;

    bool running;
    uint8_t interval;
} chue_controller_t;

static chue_controller_t controller;

int create() {
    view_t *view = view_create();
    if (view == NULL) {
        return 1;
    }

    config_t *config = config_read();
    if (config == NULL) {
        return 1;
    }
    for (size_t i = 0; i < config->bridges_count; i++) {
        snprintf(
            view->bridges[i].id,
            sizeof(view->bridges[i].id),
            "%s",
            config->bridges[i].id
        );
        snprintf(
            view->bridges[i].internal_ip_address,
            sizeof(view->bridges[i].internal_ip_address),
            "%s",
            config->bridges[i].internal_ip_address
        );
        snprintf(
            view->bridges[i].name,
            sizeof(view->bridges[i].name),
            "%s",
            config->bridges[i].name
        );
        snprintf(
            view->bridges[i].key,
            sizeof(view->bridges[i].key),
            "%s",
            config->bridges[i].key
        );

        view->bridges[i].port = config->bridges[i].port;

        view->bridges_count += 1;
    }

    controller = (chue_controller_t){
        .view = view,
        .state = CHUE_STATE_READY,
        .config = config,
        .running = true,
        .interval = 1,
    };

    hue_code_t hresult = hue_create();
    if (hresult != HUE_CODE_OK) {
        return 1;
    }

    return 0;
}

int destroy() {
    view_destroy(controller.view);

    hue_code_t hresult = hue_destroy();
    if (hresult != HUE_CODE_OK) {
        return 1;
    }

    return 0;
}

void chue_controller_handle_brightness(chue_controller_t *controller, double brightness_delta) {    
    // TODO add rate limiting
    
    hue_grouped_light_t *grouped_light;
    if (!view_get_focused_grouped_light(controller->view, &grouped_light)) {
        return;
    }

    double brightness = grouped_light->brightness + brightness_delta;
    if (brightness < 0.0) {
        brightness = 0.0;
    } else if (brightness > 100.0) {
        brightness = 100.0;
    }

    // TODO handle error
    hue_grouped_light_put_one_brightness(
        &controller->view->bridges[0],
        grouped_light,
        brightness
    );

    // optimistic updating
    grouped_light->brightness = brightness;
}

void chue_controller_toggle_grouped_light(chue_controller_t *controller) {
    hue_grouped_light_t *grouped_light;
    if (!view_get_focused_grouped_light(controller->view, &grouped_light)) {
        return;
    }
    
    // TODO handle error
    hue_grouped_light_put_one(
        &controller->view->bridges[0],
        grouped_light
    );

    // optimistic updating
    grouped_light->on = !grouped_light->on;
}

void chue_controller_select(chue_controller_t *controller) {
    // TODO add rate limiting

    switch (controller->view->window_content_type) {
    case VIEW_CONTENT_BRIDGES:
        break;
    case VIEW_CONTENT_ROOMS:
        chue_controller_toggle_grouped_light(controller);
        break;
    case VIEW_CONTENT_LIGHTS:
        break;
    };
}

void chue_controller_handle_input(chue_controller_t *controller) {
    int ch = getch();

    switch (ch) {
    case 'j':
        view_move_down(controller->view);
        break;
    case 'k':
        view_move_up(controller->view);
        break;
    case 'h':
        view_move_left(controller->view);
        break;
    case 'l':
        view_move_right(controller->view);
        break;
    case 'H':
        chue_controller_handle_brightness(controller, -10.0);
        break;
    case 'L':
        chue_controller_handle_brightness(controller, 10.0);
        break;
    case ' ':
        chue_controller_select(controller);
        break;
    case 'd':
        view_toggle_panel(controller->view);
        // if ( && controller.config->bridges_count == 0) {
        //     // TODO handle error
        //     hue_discover(controller.view->bridges, &controller.view->bridges_count);
        // }

        // // TODO figure out a better flow for sync requests
        // for (size_t i = 0; i < controller.view->bridges_count; i++) {
        //     hue_config_get_one(&controller.view->bridges[i]);
        // }
        break;
    case 'q':
        controller->running = false;
        break;
    }
}

int main() {
    if (create() != 0) {
        return EXIT_FAILURE;
    }

    time_t start = time(NULL) - controller.interval;
    while (controller.running) {
        chue_controller_handle_input(&controller);

        time_t now = time(NULL);
        if (now >= (start + controller.interval)) {
            start = now;

            switch (controller.view->window_content_type) {
            case VIEW_CONTENT_BRIDGES:
                break;
            case VIEW_CONTENT_ROOMS:
                hue_code_t result = hue_room_get_many(
                    &controller.view->bridges[0],
                    controller.view->rooms,
                    &controller.view->rooms_count
                );
                if (result != HUE_CODE_OK) {
                    break;
                }
                result = hue_grouped_light_get_many(
                    &controller.view->bridges[0],
                    controller.view->grouped_lights,
                    &controller.view->grouped_lights_count
                );
                if (result != HUE_CODE_OK) {
                    break;
                }
                break;
            case VIEW_CONTENT_LIGHTS:
                break;
            }
        }

        hue_code_t result = hue_poll();
        if (result != HUE_CODE_OK) {
            break;
        }

        view_render(controller.view);
    }

    if (destroy() != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
