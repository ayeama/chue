#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

#include "hue.h"

typedef enum {
    CURL_REQUEST_ROOM_GET_MANY,
    CURL_REQUEST_GROUPED_LIGHT_GET_MANY,
    CURL_REQUEST_GROUPED_LIGHT_PUT_ONE,
    CURL_REQUEST_GROUPED_LIGHT_PUT_ONE_BRIGHTNESS,
    CURL_REQUEST_LIGHT_GET_MANY,
    CURL_REQUEST_DISCOVER,
    CURL_REQUEST_CONFIG_GET_ONE,
} curl_request_t;

typedef struct {
    char *data;
    size_t size;
    long status_code;
} curl_response_t;

typedef struct {
    CURL *curl;
    struct curl_slist *headers;
    curl_response_t response;
} curl_handle_t;

typedef struct {
    curl_handle_t *handle;
    curl_request_t request;
    char *body;

    union {
        struct {
            hue_room_t *rooms;
            size_t *rooms_count;
        } room_get_many;

        struct {
            hue_grouped_light_t *grouped_lights;
            size_t *grouped_lights_count;
        } grouped_light_get_many;

        struct {
            hue_light_t *lights;
            size_t *lights_count;
        } light_get_many;

        struct {
            hue_bridge_t *bridges;
            size_t *bridges_count;
        } discover;

        struct {
            hue_bridge_t *bridge;
        } config_get_one;
    };
} curl_request_context_t;

static CURLM *curl_multi;

size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;

    curl_response_t *response = userdata;

    char *data = realloc(response->data, response->size + realsize + 1);
    if (data == NULL) {
        return 0;
    }

    response->data = data;
    memcpy(response->data + response->size, ptr, realsize);
    response->size += realsize;
    response->data[response->size] = '\0';

    return realsize;
}

hue_code_t room_get_many_deserialise(char *json, size_t size, hue_room_t *rooms, size_t *rooms_count) {
    *rooms_count = 0;

    cJSON *root = cJSON_ParseWithLength(json, size);
    if (root != NULL) {
        cJSON *data = cJSON_GetObjectItem(root, "data");
        
        cJSON *room;
        cJSON_ArrayForEach(room, data) {
            cJSON *id = cJSON_GetObjectItem(room, "id");
            cJSON *metadata = cJSON_GetObjectItem(room, "metadata");
            cJSON *name = cJSON_GetObjectItem(metadata, "name");

            if (!cJSON_IsString(id) || !cJSON_IsString(name)) {
                continue;
            }

            // TODO, figure out sizes
            if ((*rooms_count) < 16) {
                hue_room_t *dest = &rooms[*rooms_count];

                snprintf(
                    dest->id,
                    sizeof(dest->id),
                    "%s",
                    id->valuestring
                );
                snprintf(
                    dest->name,
                    sizeof(dest->name),
                    "%s",
                    name->valuestring
                );

                (*rooms_count) += 1;
            }
        }
    }
    
    cJSON_Delete(root);

    return HUE_CODE_OK;
}

hue_code_t grouped_light_get_many_deserialise(char *json, size_t size, hue_grouped_light_t *grouped_lights, size_t *grouped_lights_count) {
    *grouped_lights_count = 0;

    cJSON *root = cJSON_ParseWithLength(json, size);
    if (root != NULL) {
        cJSON *data = cJSON_GetObjectItem(root, "data");
    
        cJSON *grouped_light;
        cJSON_ArrayForEach(grouped_light, data) {
            cJSON *id = cJSON_GetObjectItem(grouped_light, "id");

            cJSON *owner = cJSON_GetObjectItem(grouped_light, "owner");
            cJSON *owner_id = cJSON_GetObjectItem(owner, "rid");
            cJSON *owner_type = cJSON_GetObjectItem(owner, "rtype");

            if (!cJSON_IsString(owner_id) || !cJSON_IsString(owner_type)) {
                continue;
            }

            if (strcmp(owner_type->valuestring, "room") != 0) {
                continue;
            }

            cJSON *on_object = cJSON_GetObjectItem(grouped_light, "on");
            cJSON *on = cJSON_GetObjectItem(on_object, "on");

            cJSON *dimming = cJSON_GetObjectItem(grouped_light, "dimming");
            cJSON *brightness = cJSON_GetObjectItem(dimming, "brightness");

            if (!cJSON_IsBool(on) || !cJSON_IsNumber(brightness)) {
                continue;
            }

            // TODO, figure out sizes
            if ((*grouped_lights_count) < 16) {
                hue_grouped_light_t *dest = &grouped_lights[*grouped_lights_count];

                snprintf(
                    dest->id,
                    sizeof(dest->id),
                    "%s",
                    id->valuestring
                );
                snprintf(
                    dest->owner_id,
                    sizeof(dest->owner_id),
                    "%s",
                    owner_id->valuestring
                );
                snprintf(
                    dest->owner_type,
                    sizeof(dest->owner_type),
                    "%s",
                    owner_type->valuestring
                );

                dest->brightness = brightness->valuedouble;
                dest->on = cJSON_IsTrue(on);

                (*grouped_lights_count) += 1;
            }
        }
    }

    cJSON_Delete(root);

    return HUE_CODE_OK;
}

