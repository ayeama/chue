#include <stdio.h>
#include <stdlib.h>

#include <application/lightlist.h>

size_t lightlist_cols(void *data) {
    (void)data;
    return 3;
}

size_t lightlist_rows(void *data) {
    return ((LightList *)data)->count;
}

void lightlist_title(void *data, char *buf, size_t size) {
    snprintf(buf, size, "%s[%ld]", "lights", ((LightList *)data)->count);
}

void lightlist_header(void *data, size_t col, char *buf, size_t size) {
    (void)data;

    if (col == 0) {
        snprintf(buf, size, "%s", "NAME");
    } else if (col == 1) {
        snprintf(buf, size, "%s", "STATE");
    } else if (col == 2) {
        snprintf(buf, size, "%s", "BRIGHTNESS");
    }
}

void lightlist_text(void *data, size_t col, size_t row, char *buf, size_t size) {
    Light *light = &(((LightList *)data)->items[row]);

    if (col == 0) {
        snprintf(buf, size, "%s", light->name);
    } else if (col == 1) {
        snprintf(buf, size, "%s", light->on ? "on" : "off");
    } else if (col == 2) {
        snprintf(buf, size, "%*.0lf%%", 3, light->brightness);
    }
}

void lightlist_select(void *data, size_t row) {
    (void)data;
    (void)row;

    // Light *light = &(((LightList *)data)->items[row]);
    // curl_hue_light_toggle();
}
