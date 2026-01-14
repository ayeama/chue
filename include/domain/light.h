#ifndef CHUE_DOMAIN_LIGHT_H
#define CHUE_DOMAIN_LIGHT_H

#include <stdbool.h>

typedef struct Light {
    char id[37];
    char *name;
    bool on;
    double brightness;
} Light;

void light_free(Light *l);

#endif  // CHUE_DOMAIN_LIGHT_H
