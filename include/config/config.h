#ifndef CHUE_CONFIG_H
#define CHUE_CONFIG_H

typedef struct Config {
    char host[256];
} Config;

extern Config config;

int config_parse();

#endif  // CHUE_CONFIG_H
