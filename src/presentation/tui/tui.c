#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <ncurses.h>

#include <application/lightlist.h>
#include <application/roomlist.h>
#include <domain/bridge.h>
#include <domain/light.h>
#include <domain/room.h>
#include <presentation/tui/footer.h>
#include <presentation/tui/header.h>
#include <presentation/tui/table.h>

#define BS 8
#define HT 9
#define LF 10
#define ESC 27
#define DEL 127

#define BADDR "<bridge address>"
#define BUSER "<bridge username>"

typedef enum {
    RESOURCE_LIGHT,
    RESOURCE_ROOM,
} Resource;

Resource resource = RESOURCE_LIGHT;

typedef enum {
    MODE_NORMAL,
    MODE_COMMAND,
} Mode;

Mode mode = MODE_NORMAL;

typedef enum {
    COMMAND_ERROR,
    COMMAND_LIGHT,
    COMMAND_ROOM,
    COMMAND_HELP,
    COMMAND_QUIT,
} Command;

time_t now = 0;
time_t poll = 0;

void curl_hue_light_toggle();

// typedef struct Content {
//     Table *table;
// } Content;

// typedef struct Application {
//     Content *content;
// } Application;

// Application *app = NULL;

// TODO
TableBehavior *light = NULL;
LightList *lightlist = NULL;

TableBehavior *room = NULL;
RoomList *roomlist = NULL;

Table *t = NULL;

Header *h = NULL;
Footer *f = NULL;

static WINDOW *wheader = NULL;
static WINDOW *wfooter = NULL;
static WINDOW *wcontent = NULL;

CURLM *cm;

struct buffer {
    char *data;
    size_t len;
};

enum request_type {
    REQUEST_TYPE_BRIDGE,
    REQUEST_TYPE_LIGHT,
    REQUEST_TYPE_ROOM,
};

struct request {
    enum request_type type;
    struct buffer buf;
    struct curl_slist *headers;
};

void curl_init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    cm = curl_multi_init();
}

void curl_end() {
    curl_multi_cleanup(cm);
    curl_global_cleanup();
}

static size_t curl_hue_lights_read_callback(void *buffer, size_t size, size_t nmemb, void *stream) {
    size_t total = size * nmemb;
    struct buffer *buf = stream;

    char *ndata = realloc(buf->data, (buf->len + total + 1));
    if (!ndata) {
        return 0;
    }

    buf->data = ndata;
    memcpy((buf->data + buf->len), buffer, total);
    buf->len += total;
    buf->data[buf->len] = '\0';

    return total;
}

// TODO needed?
static size_t curl_hue_rooms_read_callback(void *buffer, size_t size, size_t nmemb, void *stream) {
    size_t total = size * nmemb;
    struct buffer *buf = stream;

    char *ndata = realloc(buf->data, (buf->len + total + 1));
    if (!ndata) {
        return 0;
    }

    buf->data = ndata;
    memcpy((buf->data + buf->len), buffer, total);
    buf->len += total;
    buf->data[buf->len] = '\0';

    return total;
}

// TODO needed?
static size_t curl_hue_bridges_read_callback(void *buffer, size_t size, size_t nmemb, void *stream) {
    size_t total = size * nmemb;
    struct buffer *buf = stream;

    char *ndata = realloc(buf->data, (buf->len + total + 1));
    if (!ndata) {
        return 0;
    }

    buf->data = ndata;
    memcpy((buf->data + buf->len), buffer, total);
    buf->len += total;
    buf->data[buf->len] = '\0';

    return total;
}

static size_t curl_hue_light_toggle_callback(void *buffer, size_t size, size_t nmemb, void *stream) {
    // noop
    (void)buffer;
    (void)stream;
    return size * nmemb;
}

void curl_hue_light_toggle() {
    // lights[selected].on = !lights[selected].on;
    Light *light = &((LightList *)t->data)->items[t->sindex];
    light->on = !light->on;

    /* curl request */
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl init error");
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    char url[1024] = {0};
    snprintf(url, 1024, "https://%s/clip/v2/resource/light/%s", BADDR, light->id);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: chue/0.0.1");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    char hauth[1024] = {0};
    snprintf(hauth, 1024, "hue-application-key: %s", BUSER);
    headers = curl_slist_append(headers, hauth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    cJSON *json = cJSON_CreateObject();
    cJSON *on = cJSON_CreateObject();
    cJSON_AddBoolToObject(on, "on", light->on);
    cJSON_AddItemToObject(json, "on", on);
    char *content = cJSON_PrintUnformatted(json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_hue_light_toggle_callback);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl perform error");
    }

    free(content);
    cJSON_Delete(json);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void curl_request_bridge() {
    CURL *c = curl_easy_init();

    struct request *req = calloc(1, sizeof(struct request));
    req->type = REQUEST_TYPE_BRIDGE;

    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);

    char url[1024] = {0};
    snprintf(url, 1024, "https://%s/clip/v2/resource/bridge", BADDR);
    curl_easy_setopt(c, CURLOPT_URL, url);

    req->headers = NULL;
    req->headers = curl_slist_append(req->headers, "User-Agent: chue/0.0.1");

    char hauth[1024] = {0};
    snprintf(hauth, 1024, "hue-application-key: %s", BUSER);
    req->headers = curl_slist_append(req->headers, hauth);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, req->headers);

    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_hue_bridges_read_callback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &req->buf);

    curl_easy_setopt(c, CURLOPT_PRIVATE, req);
    curl_multi_add_handle(cm, c);
}

