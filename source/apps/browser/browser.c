#include "browser.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "Am79C973.h"
#include "https.h"

int https_mode = 0;
int browser_id;

static BrowserState browser;

static int tag_match(const char* html, const char* tag) {
    int i = 0;
    while (tag[i]) {
        char c1 = html[i];
        char c2 = tag[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 0;
        i++;
    }
    char next = html[i];
    return (next == '\0' || next == '>' || next == ' ' || next == '\t' || next == '\n' || next == '\r' || next == '/');
}

static void add_control_item(ItemType type) {
    if (browser.item_count >= MAX_RENDER_ITEMS) return;
    if (browser.item_count > 0) {
        ItemType last = browser.items[browser.item_count - 1].type;
        if ((type == ITEM_NEWLINE || type == ITEM_PARAGRAPH) && 
            (last == ITEM_PARAGRAPH || last == ITEM_LI)) {
            return;
        }
    }
    RenderItem* item = &browser.items[browser.item_count++];
    item->type = type;
    item->text[0] = '\0';
    item->href[0] = '\0';
}

void resolve_url(const char* base_url, const char* href, char* out_url, size_t max_len) {
    if (strncmp(href, "http://", 7) == 0) {
        strncpy(out_url, href, max_len - 1);
        out_url[max_len - 1] = '\0';
        return;
    }

    if (strncmp(href, "https://", 8) == 0) {
        strncpy(out_url, href, max_len - 1);
        out_url[max_len - 1] = '\0';
        return;
    }

    char host_part[256] = {0};
    const char* path_start = NULL;

    if (strncmp(base_url, "http://", 7) == 0) {
        const char* slash = base_url + 7;
        while (*slash && *slash != '/') slash++;
        size_t host_len = slash - base_url;
        if (host_len >= sizeof(host_part)) host_len = sizeof(host_part) - 1;
        strncpy(host_part, base_url, host_len);
        host_part[host_len] = '\0';
        path_start = (*slash == '/') ? slash : "/";
    } else if (strncmp(base_url, "https://", 8) == 0) {
        const char* slash = base_url + 8;
        while (*slash && *slash != '/') slash++;
        size_t host_len = slash - base_url;
        if (host_len >= sizeof(host_part)) host_len = sizeof(host_part) - 1;
        strncpy(host_part, base_url, host_len);
        host_part[host_len] = '\0';
        path_start = (*slash == '/') ? slash : "/";
    } else {
        ksnprintf(host_part, sizeof(host_part), "http://%s", base_url);
        path_start = "/";
    }

    if (href[0] == '/') {
        ksnprintf(out_url, max_len, "%s%s", host_part, href);
        return;
    }

    char dir_path[256];
    strncpy(dir_path, path_start, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';

    char* last_slash = NULL;
    for (char* p = dir_path; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    if (last_slash) {
        *(last_slash + 1) = '\0';
    } else {
        strcpy(dir_path, "/");
    }

    ksnprintf(out_url, max_len, "%s%s%s", host_part, dir_path, href);
}

void parse_html(const char* html) {
    browser.item_count = 0;
    browser.scroll_y = 0;

    ItemType current_type = ITEM_TEXT;
    char current_href[256] = {0};
    int in_invisible = 0;
    int in_list = 0;
    int i = 0;

    int in_title = 0;
    int title_idx = 0;
    browser.page_title[0] = '\0';

    while (html[i] && browser.item_count < MAX_RENDER_ITEMS) {
        if (html[i] == '<') {
            const char* tag = &html[i] + 1;

            if (strncmp(tag, "!--", 3) == 0) {
                const char* end_comment = strstr(tag + 3, "-->");
                if (end_comment) {
                    i = (end_comment - html) + 3;
                    continue;
                }
            }

            if (tag_match(tag, "head") || tag_match(tag, "style") || tag_match(tag, "script")) {
                in_invisible = 1;
            } else if (tag_match(tag, "/head") || tag_match(tag, "/style") || tag_match(tag, "/script")) {
                in_invisible = 0;
            } 

            if (tag_match(tag, "title")) {
                in_title = 1;
                title_idx = 0;
            } else if (tag_match(tag, "/title")) {
                in_title = 0;
                browser.page_title[title_idx] = '\0';
                uint64_t new_title = (uint64_t)kmalloc(256);
                if(new_title != 0) {
                    ksnprintf((char*)new_title, 256, "%s - Browser", browser.page_title);
                    set_window_title(browser_id, (const char*)new_title);
                }
            }

            else if (!in_invisible) {
                if (tag_match(tag, "h1") || tag_match(tag, "h2") || tag_match(tag, "h3")) {
                    add_control_item(ITEM_PARAGRAPH);
                    current_type = ITEM_HEADER;
                } else if (tag_match(tag, "/h1") || tag_match(tag, "/h2") || tag_match(tag, "/h3")) {
                    current_type = ITEM_TEXT;
                    add_control_item(ITEM_PARAGRAPH);
                } else if (tag_match(tag, "p") || tag_match(tag, "div") || tag_match(tag, "/div")) {
                    add_control_item(ITEM_PARAGRAPH);
                } else if (tag_match(tag, "br")) {
                    add_control_item(ITEM_NEWLINE);
                } else if (tag_match(tag, "hr")) {
                    add_control_item(ITEM_HR);
                } else if (tag_match(tag, "ul") || tag_match(tag, "ol")) {
                    in_list = 1;
                    add_control_item(ITEM_NEWLINE);
                } else if (tag_match(tag, "/ul") || tag_match(tag, "/ol")) {
                    in_list = 0;
                    add_control_item(ITEM_PARAGRAPH);
                } else if (tag_match(tag, "li")) {
                    add_control_item(ITEM_LI);
                } else if (tag_match(tag, "b") || tag_match(tag, "strong")) {
                    if (current_type == ITEM_TEXT) current_type = ITEM_BOLD;
                } else if (tag_match(tag, "/b") || tag_match(tag, "/strong")) {
                    if (current_type == ITEM_BOLD) current_type = ITEM_TEXT;
                } else if (tag_match(tag, "code") || tag_match(tag, "pre")) {
                    if (current_type == ITEM_TEXT) current_type = ITEM_CODE;
                } else if (tag_match(tag, "/code") || tag_match(tag, "/pre")) {
                    if (current_type == ITEM_CODE) current_type = ITEM_TEXT;
                } else if (tag_match(tag, "a")) {
                    current_type = ITEM_LINK;
                    current_href[0] = '\0';
                    const char* p = tag + 1;
                    while (*p && *p != '>') {
                        if ((strncmp(p, "href=", 5) == 0 || strncmp(p, "HREF=", 5) == 0)) {
                            p += 5;
                            char quote = 0;
                            if (*p == '"' || *p == '\'') quote = *p++;
                            int h_idx = 0;
                            while (*p && *p != '>' && (quote ? *p != quote : *p != ' ') && h_idx < 255) {
                                current_href[h_idx++] = *p++;
                            }
                            current_href[h_idx] = '\0';
                            break;
                        }
                        p++;
                    }
                } else if (tag_match(tag, "/a")) {
                    current_type = ITEM_TEXT;
                }
            }

            while (html[i] && html[i] != '>') i++;
            if (html[i] == '>') i++;
        } else {

            if (in_title) {
                if (title_idx < 127 && html[i] != '\n' && html[i] != '\r') {
                    browser.page_title[title_idx++] = html[i];
                }
                i++;
                continue;
            }

            if (in_invisible) {
                i++;
                continue;
            }

            while (html[i] == ' ' || html[i] == '\n' || html[i] == '\r' || html[i] == '\t') i++;
            if (html[i] == '<' || html[i] == '\0') continue;

            char word[128];
            int w_idx = 0;

            while (html[i] && html[i] != '<' && html[i] != ' ' && html[i] != '\n' && html[i] != '\r' && html[i] != '\t' && w_idx < 127) {
                if (html[i] == '&') {
                    if (strncmp(&html[i], "&nbsp;", 6) == 0) { word[w_idx++] = ' '; i += 6; continue; }
                    if (strncmp(&html[i], "&lt;", 4) == 0)   { word[w_idx++] = '<'; i += 4; continue; }
                    if (strncmp(&html[i], "&gt;", 4) == 0)   { word[w_idx++] = '>'; i += 4; continue; }
                    if (strncmp(&html[i], "&amp;", 5) == 0)  { word[w_idx++] = '&'; i += 5; continue; }
                    if (strncmp(&html[i], "&quot;", 6) == 0) { word[w_idx++] = '"'; i += 6; continue; }
                }
                word[w_idx++] = html[i++];
            }
            word[w_idx] = '\0';

            if (w_idx > 0) {
                RenderItem* item = &browser.items[browser.item_count++];
                item->type = current_type;
                item->indent = in_list ? 20 : 0;
                strncpy(item->text, word, 128);
                strncpy(item->href, current_href, 256);
            }

        }
    }
    browser.last_body_w = -1;
}

void layout_items(int body_w) {
    if (body_w == browser.last_body_w) return;
    browser.last_body_w = body_w;

    int margin = 10;
    int cur_x = margin;
    int cur_y = TOOLBAR_H + 10;

    for (int i = 0; i < browser.item_count; i++) {
        RenderItem* item = &browser.items[i];

        if (item->type == ITEM_NEWLINE) {
            cur_x = margin + item->indent;
            cur_y += LINE_HEIGHT;
            item->w = 0; item->h = 0;
            continue;
        }

        if (item->type == ITEM_PARAGRAPH) {
            cur_x = margin + item->indent;
            cur_y += LINE_HEIGHT + 6;
            item->w = 0; item->h = 0;
            continue;
        }

        if (item->type == ITEM_LI) {
            cur_y += LINE_HEIGHT + 2;
            item->x = margin + 12;
            item->y = cur_y;
            item->w = 6;
            item->h = CHAR_H;
            cur_x = margin + 24;
            continue;
        }

        if (item->type == ITEM_HR) {
            cur_y += LINE_HEIGHT;
            item->x = margin;
            item->y = cur_y;
            item->w = body_w - margin * 2;
            item->h = 2;
            cur_x = margin;
            cur_y += 8;
            continue;
        }

        int word_w = get_string_width(item->text);
        int left_bound = margin + item->indent;

        if (cur_x < left_bound) {
            cur_x = left_bound;
        }

        if (cur_x + word_w > body_w - margin && cur_x > left_bound) {
            cur_x = left_bound;
            cur_y += LINE_HEIGHT;
        }

        item->x = cur_x;
        item->y = cur_y;
        item->w = word_w;
        item->h = CHAR_H;

        cur_x += word_w + CHAR_W;
    }
}

static struct tcp_pcb *browser_pcb = NULL;
static char browser_host[128];
static char browser_path[128];
static int browser_in_progress = 0;
static int browser_headers_done = 0;

static void browser_close(struct tcp_pcb *tpcb) {
    if (tpcb != NULL) {
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_err(tpcb, NULL);
        tcp_close(tpcb);
    }
    browser_pcb = NULL;
    browser_in_progress = 0;
}

static void browser_err_cb(void *arg, err_t err) {
    (void)arg;
    const char* dns_error_html = 
        "<h1>TCP Error</h1>"
        "<p>ERR_CONNECTION_RESET</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
    browser_pcb = NULL;
    browser_in_progress = 0;
}

static err_t browser_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;

    if (err != ERR_OK) {
        const char* dns_error_html = 
        "<h1>This site can't be reached</h1>"
        "<p>ERR_CONNECTION_REFUSED</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
        browser_close(tpcb);
        return err;
    }

    char request[512];
    strcpy(request, "GET ");
    strcat(request, browser_path);
    strcat(request, " HTTP/1.0\r\nHost: ");
    strcat(request, browser_host);
    strcat(request, "\r\nUser-Agent: OrangeOS/1.2\r\nConnection: close\r\n\r\n");

    err_t wr_err = tcp_write(tpcb, request, (u16_t)strlen(request), TCP_WRITE_FLAG_COPY);
    if (wr_err == ERR_OK) {
        tcp_output(tpcb);
    } else {
        const char* dns_error_html = 
        "<h1>TCP Error</h1>"
        "<p>Could not write TCP.</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
        browser_close(tpcb);
    }

    return ERR_OK;
}

int curl_headers_done = 0;

static char *html_buffer = NULL;
static size_t html_size = 0;
static size_t html_capacity = 0;
static char prev_char = 0;

void browser_reset_html() {
    if (html_buffer != NULL) {
        kfree(html_buffer);
        html_buffer = NULL;
    }
    html_size = 0;
    html_capacity = 0;
    prev_char = 0;
    curl_headers_done = 0;
}

static void append_html_data(const char *data, size_t len) {
    if (len == 0) return;

    if (html_buffer == NULL || html_size + len + 1 > html_capacity) {
        size_t new_capacity = (html_capacity == 0) ? 4096 : html_capacity * 2;
        while (new_capacity < html_size + len + 1) {
            new_capacity *= 2;
        }

        char *new_buf = (char *)kmalloc(new_capacity);
        if (!new_buf) return;

        if (html_buffer != NULL) {
            memcpy(new_buf, html_buffer, html_size);
            kfree(html_buffer);
        }

        html_buffer = new_buf;
        html_capacity = new_capacity;
    }

    memcpy(html_buffer + html_size, data, len);
    html_size += len;
    html_buffer[html_size] = '\0';
}


static err_t browser_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;

    if (p == NULL) {
        browser_close(tpcb);
        parse_html(html_buffer);
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    tcp_recved(tpcb, p->tot_len);

    struct pbuf *q = p;
    while (q != NULL) {
        const char *data = (const char *)q->payload;
        u16_t i = 0;

        if (!curl_headers_done) {
            for (; i < q->len; i++) {
                char c = data[i];
                if (c == '\r') continue;

                if (c == '\n' && prev_char == '\n') {
                    curl_headers_done = 1;
                    i++;
                    break;
                }
                prev_char = c;
            }
        }

        if (curl_headers_done && i < q->len) {
            append_html_data(data + i, q->len - i);
        }

        q = q->next;
    }

    pbuf_free(p);
    return ERR_OK;
}

char *http_split_headers_and_body(char *response_buf, size_t total_len, size_t *out_body_len) {
    if (!response_buf || total_len < 4) {
        return NULL;
    }

    for (size_t i = 0; i <= total_len - 4; i++) {
        if (response_buf[i]     == '\r' &&
            response_buf[i + 1] == '\n' &&
            response_buf[i + 2] == '\r' &&
            response_buf[i + 3] == '\n') {
            
            response_buf[i] = '\0';

            size_t body_offset = i + 4;
            if (out_body_len) {
                *out_body_len = total_len - body_offset;
            }

            return response_buf + body_offset;
        }
    }

    return NULL;
}

void on_page_loaded(int status, const char *response, size_t len, void *user_data) {



    if (status == 0) {
        size_t body_len = 0;
        char *body = http_split_headers_and_body(response, len, &body_len);

        if (body != NULL) {
            parse_html(https_client.response_buf);
        }

    } else {
        const char* dns_error_html = 
        "<h1>TLS Error</h1>"
        "<p>Could not resolve host.</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
    }

    browser_in_progress = 0;

    kfree(https_client.response_buf);
}

static void dns_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    (void)callback_arg;

    if (ipaddr == NULL) {
        const char* dns_error_html = 
        "<h1>DNS Error</h1>"
        "<p>Could not resolve host.</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
        return;
    }

    if(https_mode == 0) {

        browser_pcb = tcp_new();
        if (browser_pcb == NULL) {
            const char* dns_error_html = 
            "<h1>Out of memory</h1>"
            "<p>Something went wrong while displaying the webpage.</p>";
            browser_in_progress = 0;
            parse_html(dns_error_html);
            return;
        }

        tcp_recv(browser_pcb, browser_recv_cb);
        tcp_err(browser_pcb, browser_err_cb);

        err_t err = tcp_connect(browser_pcb, ipaddr, 80, browser_connected_cb);
        if (err != ERR_OK) {
            const char* dns_error_html = 
            "<h1>TCP Error</h1>"
            "<p>ERR_CONNECTION_RESET</p>";
            browser_in_progress = 0;
            parse_html(dns_error_html);
            browser_close(browser_pcb);
        }

    } else {
        https_client.response_capacity = 65536;
        https_client.response_buf = (char *)kmalloc(https_client.response_capacity);
        https_client.response_len = 0;
        https_client.is_finished = 0;
        https_client.current_host = browser_host;
        https_client.current_path = browser_path;
        https_client.request_sent = 0;
        https_client.rx_len = 0;
        https_client.pcb = tcp_new();
        https_client.is_active = 1;
        https_client.on_done = on_page_loaded;
        tcp_arg(https_client.pcb, &https_client);
        tcp_recv(https_client.pcb, https_on_tcp_recv);
        tcp_err(https_client.pcb, https_on_tcp_err);
        tcp_connect(https_client.pcb, ipaddr, 443, https_on_tcp_connected);

    }
}

void load_url(const char* url) {
    strncpy(browser.url, url, 256);
    browser.url_len = strlen(browser.url);

    https_mode = 0;

    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }

    if(strncmp(url, "https://", 8) == 0) {
        url += 8;
        https_mode = 1;
    }

    browser_reset_html();

    const char *slash = strchr(url, '/');
    if (slash != NULL) {
        size_t host_len = (size_t)(slash - url);
        if (host_len >= sizeof(browser_host)) host_len = sizeof(browser_host) - 1;
        memcpy(browser_host, url, host_len);
        browser_host[host_len] = '\0';

        strncpy(browser_path, slash, sizeof(browser_path) - 1);
        browser_path[sizeof(browser_path) - 1] = '\0';
    } else {
        strncpy(browser_host, url, sizeof(browser_host) - 1);
        browser_host[sizeof(browser_host) - 1] = '\0';
        strcpy(browser_path, "/");
    }

    browser_in_progress = 1;
    browser_headers_done = 0;

    ip_addr_t addr;
    err_t err = dns_gethostbyname(browser_host, &addr, dns_cb, NULL);
    if (err == ERR_OK) {
        dns_cb(browser_host, &addr, NULL);
    } else if (err != ERR_INPROGRESS) {
        const char* dns_error_html = 
        "<h1>DNS Error</h1>"
        "<p>DNS address could not be found. Diagnosing the problem.</p>";
        browser_in_progress = 0;
        parse_html(dns_error_html);
        return;
    }

}

