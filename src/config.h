#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char *bridge_address;
    char *bridge_key;
} Config;

Config* config_read();

#endif // CONFIG_H