void curl_request_light() {
    CURL *c = curl_easy_init();

    struct request *req = calloc(1, sizeof(struct request));
    req->type = REQUEST_TYPE_LIGHT;

    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);

    char url[1024] = {0};
    snprintf(url, 1024, "https://%s/clip/v2/resource/light", BADDR);
    curl_easy_setopt(c, CURLOPT_URL, url);

    req->headers = NULL;
    req->headers = curl_slist_append(req->headers, "User-Agent: chue/0.0.1");

    char hauth[1024] = {0};
    snprintf(hauth, 1024, "hue-application-key: %s", BUSER);
    req->headers = curl_slist_append(req->headers, hauth);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, req->headers);

    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_hue_lights_read_callback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &req->buf);

    curl_easy_setopt(c, CURLOPT_PRIVATE, req);
    curl_multi_add_handle(cm, c);
}

void curl_request_room() {
    CURL *c = curl_easy_init();

    struct request *req = calloc(1, sizeof(struct request));
    req->type = REQUEST_TYPE_ROOM;

    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);

    char url[1024] = {0};
    snprintf(url, 1024, "https://%s/clip/v2/resource/room", BADDR);
    curl_easy_setopt(c, CURLOPT_URL, url);

    req->headers = NULL;
    req->headers = curl_slist_append(req->headers, "User-Agent: chue/0.0.1");

    char hauth[1024] = {0};
    snprintf(hauth, 1024, "hue-application-key: %s", BUSER);
    req->headers = curl_slist_append(req->headers, hauth);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, req->headers);

    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_hue_rooms_read_callback);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &req->buf);

    curl_easy_setopt(c, CURLOPT_PRIVATE, req);
    curl_multi_add_handle(cm, c);
}

void curl_poll() {
    int r;
    curl_multi_perform(cm, &r);
    curl_multi_wait(cm, NULL, 0, 25, NULL);
}

