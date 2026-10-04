#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#include <hue.h>

typedef struct {
    hue_bridge_t bridges[HUE_MAX_BRIDGES];
    size_t bridges_count;
} config_t;

config_t *config_read();
void config_write();

#endif // CONFIG_H
