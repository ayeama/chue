#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cjson/cJSON.h>

#include <config/config.h>

Config config = {0};

int config_parse() {
    const char *path_home = getenv("HOME");
    if (path_home == NULL) {
        return 1;
    }

    char path_config[512];
    snprintf(path_config, sizeof(path_config), "%s/.config/chue/chue.json", path_home);

    FILE *file_config = fopen(path_config, "r");
    if (file_config == NULL) {
        return 1;
    }

    fseek(file_config, 0, SEEK_END);
    long config_length = ftell(file_config);
    fseek(file_config, 0, SEEK_SET);

    char *buf = malloc(config_length + 1);
    if (buf) {
        fread(buf, 1, config_length, file_config);
        buf[config_length] = '\0';
    }

    fclose(file_config);

    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        fprintf(stderr, "cjson parse error");
    }

    cJSON *host = cJSON_GetObjectItem(json, "host");
    if (cJSON_IsString(host)) {
        strncpy(config.host, host->valuestring, 256-1);
    }

    cJSON_Delete(json);
    free(buf);

    return 0;
}