void curl_response() {
    CURLMsg *msg;
    int msgr;

    while ((msg = curl_multi_info_read(cm, &msgr))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *c = msg->easy_handle;
            
            struct request *req = NULL;
            curl_easy_getinfo(c, CURLINFO_PRIVATE, &req);

            double response_time;
            curl_easy_getinfo(c, CURLINFO_TOTAL_TIME, &response_time);
            mvwprintw(wfooter, 0, (getmaxx(wfooter) - 7), "%d:%3.0lfms", req->type, (response_time * 1000)); // TODO tmp

            if (req != NULL) {
                // TODO refactor
                cJSON *json = cJSON_Parse(req->buf.data);
                if (!json) {
                    fprintf(stderr, "cjson parse error");
                }

                switch (req->type) {
                    case REQUEST_TYPE_BRIDGE: {
                        cJSON *data = cJSON_GetObjectItem(json, "data");
                        cJSON *item = NULL;

                        if (!cJSON_IsArray(data)) {
                            break;
                        }
                        size_t count = cJSON_GetArraySize(data);
                        Bridge *nbridges = calloc(count, sizeof(Bridge)); // TODO free

                        int i = 0;
                        cJSON_ArrayForEach(item, data) {
                            Bridge *bridge = &(nbridges[i]);

                            cJSON *id = cJSON_GetObjectItem(item, "id");
                            if (cJSON_IsString(id)) {
                                strncpy(bridge->id, id->valuestring, 37);
                            }

                            cJSON *owner = cJSON_GetObjectItem(item, "owner");
                            cJSON *rid = cJSON_GetObjectItem(owner, "rid");
                            if (cJSON_IsString(rid)) {
                                strncpy(bridge->rid, rid->valuestring, 37);
                            }

                            cJSON *bridge_id = cJSON_GetObjectItem(item, "bridge_id");
                            if (cJSON_IsString(bridge_id)) {
                                strncpy(bridge->bridge_id, bridge_id->valuestring, 17);
                            }

                            i++;
                        }

                        mvwprintw(wheader, 1, 0, "%s: %s", "Bridge ID", nbridges[0].bridge_id);
                        free(nbridges);

                        break;
                    }
                    case REQUEST_TYPE_LIGHT: {
                        cJSON *data = cJSON_GetObjectItem(json, "data");
                        cJSON *item = NULL;

                        // TODO error handling
                        if (!cJSON_IsArray(data)) {
                            break;
                        }
                        size_t count = cJSON_GetArraySize(data);
                        Light *nlights = calloc(count, sizeof(Light));

                        int i = 0;
                        cJSON_ArrayForEach(item, data) {
                            cJSON *id = cJSON_GetObjectItem(item, "id");
                            if (cJSON_IsString(id)) {
                                Light *light = &(nlights[i]);
                                strncpy(light->id, id->valuestring, 37);
                            }

                            cJSON *metadata = cJSON_GetObjectItem(item, "metadata");
                            cJSON *name = cJSON_GetObjectItem(metadata, "name");
                            if (cJSON_IsString(name)) {
                                Light *light = &(nlights[i]);
                                light->name = strdup(name->valuestring);
                            }

                            cJSON *on = cJSON_GetObjectItem(item, "on");
                            cJSON *onon = cJSON_GetObjectItem(on, "on");
                            if (cJSON_IsBool(onon)) {
                                Light *light = &(nlights[i]);
                                light->on = onon->valueint;
                            }

                            cJSON *dimming = cJSON_GetObjectItem(item, "dimming");
                            cJSON *brightness = cJSON_GetObjectItem(dimming, "brightness");
                            if (cJSON_IsNumber(brightness)) {
                                Light *light = &(nlights[i]);
                                light->brightness = brightness->valuedouble;   
                            }

                            i++;
                        }

                        free(lightlist->items);
                        lightlist->items = nlights;
                        lightlist->count = count;

                        break;
                    }
                    case REQUEST_TYPE_ROOM: {
                        cJSON *data = cJSON_GetObjectItem(json, "data");
                        cJSON *item = NULL;

                        // TODO error handling
                        if (!cJSON_IsArray(data)) {
                            break;
                        }

                        size_t count = cJSON_GetArraySize(data);
                        Room *nrooms = calloc(count, sizeof(Room));

                        int i = 0;
                        cJSON_ArrayForEach(item, data) {
                            cJSON *id = cJSON_GetObjectItem(item, "id");
                            if (cJSON_IsString(id)) {
                                Room *room = &(nrooms[i]);
                                strncpy(room->id, id->valuestring, 37);
                            }

                            cJSON *metadata = cJSON_GetObjectItem(item, "metadata");
                            cJSON *name = cJSON_GetObjectItem(metadata, "name");
                            if (cJSON_IsString(name)) {
                                Room *room = &(nrooms[i]);
                                room->name = strdup(name->valuestring);
                            }

                            i++;
                        }

                        free(roomlist->items);
                        roomlist->items = nrooms;
                        roomlist->count = count;

                        break;
                    }
                    default:
                        break;
                }
            
                cJSON_Delete(json);
    
                curl_slist_free_all(req->headers);
                free(req->buf.data);
                free(req);
            }

            curl_multi_remove_handle(cm, c);
            curl_easy_cleanup(c);
        }
    }
}

void draw_header() {
    if (wheader == NULL) {
        int starty = 0;
        int startx = 0;
        int height = 4;
        int width = COLS;
        wheader = newwin(height, width, starty, startx);
    }

    header_draw(wheader, h);
    wnoutrefresh(wheader);
}

void draw_footer() {
    if (wfooter == NULL) {
        int starty = LINES - 1;
        int startx = 0;
        int height = 1;
        int width = COLS;
        wfooter = newwin(height, width, starty, startx);
    }

    footer_draw(wfooter, f);
    wnoutrefresh(wfooter);
}

void draw_content() {
    if (wcontent == NULL) {
        int starty = 4;
        int startx = 0;
        int height = LINES - 4 - 1;
        int width = COLS;
        wcontent = newwin(height, width, starty, startx);

        t->wsize = getmaxy(wcontent) - 3;
    }

    table_draw(wcontent, t);
    wnoutrefresh(wcontent);
}

void draw() {
    refresh();

    draw_header();
    draw_footer();
    draw_content();

    doupdate();
}

