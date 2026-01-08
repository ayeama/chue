#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <ncurses.h>

#include <application/lightlist.h>
#include <domain/light.h>
#include <domain/room.h>
#include <presentation/tui/footer.h>
#include <presentation/tui/header.h>
#include <presentation/tui/table.h>

#define ESC 17

#define BADDR "<bridge address>"
#define BUSER "<bridge username>"

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
TableBehavior *b = NULL;
Table *t = NULL;
LightList *ll = NULL;

Header *h = NULL;
Footer *f = NULL;

CURLM *cm;

struct buffer {
    char *data;
    size_t len;
};

struct request {
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

void curl_hue_lights_read() {
    /* curl request */
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl init error");
        return;
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    char url[1024] = {0};
    snprintf(url, 1024, "https://%s/clip/v2/resource/light", BADDR);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: chue/0.0.1");

    char hauth[1024] = {0};
    snprintf(hauth, 1024, "hue-application-key: %s", BUSER);
    headers = curl_slist_append(headers, hauth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    struct buffer buf = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_hue_lights_read_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl perform error");
    }

    /* cjson response parsing */
    if (res == CURLE_OK) {
        cJSON *json = cJSON_Parse(buf.data);

        if (!json) {
            fprintf(stderr, "cjson parse error");
        } else {
            cJSON *data = cJSON_GetObjectItem(json, "data");
            cJSON *item = NULL;

            // TODO error handling
            if (cJSON_IsArray(data)) {
                ((LightList *)t->data)->count = cJSON_GetArraySize(data);
                ((LightList *)t->data)->items = calloc(((LightList *)t->data)->count, sizeof(Light));
            }

            int i = 0;
            cJSON_ArrayForEach(item, data) {
                cJSON *id = cJSON_GetObjectItem(item, "id");
                if (cJSON_IsString(id)) {
                    // lights[i].id = strdup(id->valuestring);
                    Light *light = &(((LightList *)t->data)->items[i]);
                    strncpy(light->id, id->valuestring, 37);
                }

                cJSON *metadata = cJSON_GetObjectItem(item, "metadata");
                cJSON *name = cJSON_GetObjectItem(metadata, "name");
                if (cJSON_IsString(name)) {
                    // lights[i].name = strdup(name->valuestring);
                    Light *light = &(((LightList *)t->data)->items[i]);
                    light->name = strdup(name->valuestring);
                }

                cJSON *on = cJSON_GetObjectItem(item, "on");
                cJSON *onon = cJSON_GetObjectItem(on, "on");
                if (cJSON_IsBool(onon)) {
                    // lights[i].on = onon->valueint;
                    Light *light = &(((LightList *)t->data)->items[i]);
                    light->on = onon->valueint;
                }

                cJSON *dimming = cJSON_GetObjectItem(item, "dimming");
                cJSON *brightness = cJSON_GetObjectItem(dimming, "brightness");
                if (cJSON_IsNumber(brightness)) {
                    // lights[i].brightness = brightness->valuedouble;
                    Light *light = &(((LightList *)t->data)->items[i]);
                    light->brightness = brightness->valuedouble;   
                }

                i++;
            }
        }

        cJSON_Delete(json);
    }

    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
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


void curl_request() {
    CURL *c = curl_easy_init();

    struct request *req = calloc(1, sizeof(struct request));

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

            if (req != NULL) {
                cJSON *json = cJSON_Parse(req->buf.data);

                if (!json) {
                    fprintf(stderr, "cjson parse error");
                } else {
                    cJSON *data = cJSON_GetObjectItem(json, "data");
                    cJSON *item = NULL;

                    // TODO error handling
                    if (cJSON_IsArray(data)) {
                        ((LightList *)t->data)->count = cJSON_GetArraySize(data);
                        ((LightList *)t->data)->items = calloc(((LightList *)t->data)->count, sizeof(Light));
                    }

                    int i = 0;
                    cJSON_ArrayForEach(item, data) {
                        cJSON *id = cJSON_GetObjectItem(item, "id");
                        if (cJSON_IsString(id)) {
                            Light *light = &(((LightList *)t->data)->items[i]);
                            strncpy(light->id, id->valuestring, 37);
                        }

                        cJSON *metadata = cJSON_GetObjectItem(item, "metadata");
                        cJSON *name = cJSON_GetObjectItem(metadata, "name");
                        if (cJSON_IsString(name)) {
                            Light *light = &(((LightList *)t->data)->items[i]);
                            light->name = strdup(name->valuestring);
                        }

                        cJSON *on = cJSON_GetObjectItem(item, "on");
                        cJSON *onon = cJSON_GetObjectItem(on, "on");
                        if (cJSON_IsBool(onon)) {
                            Light *light = &(((LightList *)t->data)->items[i]);
                            light->on = onon->valueint;
                        }

                        cJSON *dimming = cJSON_GetObjectItem(item, "dimming");
                        cJSON *brightness = cJSON_GetObjectItem(dimming, "brightness");
                        if (cJSON_IsNumber(brightness)) {
                            Light *light = &(((LightList *)t->data)->items[i]);
                            light->brightness = brightness->valuedouble;   
                        }

                        i++;
                    }
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

static WINDOW *wheader = NULL;
static WINDOW *wfooter = NULL;
static WINDOW *wcontent = NULL;

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

void loop() {
    int ch = 0;
    do {
        switch (ch) {
            case 'q': // quit
                return;
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

        now = time(NULL);
        if ((now - poll) > 1) {
            poll = now;
            curl_request();
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
    ll = calloc(1, sizeof(LightList));
    ll->items = NULL;
    ll->count = 0;

    b = tablebehavior_create();
    b->cols = lightlist_cols;
    b->rows = lightlist_rows;
    b->header = lightlist_header;
    b->text = lightlist_text;
    b->select = lightlist_select;

    t = table_create(b, ll);
    h = header_create();
    f = footer_create();

    curl_init();
    ncurses_init();
}

void end() {
    free(((LightList *)t->data)->items);
    tablebehavior_free(b);
    table_free(t);
    header_free(h);
    footer_free(f);

    curl_end();
    ncurses_end();
}

int main() {
    init();
    loop();
    end();

    return 0;
}