void browser_on_draw(const struct window* win, int body_x, int body_y, int body_w, int body_h) {
    layout_items(win->width);
    
    draw_window_rect(win, 0, 0, win->width, win->height, 245, 245, 245, 255);
    
    for (int i = 0; i < browser.item_count; i++) {
        RenderItem* item = &browser.items[i];

        if (item->type == ITEM_NEWLINE || item->type == ITEM_PARAGRAPH) {
            continue;
        }

        int draw_y = item->y - browser.scroll_y;
        if (draw_y < TOOLBAR_H || draw_y + item->h > win->height) {
            continue;
        }

        int draw_x = item->x;

        if (item->type == ITEM_LI) {
            draw_window_rect(win, draw_x, draw_y + (CHAR_H / 2) - 2, 4, 4, 40, 40, 40, 255);
        } else if (item->type == ITEM_HR) {
            draw_window_line(win, draw_x, draw_y, draw_x + item->w, draw_y, 160, 160, 160, 255);
        } else if (item->type == ITEM_HEADER) {
            win_printf(win, draw_x, draw_y, 160, 20, 20, 255, "%s", item->text);
            win_printf(win, draw_x + 1, draw_y, 160, 20, 20, 255, "%s", item->text);
            draw_window_line(win, draw_x, draw_y + CHAR_H + 1, draw_x + item->w, draw_y + CHAR_H + 1, 160, 20, 20, 255);
        } else if (item->type == ITEM_BOLD) {
            win_printf(win, draw_x, draw_y, 10, 10, 10, 255, "%s", item->text);
            win_printf(win, draw_x + 1, draw_y, 10, 10, 10, 255, "%s", item->text);
        } else if (item->type == ITEM_CODE) {
            draw_window_rect(win, draw_x - 2, draw_y - 1, item->w + 4, CHAR_H + 2, 230, 230, 235, 255);
            win_printf(win, draw_x, draw_y, 150, 30, 90, 255, "%s", item->text);
        } else if (item->type == ITEM_LINK) {
            win_printf(win, draw_x, draw_y, 0, 80, 220, 255, "%s", item->text);
            draw_window_line(win, draw_x, draw_y + CHAR_H, draw_x + item->w, draw_y + CHAR_H, 0, 80, 220, 255);
        } else {
            win_printf(win, draw_x, draw_y, 20, 20, 20, 255, "%s", item->text);
        }
    }
    
    draw_window_rect(win, 0, 0, win->width, TOOLBAR_H, 210, 210, 210, 255);
    draw_window_line(win, 0, TOOLBAR_H, win->width, TOOLBAR_H, 120, 120, 120, 255);
    
    browser.btn_back.rel_x = 6; browser.btn_back.rel_y = 6; browser.btn_back.width = 28; browser.btn_back.height = 28;
    browser.btn_go.rel_x = win->width - 46; browser.btn_go.rel_y = 6; browser.btn_go.width = 40; browser.btn_go.height = 28;
    
    draw_window_button(win, 0, 0, &browser.btn_back);
    draw_window_button(win, 0, 0, &browser.btn_go);
    
    int url_box_x = 40;
    int url_box_y = 6;
    int url_box_w = win->width - 92;
    int url_box_h = 28;
    
    draw_window_rect(win, url_box_x, url_box_y, url_box_w, url_box_h, 255, 255, 255, 255);
    if (browser.url_focused) {
        draw_window_line(win, url_box_x, url_box_y, url_box_x + url_box_w, url_box_y, 0, 120, 215, 255);
        draw_window_line(win, url_box_x, url_box_y + url_box_h, url_box_x + url_box_w, url_box_y + url_box_h, 0, 120, 215, 255);
    }
    win_printf(win, url_box_x + 6, url_box_y + 6, 0, 0, 0, 255, "%s%s", browser.url, browser.url_focused ? "_" : "");
}

