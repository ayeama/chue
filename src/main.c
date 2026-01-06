#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <ncurses.h>

#define ESC 17


time_t now = 0;
time_t poll = 0;

typedef struct light {
    char *id;
    char *name;
    bool on;
    double brightness;
} light;

light *lights = NULL;
int lights_size = 0; // TODO size_t?

int selected = 0;
int windex = 0;
int wsize = 0;

void curl_hue_light_toggle();

typedef struct Light {
    char id[37];
    char *name;
    bool on;
    double brightness;
} Light;

typedef struct LightList {
    Light *items;
    size_t count;
} LightList;

size_t light_cols(void *data) {
    (void)data;
    return 3;
}

size_t light_rows(void *data) {
    return ((LightList *)data)->count;
}

void light_header(void *data, size_t col, char *buf, size_t size) {
    if (col == 0) {
        snprintf(buf, size, "%s", "NAME");
    } else if (col == 1) {
        snprintf(buf, size, "%s", "STATE");
    } else if (col == 2) {
        snprintf(buf, size, "%s", "BRIGHTNESS");
    }
}

void light_text(void *data, size_t col, size_t row, char *buf, size_t size) {
    Light *light = &(((LightList *)data)->items[row]);

    if (col == 0) {
        snprintf(buf, size, "%s", light->name);
    } else if (col == 1) {
        snprintf(buf, size, "%s", light->on ? "on" : "off");
    } else if (col == 2) {
        snprintf(buf, size, "%*.0lf%%", 3, light->brightness);
    }
}

void light_select(void *data, size_t row) {
    Light *light = &(((LightList *)data)->items[row]);
    // TODO
    // curl_hue_light_toggle();
}

typedef struct Room {
    char id[37];
    char *name;
} Room;

typedef struct RoomList {
    Room *items;
    size_t count;
} RoomList;

// TODO add const
typedef struct TableBehavior {
    size_t (*cols)(void *data);
    size_t (*rows)(void *data);
    void (*header)(void *data, size_t col, char *buf, size_t size);
    void (*text)(void *data, size_t col, size_t row, char *buf, size_t size);
    void (*select)(void *data, size_t row);
} TableBehavior;

typedef struct Table {
    void *data;
    TableBehavior *behavior;

    size_t sindex;
    size_t windex;
    size_t wsize;
} Table;

void table_draw(WINDOW *w, Table *t) {
    size_t cols = t->behavior->cols(t->data);
    size_t rows = t->behavior->rows(t->data);

    size_t maxy = getmaxy(w);
    size_t maxx = getmaxx(w);

    size_t colwidth = (maxx - 2) / cols;

    box(w, 0, 0);
    char title[maxy - 2];
    sprintf(title, "%s[%ld]", "lights", ((LightList *)t->data)->count);
    mvwprintw(w, 0, ((maxx - strlen(title)) / 2), " %s ", title);

    wmove(w, 1, 1);
    char buf[64];
    for (size_t col = 0; col < cols; col++) {
        t->behavior->header(t->data, col, buf, sizeof buf);
        wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
    }

    for (size_t row = 0; (row < rows) && (row < (t->wsize)); row++) {
        wmove(w, (row + 2), 1);
        
        if (row == t->sindex) {
            mvwhline(w, (row + 2), 1, ' ', (maxx - 2));
        }

        for (size_t col = 0; col < cols; col++) {
            t->behavior->text(t->data, col, (row + t->windex), buf, sizeof buf);
            
            if ((row + t->windex) == t->sindex) {
                wattrset(w, A_REVERSE);
                wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
                wattrset(w, A_NORMAL);
            } else {
                wprintw(w, "%-*.*s", colwidth, (colwidth - 1), buf);
            }
        }
    }
}

void table_resize(WINDOW *w, Table *t) {
    t->wsize = getmaxy(w) - 3;
}

void table_up(Table *t) {
    size_t rows = t->behavior->rows(t->data);

    size_t wmargin = 4;
    size_t ls = ((LightList *)t->data)->count;

    if (t->windex > 0) {
        if (t->sindex > (t->windex + (4 - 1))) {
            t->sindex--;
        } else {
            t->sindex--;
            t->windex--;
        }
    } else {
        if (t->sindex > 0) {
            t->sindex--;
        } else {
            t->sindex = (ls - 1);

            if (t->wsize < ls) {
                t->windex = (ls - t->wsize);
            }
        }
    }
}

void table_down(Table *t) {
    size_t rows = t->behavior->rows(t->data);

    size_t wmargin = 4;
    size_t ls = ((LightList *)t->data)->count;

    if ((t->windex + t->wsize) < ls) {
        if (t->sindex < (t->windex + t->wsize - 4)) {
            t->sindex++;
        } else {
            t->sindex++;
            t->windex++;
        }
    } else {
        if (t->sindex < (ls - 1)) {
            t->sindex++;
        } else {
            t->sindex = 0;
            t->windex = 0;
        }
    }
}

void table_top(Table *t) {
    t->sindex = 0;
    t->windex = 0;
}

void table_bottom(Table *t) {
    size_t ls = ((LightList *)t->data)->count;
    t->sindex = (ls - 1);
    if (ls > t->wsize) {
        t->windex = (ls - t->wsize);
    }
}

void table_select(Table *t) {
    t->behavior->select(t->data, t->sindex);
}

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


struct buffer {
    char *data;
    size_t len;
};

void curl_init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void curl_end() {
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
    Light *light = &((LightList *)t->data)->items[selected];
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

static WINDOW *wheader = NULL;
static WINDOW *wcontent = NULL;

void draw_header() {
    if (wheader == NULL) {
        int starty = 0;
        int startx = 0;
        int height = 4;
        int width = COLS;
        wheader = newwin(height, width, starty, startx);
    }

    mvwprintw(wheader, 0, 0, "chue 0.0.1");
    // mvwprintw(wheader, 1, 0, "win s%3d wi%3d ws%3d ls%3d", t->sindex, t->windex, t->wsize, 999); // TODO fix
    // mvwprintw(wheader, 2, 0, "time %ld", now);

    mvwprintw(wheader, 0, (COLS - 17), "     _           ");
    mvwprintw(wheader, 1, (COLS - 17), " ___| |_ _ _ ___ ");
    mvwprintw(wheader, 2, (COLS - 17), "|  _|   | | | -_|");
    mvwprintw(wheader, 3, (COLS - 17), "|___|_|_|___|___|");

    wnoutrefresh(wheader);
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
            curl_hue_lights_read(); // TODO ui hangs
            poll = now;
        }

        draw();
    } while ((ch = getch()));
}

int ncurses_init() {
    initscr();
    timeout(50);
    // raw();
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

    endwin();
}

void init() {
    ll = calloc(1, sizeof(LightList));
    ll->items = NULL;
    ll->count = 0;

    b = calloc(1, sizeof(TableBehavior));
    b->cols = light_cols;
    b->rows = light_rows;
    b->header = light_header;
    b->text = light_text;
    b->select = light_select;

    t = calloc(1, sizeof(Table));
    t->behavior = b;
    t->data = ll;
    t->sindex = 0;
    t->windex = 0;
    t->wsize = 0;

    curl_init();
    ncurses_init();
}

void end() {
    free(((LightList *)t->data)->items);
    free(t->behavior);
    free(t);

    curl_end();
    ncurses_end();
}

int main() {
    init();
    loop();
    end();

    return 0;
}
