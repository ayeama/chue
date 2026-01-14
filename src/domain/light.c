#include <stdlib.h>

#include <domain/light.h>

void light_free(Light *l) {
    free(l->name);
}