Command command_execute() {
    if (f->command == NULL || strcmp(f->command, "") == 0) {
        return COMMAND_ERROR;
    }

    if (strncmp(f->command, "light", strlen(f->command)) == 0) {
        t->data = lightlist;
        t->behavior = light;
        t->sindex = 0;
        t->windex = 0;
        table_clear(wcontent, t);
        return COMMAND_LIGHT;
    } else if (strncmp(f->command, "room", strlen(f->command)) == 0) {
        t->data = roomlist;
        t->behavior = room;
        t->sindex = 0;
        t->windex = 0;
        table_clear(wcontent, t);
        return COMMAND_ROOM;
    } else if (strncmp(f->command, "help", strlen(f->command)) == 0) {
        // TODO
        return COMMAND_HELP;
    } else if (strncmp(f->command, "quit", strlen(f->command)) == 0) {
        // TODO
        return COMMAND_QUIT;
    }

    return COMMAND_ERROR;
}

void loop() {
    curl_request_bridge(); // TODO tmp

    int ch = 0;
    do {
        if (mode == MODE_NORMAL) {
            switch (ch) {
                case 'h':
                    break;
                case 'j': // down
                    table_down(t);
                    break;
                case 'k': // up
                    table_up(t);
                    break;
                case 'l':
                    break;
                case ' ': // mark
                    table_select(t);
                    break;
                case 'g': // top
                    table_top(t);
                    break;
                case 'G': // bottom
                    table_bottom(t);
                    break;
                case ':': // command
                    mode = MODE_COMMAND;
                    break;
                case '/': // filter
                    break;
                case ESC: // clear, cancel, back
                    break;
                case KEY_RESIZE: // window resize
                    // TODO improve?
                    delwin(wheader);
                    wheader = NULL;
                    delwin(wfooter);
                    wfooter = NULL;
                    delwin(wcontent);
                    wcontent = NULL;
                    break;
                case ERR:
                    break;
                default:
                    break;
            }
        } else if (mode == MODE_COMMAND) {
            if (f->command == NULL) {
                f->command = calloc(32, sizeof(char));
            }

            switch (ch) {
                case HT:
                case ERR:
                    break;
                case LF:
                    // TODO
                    Command c = command_execute();
                    if (c == COMMAND_QUIT) {
                        return;
                    }
                    [[fallthrough]]; // NOTE C23
                case ESC:
                    footer_command_clear(wfooter, f); // TODO tmp
                    mode = MODE_NORMAL;
                    break;
                case DEL:
                case BS:
                    if (f->command != NULL) {
                        size_t len = strlen(f->command);
                        if (len == 0) {
                            footer_command_clear(wfooter, f);
                            mode = MODE_NORMAL;
                        } else if (len > 0) {
                            f->command[(len - 1)] = '\0';
                        }
                    }
                    break;
                default:
                    char buf[32] = {0};
                    snprintf(buf, 32, "%s%c", f->command, ch);
                    strncpy(f->command, buf, 32);
                    break;
            }
        }

        now = time(NULL);
        if ((now - poll) > 1) {
            poll = now;
            curl_request_light();
            curl_request_room();
        }

        curl_poll();
        curl_response();

        draw();
    } while ((ch = getch()));
}

int ncurses_init() {
    initscr();
    timeout(25);
    cbreak();
    noecho();
    set_escdelay(25);
    curs_set(0);

    if (!has_colors()) {
        fprintf(stderr, "colors not supported");
        return 1;
    }

    if (!can_change_color()) {
        fprintf(stderr, "changing colors not supported");
        return 1;
    }

    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);

    return 0;
}

void ncurses_end() {
    if (wheader != NULL) {
        delwin(wheader);
    }

    if (wfooter != NULL) {
        delwin(wfooter);
    }

    if (wcontent != NULL) {
        delwin(wcontent);
    }

    endwin();
}

void init() {
    lightlist = calloc(1, sizeof(LightList));
    lightlist->items = NULL;
    lightlist->count = 0;

    light = tablebehavior_create();
    light->cols = lightlist_cols;
    light->rows = lightlist_rows;
    light->title = lightlist_title;
    light->header = lightlist_header;
    light->text = lightlist_text;
    light->select = lightlist_select;

    roomlist = calloc(1, sizeof(RoomList));
    roomlist->items = NULL;
    roomlist->count = 0;

    room = tablebehavior_create();
    room->cols = roomlist_cols;
    room->rows = roomlist_rows;
    room->title = roomlist_title;
    room->header = roomlist_header;
    room->text = roomlist_text;
    room->select = roomlist_select;

    t = table_create(light, lightlist);
    h = header_create();
    f = footer_create();

    curl_init();
    ncurses_init();
}

void end() {
    free(((LightList *)t->data)->items); // TODO unknown type?
    
    tablebehavior_free(light);
    tablebehavior_free(room);

    table_free(t);
    header_free(h);
    footer_free(f);

    curl_end();
    ncurses_end();
}

void tui() {
    init();
    loop();
    end();
}