hue_code_t light_get_many_deserialise(char *json, size_t size, hue_light_t *lights, size_t *lights_count) {
    *lights_count = 0;

    cJSON *root = cJSON_ParseWithLength(json, size);
    if (root != NULL) {
        cJSON *data = cJSON_GetObjectItem(root, "data");
    
        cJSON *light;
        cJSON_ArrayForEach(light, data) {
            cJSON *id = cJSON_GetObjectItem(light, "id");

            cJSON *metadata = cJSON_GetObjectItem(light, "metadata");
            cJSON *name = cJSON_GetObjectItem(metadata, "name");
            cJSON *archetype = cJSON_GetObjectItem(metadata, "archetype");

            if (!cJSON_IsString(name) || !cJSON_IsString(archetype)) {
                continue;
            }

            cJSON *on_object = cJSON_GetObjectItem(light, "on");
            cJSON *on = cJSON_GetObjectItem(on_object, "on");

            cJSON *dimming = cJSON_GetObjectItem(light, "dimming");
            cJSON *brightness = cJSON_GetObjectItem(dimming, "brightness");

            if (!cJSON_IsBool(on) || !cJSON_IsNumber(brightness)) {
                continue;
            }

            // TODO, figure out sizes
            if ((*lights_count) < 16) {
                hue_light_t *dest = &lights[*lights_count];

                snprintf(
                    dest->id,
                    sizeof(dest->id),
                    "%s",
                    id->valuestring
                );
                snprintf(
                    dest->name,
                    sizeof(dest->name),
                    "%s",
                    name->valuestring
                );
                snprintf(
                    dest->archetype,
                    sizeof(dest->archetype),
                    "%s",
                    archetype->valuestring
                );

                dest->brightness = brightness->valuedouble;
                dest->on = cJSON_IsTrue(on);

                (*lights_count) += 1;
            }
        }
    }

    cJSON_Delete(root);

    return HUE_CODE_OK;
}

hue_code_t discover_deserialise(char *json, size_t size, hue_bridge_t *bridges, size_t *bridges_count) {
    *bridges_count = 0;

    cJSON *root = cJSON_ParseWithLength(json, size);
    if (root != NULL) {
        cJSON *bridge;
        cJSON_ArrayForEach(bridge, root) {
            cJSON *id = cJSON_GetObjectItem(bridge, "id");
            cJSON *ip = cJSON_GetObjectItem(bridge, "internalipaddress");
            cJSON *port = cJSON_GetObjectItem(bridge, "port");

            if (
                !cJSON_IsString(id) ||
                !cJSON_IsString(ip) ||
                !cJSON_IsNumber(port)
            ) {
                continue;
            }

            if ((*bridges_count) < 4) {
                hue_bridge_t *dest = &bridges[*bridges_count];
                
                snprintf(
                    dest->id,
                    sizeof(dest->id),
                    "%s",
                    id->valuestring
                );
                snprintf(
                    dest->internal_ip_address,
                    sizeof(dest->internal_ip_address),
                    "%s",
                    ip->valuestring
                );

                dest->port = port->valueint;

                (*bridges_count) += 1;
            }
        }
    }

    cJSON_Delete(root);

    return HUE_CODE_OK;
}

hue_code_t config_get_one_deserialise(char *json, size_t size, hue_bridge_t *bridge) {
    cJSON *root = cJSON_ParseWithLength(json, size);
    if (root != NULL) {
        cJSON *name = cJSON_GetObjectItem(root, "name");
        
        if (cJSON_IsString(name)) {
            snprintf(
                bridge->name,
                sizeof(bridge->name),
                "%s",
                name->valuestring
            );
        }
    }

    cJSON_Delete(root);

    return HUE_CODE_OK;
}

