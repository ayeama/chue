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
} light;

light *lights = NULL;
int lights_size = 0; // TODO size_t?

int selected = 0;
int windex = 0;

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
    snprintf(url, (1024 - 1), "https://%s/clip/v2/resource/light", BADDR);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: chue/0.0.1");

    char hauth[1024] = {0};
    snprintf(hauth, (1024 - 1), "hue-application-key: %s", BUSER);
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
                lights_size = cJSON_GetArraySize(data);
                lights = calloc(lights_size, sizeof *lights);
                if (!lights) {
                    printf("FAIL\n");
                }
            }

            int i = 0;
            cJSON_ArrayForEach(item, data) {
                cJSON *id = cJSON_GetObjectItem(item, "id");
                if (cJSON_IsString(id)) {
                    lights[i].id = strdup(id->valuestring);
                }

                cJSON *metadata = cJSON_GetObjectItem(item, "metadata");
                cJSON *name = cJSON_GetObjectItem(metadata, "name");
                if (cJSON_IsString(name)) {
                    lights[i].name = strdup(name->valuestring);
                }

                cJSON *on = cJSON_GetObjectItem(item, "on");
                cJSON *onon = cJSON_GetObjectItem(on, "on");
                if (cJSON_IsBool(onon)) {
                    lights[i].on = onon->valueint;
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
    snprintf(url, (1024 - 1), "https://%s/clip/v2/resource/light/%s", BADDR, lights[selected].id);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: chue/0.0.1");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    char hauth[1024] = {0};
    snprintf(hauth, (1024 - 1), "hue-application-key: %s", BUSER);
    headers = curl_slist_append(headers, hauth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    cJSON *json = cJSON_CreateObject();
    cJSON *on = cJSON_CreateObject();
    cJSON_AddBoolToObject(on, "on", lights[selected].on);
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
    mvwprintw(wheader, 1, 0, "sindex %3d %3d", selected, windex); // TODO fix
    mvwprintw(wheader, 2, 0, "time %ld", now);

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
    }

    int maxy = getmaxy(wcontent);
    int maxx = getmaxx(wcontent);

    /* title */
    //wattrset(wcontent, COLOR_PAIR(1));
    box(wcontent, 0, 0);
    //wattrset(wcontent, A_NORMAL);

    char title[COLS - 2];
    int count = lights_size;
    // sprintf(title, "%s(%s)[%d]", "lights", "all", count);
    sprintf(title, "%s[%d]", "lights", count);
    mvwprintw(wcontent, 0, ((maxx - strlen(title)) / 2), " %s ", title);

    /* table header */
    char *header[] = {"NAME", "STATE"};
    int header_count = 2;
    int header_width = (maxx - 2) / header_count;

    wmove(wcontent, 1, 1);
    for (int i = 0; i < header_count; i++) {
        wprintw(wcontent, "%-*s", header_width, header[i]);
    }

    /* table items */
    for (int i = 0; (i < lights_size) && (i < (maxy - 3)); i++) {
        wmove(wcontent, (2 + i), 1);

        light *l = &lights[i + windex];

        if (i == selected) {
            wattrset(wcontent, A_REVERSE);
            mvwhline(wcontent, (2 + i), 1, ' ', (maxx - 2));
            wprintw(wcontent, "%-*s%-*s", header_width, l->name, header_width, (l->on ? "on" : "off"));
            wattrset(wcontent, A_NORMAL);
        } else {
            wprintw(wcontent, "%-*s%-*s", header_width, l->name, header_width, (l->on ? "on" : "off"));
        }
    }

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
                if (((getmaxy(wcontent) - 3) - 1) < 1) {
                    break;
                }

                // TODO fix
                if (selected >= ((getmaxy(wcontent) - 3) - 4) && ((selected + windex) < (lights_size - 4))) {
                    windex++;
                    break;
                }

                if (selected < ((getmaxy(wcontent) - 3) - 1)) {
                    selected++;
                } else {
                    selected = 0;
                    windex = 0;
                }
                break;
            case 'k': // up
                if (((getmaxy(wcontent) - 3) - 1) < 1) {
                    break;
                }

                // TODO fix
                if ((selected + windex) <= 4 && (windex > 0)) {
                    windex--;
                    break;
                }

                if (selected > 0) {
                    selected--;
                } else {
                    selected = ((getmaxy(wcontent) - 3) - 1);
                    windex = (getmaxy(wcontent) - 3) - selected;
                }
                break;
            case 'l':
                break;
            case ' ': // mark
                lights[selected].on = !lights[selected].on;
                curl_hue_light_toggle();
                break;
            case 'g': // top
                selected = 0;
                windex = 0;
                break;
            case 'G': // bottom
                // TODO hardcoded
                selected = 15;
                windex = 1;
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

                selected = 0;
                windex = 0;
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
    curl_init();
    ncurses_init();
}

void end() {
    free(lights); // TODO fix
    curl_end();
    ncurses_end();
}

int main() {
    init();
    loop();
    end();

    return 0;
}
