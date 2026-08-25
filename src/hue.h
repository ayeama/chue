#ifndef HUE_H
#define HUE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HUE_MAX_BRIDGES 4
#define HUE_MAX_ROOMS 16
#define HUE_MAX_GROUPED_LIGHTS 16
#define HUE_MAX_LIGHTS 16

#define HUE_ID_LEN 36
#define HUE_ID_SIZE (HUE_ID_LEN + 1)
#define HUE_NAME_LEN 32
#define HUE_NAME_SIZE (HUE_NAME_LEN + 1)
#define HUE_BRIDGE_ID_LEN 16
#define HUE_BRIDGE_ID_SIZE (HUE_BRIDGE_ID_LEN + 1)
#define HUE_BRIDGE_INTERNAL_IP_LEN 15
#define HUE_BRIDGE_INTERNAL_IP_SIZE (HUE_BRIDGE_INTERNAL_IP_LEN + 1)
#define HUE_BRIDGE_KEY_LEN 40
#define HUE_BRIDGE_KEY_SIZE (HUE_BRIDGE_KEY_LEN + 1)

typedef struct {
    char id[HUE_BRIDGE_ID_SIZE];
    char internal_ip_address[HUE_BRIDGE_INTERNAL_IP_SIZE];
    uint16_t port;

    char name[HUE_NAME_SIZE];
    char key[HUE_BRIDGE_KEY_SIZE];
} hue_bridge_t;

typedef struct {
    char id[HUE_ID_SIZE];
    char name[HUE_NAME_SIZE];
} hue_room_t;

typedef struct {
    char id[HUE_ID_SIZE];
    char owner_id[HUE_ID_SIZE];
    char owner_type[HUE_NAME_SIZE];
    double brightness;
    bool on;
} hue_grouped_light_t;

typedef struct {
    char id[HUE_ID_SIZE];
    char name[HUE_NAME_SIZE];
    char archetype[33]; // TODO magic number
    double brightness;
    bool on;
} hue_light_t;

typedef enum {
    HUE_CODE_OK = 0,
    HUE_CODE_ERROR,
} hue_code_t;

hue_code_t hue_room_get_many(hue_bridge_t *bridge, hue_room_t *rooms, size_t *rooms_count);
hue_code_t hue_grouped_light_get_many(hue_bridge_t *bridge, hue_grouped_light_t *grouped_lights, size_t *grouped_lights_count);
hue_code_t hue_grouped_light_put_one(hue_bridge_t *bridge, hue_grouped_light_t *grouped_light);
hue_code_t hue_grouped_light_put_one_brightness(hue_bridge_t *bridge, hue_grouped_light_t *grouped_light, double brightness);
hue_code_t hue_light_get_many(hue_bridge_t *bridge, hue_light_t *lights, size_t *lights_count);
hue_code_t hue_discover(hue_bridge_t *bridges, size_t *bridges_count);
hue_code_t hue_config_get_one(hue_bridge_t *bridge);
hue_code_t hue_poll();
hue_code_t hue_create();
hue_code_t hue_destroy();

#endif // HUE_H
