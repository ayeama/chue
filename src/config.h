#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#include <hue.h>

typedef struct {
    hue_bridge_t bridges[4];
    size_t bridges_count;
} config_t;

config_t *config_read();
void config_write(config_t *config);

#endif // CONFIG_H
