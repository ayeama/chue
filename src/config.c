#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "config.h"

#define CONFIG_PATH_LEN 1024
#define CONFIG_JSON_LEN 1024

static config_t config = {0};

// TODO make config path a static variable?
static const char *config_path() {
    const char *path_override = getenv("CHUE_CONFIG");
    if (path_override != NULL) {
        return path_override;
    }

    const char *home = getenv("HOME");
    if (home == NULL) {
        return NULL;
    }

    static char path[CONFIG_PATH_LEN];
    snprintf(path, sizeof(path), "%s/.config/chue.json", home);

    return path;
}

void config_serialise(char *buf, size_t size) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }

    cJSON *bridges = cJSON_AddArrayToObject(root, "bridges");
    for (size_t i = 0; i < config.bridges_count; i++) {
        cJSON *bridge = cJSON_CreateObject();
        cJSON_AddStringToObject(bridge, "id", config.bridges[i].id);
        cJSON_AddStringToObject(bridge, "internal_ip_address", config.bridges[i].internal_ip_address);
        cJSON_AddNumberToObject(bridge, "port", config.bridges[i].port);
        cJSON_AddStringToObject(bridge, "name", config.bridges[i].name);
        cJSON_AddStringToObject(bridge, "key", config.bridges[i].key);
        cJSON_AddItemToArray(bridges, bridge);
    }

    cJSON_PrintPreallocated(root, buf, size, false);
}

void config_deserialise(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        fprintf(stderr, "error parsing config");
        return;
    }

    cJSON *bridges = cJSON_GetObjectItem(root, "bridges");
    cJSON *bridge;
    cJSON_ArrayForEach(bridge, bridges) {
        cJSON *id = cJSON_GetObjectItem(bridge, "id");
        cJSON *internal_ip_address = cJSON_GetObjectItem(bridge, "internal_ip_address");
        cJSON *port = cJSON_GetObjectItem(bridge, "port");
        cJSON *name = cJSON_GetObjectItem(bridge, "name");
        cJSON *key = cJSON_GetObjectItem(bridge, "key");

        if (
            !cJSON_IsString(id) ||
            !cJSON_IsString(internal_ip_address) ||
            !cJSON_IsNumber(port) ||
            !cJSON_IsString(name) ||
            !cJSON_IsString(key)
        ) {
            continue;
        }

        if (config.bridges_count >= sizeof(config.bridges) / sizeof(config.bridges[0])) {
            break;
        }
        
        snprintf(
            config.bridges[config.bridges_count].id,
            sizeof(config.bridges[config.bridges_count].id),
            "%s",
            id->valuestring
        );
        snprintf(
            config.bridges[config.bridges_count].internal_ip_address,
            sizeof(config.bridges[config.bridges_count].internal_ip_address),
            "%s",
            internal_ip_address->valuestring
        );
        snprintf(
            config.bridges[config.bridges_count].name,
            sizeof(config.bridges[config.bridges_count].name),
            "%s",
            name->valuestring
        );
        snprintf(
            config.bridges[config.bridges_count].key,
            sizeof(config.bridges[config.bridges_count].key),
            "%s",
            key->valuestring
        );

        config.bridges[config.bridges_count].port = port->valueint;

        config.bridges_count += 1;
    }

    cJSON_Delete(root);
}

config_t *config_read() {
    const char *path = config_path();
    if (path == NULL) {
        fprintf(stderr, "error getting config path\n");
        return NULL;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        config = (config_t){0};
        config_write(&config);
        return &config;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0 || size >= CONFIG_JSON_LEN) {
        fprintf(stderr, "error getting config file size\n");
        fclose(file);
        return NULL;
    }

    char json[CONFIG_JSON_LEN] = {0};
    fread(json, 1, size, file);
    fclose(file);

    config_deserialise(json);

    return &config;
}

void config_write() {
    const char *path = config_path();
    if (path == NULL) {
        fprintf(stderr, "error getting config path\n");
        return; // TODO handle error
    }

    FILE* file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "error opening config for writing\n");
        return; // TODO handle error
    }

    char json[CONFIG_JSON_LEN] = {0}; 
    config_serialise(json, sizeof(json));
    fputs(json, file);
    fclose(file);
}
