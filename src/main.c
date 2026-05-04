#include <stdio.h>
#include <string.h>

#include <config/config.h>
#include <presentation/tui/tui.h>

#define VERSION "0.0.1"

int main(int argc, char *argv[]) {
    if (argc >= 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("chue\n");
                printf("Usage: chue [OPTION]...\n");
                printf("\n");
                printf("OPTIONS:\n");
                printf("-h, --help\tprint this help message\n");
                printf("-v, --version\tprint version info\n");
                return 0;
            }

            if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
                printf("chue %s\n", VERSION);
                return 0;
            }

            printf("unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (config_parse() != 0) {
        return 1;
    }

    tui();

    return 0;
}
