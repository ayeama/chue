#include <stdio.h>

#include "chue.h"
#include "config.h"

int main() {
    Config *config = config_read();
    if (config == NULL) {
        fprintf(stderr, "error reading config\n");
        return 1;
    }

    State state = {
        .config = config,
    };

    return 0;
}
