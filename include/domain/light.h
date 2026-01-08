#ifndef CHUE_DOMAIN_LIGHT_H
#define CHUE_DOMAIN_LIGHT_H

typedef struct Light {
    char id[37];
    char *name;
    bool on;
    double brightness;
} Light;

#endif  // CHUE_DOMAIN_LIGHT_H
