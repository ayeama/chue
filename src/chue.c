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

void quit() {
    if (destroy() == 0) {
        exit(0);
    } else {
        exit(1);
    }
}

void chue_controller_handle_input() {
    int ch = getch();

    switch (ch) {
    case 'j':
        if (controller.view->selected_row_index < 16) {
            controller.view->selected_row_index += 1;
        }
        break;
    case 'k':
        if (controller.view->selected_row_index > 0) {
            controller.view->selected_row_index -= 1;
        }
        break;
    case 'h':
        controller.view->window_content_type = VIEW_CONTENT_ROOMS;
        break;
    case 'l':
        controller.view->window_content_type = VIEW_CONTENT_LIGHTS;
        break;
    case ' ':
        // TODO handle error
        hue_grouped_light_put_one(
            &controller.view->bridges[0],
            &controller.view->grouped_lights[controller.view->selected_row_grouped_light_index]
        );
        break;
    case 'd':
        controller.view->panel_visible = !controller.view->panel_visible;
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
        quit();
    }
}

int main() {
    if (create() != 0) {
        return EXIT_FAILURE;
    }

    time_t start = time(NULL);
    while (true) {
        chue_controller_handle_input();

        time_t now = time(NULL);
        if (now >= (start + 1)) {
            start = now;

            switch (controller.view->window_content_type) {
            case VIEW_CONTENT_ROOMS:
                hue_code_t hresult = hue_room_get_many(
                    &controller.view->bridges[0],
                    controller.view->rooms,
                    &controller.view->rooms_count
                );
                if (hresult != HUE_CODE_OK) {
                    break;
                }
                hresult = hue_grouped_light_get_many(
                    &controller.view->bridges[0],
                    controller.view->grouped_lights,
                    &controller.view->grouped_lights_count
                );
                if (hresult != HUE_CODE_OK) {
                    break;
                }
                break;
            case VIEW_CONTENT_LIGHTS:
                break;
            }
        }

        hue_code_t hresult = hue_poll();
        if (hresult != HUE_CODE_OK) {
            break;
        }

        view_render(controller.view);
    }

    return EXIT_SUCCESS;
}