hue_code_t hue_room_get_many(hue_bridge_t *bridge, hue_room_t *rooms, size_t *rooms_count) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;

    char key_header[62] = {0};
    snprintf(key_header, sizeof(key_header), "hue-application-key: %s", bridge->key);
    handle->headers = curl_slist_append(handle->headers, key_header);
    
    handle->response = (curl_response_t){0};

    context->handle = handle;
    context->request = CURL_REQUEST_ROOM_GET_MANY;
    context->room_get_many.rooms = rooms;
    context->room_get_many.rooms_count = rooms_count;

    // curl_easy_setopt(handle->curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[46];
    snprintf(url, sizeof(url), "https://%s/clip/v2/resource/room", bridge->internal_ip_address);

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_grouped_light_get_many(hue_bridge_t *bridge, hue_grouped_light_t *grouped_lights, size_t *grouped_lights_count) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;

    char key_header[62] = {0};
    snprintf(key_header, sizeof(key_header), "hue-application-key: %s", bridge->key);
    handle->headers = curl_slist_append(handle->headers, key_header);
    
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_GROUPED_LIGHT_GET_MANY;
    context->grouped_light_get_many.grouped_lights = grouped_lights;
    context->grouped_light_get_many.grouped_lights_count = grouped_lights_count;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[55];
    snprintf(url, sizeof(url), "https://%s/clip/v2/resource/grouped_light", bridge->internal_ip_address);

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_grouped_light_put_one(hue_bridge_t *bridge, hue_grouped_light_t *grouped_light) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;

    char key_header[62] = {0};
    snprintf(key_header, sizeof(key_header), "hue-application-key: %s", bridge->key);
    handle->headers = curl_slist_append(handle->headers, key_header);
    
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_GROUPED_LIGHT_PUT_ONE;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[92];
    snprintf(
        url,
        sizeof(url),
        "https://%s/clip/v2/resource/grouped_light/%s",
        bridge->internal_ip_address,
        grouped_light->id
    );

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    cJSON *root = cJSON_CreateObject();
    cJSON *on_outer = cJSON_CreateObject();
    cJSON *on_inner = cJSON_CreateBool(!grouped_light->on);
    cJSON_AddItemToObject(on_outer, "on", on_inner);
    cJSON_AddItemToObject(root, "on", on_outer);
    context->body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    curl_easy_setopt(context->handle->curl, CURLOPT_POSTFIELDS, context->body);
    curl_easy_setopt(context->handle->curl, CURLOPT_CUSTOMREQUEST, "PUT");

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_grouped_light_put_one_brightness(hue_bridge_t *bridge, hue_grouped_light_t *grouped_light, double brightness) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;

    char key_header[62] = {0};
    snprintf(key_header, sizeof(key_header), "hue-application-key: %s", bridge->key);
    handle->headers = curl_slist_append(handle->headers, key_header);
    
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_GROUPED_LIGHT_PUT_ONE;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[92];
    snprintf(
        url,
        sizeof(url),
        "https://%s/clip/v2/resource/grouped_light/%s",
        bridge->internal_ip_address,
        grouped_light->id
    );

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    cJSON *root = cJSON_CreateObject();
    cJSON *dimming = cJSON_CreateObject();
    cJSON *dimming_inner = cJSON_CreateNumber(brightness);
    cJSON_AddItemToObject(dimming, "brightness", dimming_inner);
    cJSON_AddItemToObject(root, "dimming", dimming);
    context->body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    curl_easy_setopt(context->handle->curl, CURLOPT_POSTFIELDS, context->body);
    curl_easy_setopt(context->handle->curl, CURLOPT_CUSTOMREQUEST, "PUT");

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_light_get_many(hue_bridge_t *bridge, hue_light_t *lights, size_t *lights_count) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;

    char key_header[62] = {0};
    snprintf(key_header, sizeof(key_header), "hue-application-key: %s", bridge->key);
    handle->headers = curl_slist_append(handle->headers, key_header);
    
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_LIGHT_GET_MANY;
    context->light_get_many.lights = lights;
    context->light_get_many.lights_count = lights_count;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[55];
    snprintf(url, sizeof(url), "https://%s/clip/v2/resource/light", bridge->internal_ip_address);

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_discover(hue_bridge_t *bridges, size_t *bridges_count) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;
    handle->headers = NULL;
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_DISCOVER;
    context->discover.bridges = bridges;
    context->discover.bridges_count = bridges_count;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, "https://discovery.meethue.com");
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_config_get_one(hue_bridge_t *bridge) {
    curl_request_context_t *context = calloc(1, sizeof(curl_request_context_t));
    if (context == NULL) {
        return HUE_CODE_ERROR;
    }

    curl_handle_t *handle = calloc(1, sizeof(curl_handle_t));
    if (handle == NULL) {
        return HUE_CODE_ERROR;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HUE_CODE_ERROR;
    }

    handle->curl = curl;
    handle->headers = NULL;
    handle->response = (curl_response_t){0};
    
    context->handle = handle;
    context->request = CURL_REQUEST_CONFIG_GET_ONE;
    context->config_get_one.bridge = bridge;

    curl_easy_setopt(context->handle->curl, CURLOPT_PRIVATE, context);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(context->handle->curl, CURLOPT_WRITEDATA, &context->handle->response);

    char url[37]; 
    snprintf(url, sizeof(url), "https://%s/api/0/config", bridge->internal_ip_address);

    curl_easy_setopt(context->handle->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(context->handle->curl, CURLOPT_HTTPHEADER, context->handle->headers);

    CURLMcode cmresult = curl_multi_add_handle(curl_multi, context->handle->curl);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t curl_dispatch() {
    CURLMsg *msg;
    int msgs_left;

    while ((msg = curl_multi_info_read(curl_multi, &msgs_left)) != NULL) {
        if (msg->msg != CURLMSG_DONE) {
            continue;
        }

        curl_request_context_t *context;
        CURLcode cresult = curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &context);
        if (cresult != CURLE_OK) {
            continue;
        }
        
        if (context->handle == NULL) {
            continue;
        }

        cresult = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &context->handle->response.status_code);
        if (cresult != CURLE_OK) {
            continue;
        }

        switch (context->request) {
        case CURL_REQUEST_ROOM_GET_MANY:
            room_get_many_deserialise(
                context->handle->response.data,
                context->handle->response.size,
                context->room_get_many.rooms,
                context->room_get_many.rooms_count
            );
            break;
        case CURL_REQUEST_GROUPED_LIGHT_GET_MANY:
            grouped_light_get_many_deserialise(
                context->handle->response.data,
                context->handle->response.size,
                context->grouped_light_get_many.grouped_lights,
                context->grouped_light_get_many.grouped_lights_count
            );
            break;
        case CURL_REQUEST_GROUPED_LIGHT_PUT_ONE:
            break;
        case CURL_REQUEST_GROUPED_LIGHT_PUT_ONE_BRIGHTNESS:
            break;
        case CURL_REQUEST_LIGHT_GET_MANY:
            light_get_many_deserialise(
                context->handle->response.data,
                context->handle->response.size,
                context->light_get_many.lights,
                context->light_get_many.lights_count
            );
            break;
        case CURL_REQUEST_DISCOVER:
            if (context->handle->response.status_code == 429) {
                break;
            }
            discover_deserialise(
                context->handle->response.data,
                context->handle->response.size,
                context->discover.bridges,
                context->discover.bridges_count
            );
            break;
        case CURL_REQUEST_CONFIG_GET_ONE:
            config_get_one_deserialise(
                context->handle->response.data,
                context->handle->response.size,
                context->config_get_one.bridge
            );
            break;
        default:
            // TODO error?
            break;
        }

        curl_multi_remove_handle(curl_multi, context->handle->curl);
        curl_slist_free_all(context->handle->headers);
        context->handle->headers = NULL;
        free(context->handle->response.data);
        context->handle->response.data = NULL;
        context->handle->response.size = 0;
        curl_easy_cleanup(context->handle->curl);
        free(context->handle);
        if (context->body != NULL) {
            free(context->body);
        }
        free(context);
    }

    return HUE_CODE_OK;
}

hue_code_t hue_poll() {
    int curl_running_handles = 0;
    CURLMcode cmresult = curl_multi_perform(curl_multi, &curl_running_handles);
    if (cmresult != CURLM_OK) {
        return HUE_CODE_ERROR;
    }

    if (curl_running_handles > 0) {
        cmresult = curl_multi_poll(curl_multi, NULL, 0, 0, NULL);
        if (cmresult != CURLM_OK) {
            return HUE_CODE_ERROR;
        }
    }

    hue_code_t hresult = curl_dispatch();
    if (hresult != HUE_CODE_OK) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_create() {
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        return HUE_CODE_ERROR;
    }

    curl_multi = curl_multi_init();
    if (curl_multi == NULL) {
        return HUE_CODE_ERROR;
    }

    return HUE_CODE_OK;
}

hue_code_t hue_destroy() {
    if (curl_multi != NULL) {
        curl_multi_cleanup(curl_multi);
    }

    curl_global_cleanup();

    return HUE_CODE_OK;
}
