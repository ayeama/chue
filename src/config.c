#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "config.h"

static const char *config_path() {
    const char *path_override = getenv("CHUE_CONFIG");
    if (path_override != NULL) {
        return path_override;
    }

    const char *home = getenv("HOME");
    if (home == NULL) {
        return NULL;
    }

    static char path[1024];
    snprintf(path, sizeof(path), "%s/.config/chue.json", home);

    return path;
}

char *config_serialise(Config *config) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    cJSON_AddStringToObject(root, "bridge_address", config->bridge_address);
    cJSON_AddStringToObject(root, "bridge_key", config->bridge_key);

    char *config_serialised = cJSON_PrintUnformatted(root);
    if (config_serialised == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);

    return config_serialised;
}

Config *config_deserialise(const char *json) {
    Config *config = malloc(sizeof(Config));
    if (config == NULL) {
        fprintf(stderr, "error allocating config\n");
        return NULL;
    }

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        fprintf(stderr, "error parsing config");
        return NULL;
    }

    cJSON *bridge_address = cJSON_GetObjectItemCaseSensitive(root, "bridge_address");
    cJSON *bridge_key = cJSON_GetObjectItemCaseSensitive(root, "bridge_key");

    if (!cJSON_IsString(bridge_address) || !cJSON_IsString(bridge_key)) {
        free(config);
        cJSON_Delete(root);
        return NULL;
    }

    config->bridge_address = strdup(bridge_address->valuestring);
    config->bridge_key = strdup(bridge_key->valuestring);

    cJSON_Delete(root);

    return config;
}

Config *config_read() {
    const char *path = config_path();
    if (path == NULL) {
        fprintf(stderr, "error getting config path\n");
        return NULL;
    }
    
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        Config *config = malloc(sizeof(Config));
        if (config == NULL) {
            fprintf(stderr, "error allocating config\n");
            return NULL;
        }

        config->bridge_address = strdup("");
        config->bridge_key = strdup("");

        file = fopen(path, "w");
        if (file == NULL) {
            fprintf(stderr, "error opening config for writing\n");
            free(config);
            return NULL;
        }

        char *json = config_serialise(config);
        fputs(json, file);
        free(json);

        return config;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0) {
        fprintf(stderr, "error getting config file size\n");
        return NULL;
    }

    char *json = malloc(size + 1);
    if (json == NULL) {
        fprintf(stderr, "error allocating config buffer\n");
        return NULL;
    }

    size_t read = fread(json, 1, size, file);
    json[read] = '\0';
    fclose(file);
    
    Config *config = config_deserialise(json);
    free(json);

    return config;
}