void browser_on_mouse(const struct window* win, int click_x, int click_y, int is_left_pressed) {
    if (!is_left_pressed) return;
    
    if (is_button_clicked(&browser.btn_back, click_x, click_y, is_left_pressed)) {
        browser.scroll_y = 0;
        return;
    }
    if (is_button_clicked(&browser.btn_go, click_x, click_y, is_left_pressed)) {
        browser.url_focused = 0;
        load_url(browser.url);
        return;
    }
    
    if (click_y < TOOLBAR_H && click_x >= 40) {
        browser.url_focused = 1;
        return;
    }
    
    browser.url_focused = 0;
    
    int page_y = click_y + browser.scroll_y;
    for (int i = 0; i < browser.item_count; i++) {
        RenderItem* item = &browser.items[i];
        if (item->type == ITEM_LINK) {
            if (click_x >= item->x && click_x <= item->x + item->w &&
                page_y >= item->y && page_y <= item->y + item->h) {
                char target_url[1024];
                resolve_url(browser_host, item->href, target_url, sizeof(target_url));
                load_url(item->href);
                return;
            }
        }
    }
}

void browser_on_key(const struct window* win, char key) {
    if (browser.url_focused) {
        if (key == '\n' || key == '\r') {
            browser.url_focused = 0;
            load_url(browser.url);
        } else if (key == '\b' || key == 127) {
            if (browser.url_len > 0) {
                browser.url[--browser.url_len] = '\0';
            }
        } else if (key >= 32 && key <= 126 && browser.url_len < 254) {
            browser.url[browser.url_len++] = key;
            browser.url[browser.url_len] = '\0';
        }
    } else {
        if (key == 's' || key == 'S') browser.scroll_y += 20;
        if (key == 'w' || key == 'W') {
            browser.scroll_y -= 20;
            if (browser.scroll_y < 0) browser.scroll_y = 0;
        }
    }
}

void launch_browser() {
    browser_id = createWindow(100, 80, 640, 480, "Browser", browser_on_draw, browser_on_mouse);
    set_window_key_callback(browser_id, browser_on_key);
}