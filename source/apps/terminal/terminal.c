#include "core.h"
#include "terminal.h"

#include "Am79C973.h"
#include "version.h"

#include <string.h>

#define TERM_MAX_LINES  18
#define TERM_MAX_COLS   64
#define TERM_INPUT_MAX  60
#define TERM_MAX_ARGS   8

static char cmd_history[TERM_HISTORY_MAX][TERM_INPUT_MAX];
static int history_count = 0;
static int history_index = 0;
static int is_extended_key = 0;

static char term_lines[TERM_MAX_LINES][TERM_MAX_COLS];
static int  term_line_count = 0;

static char term_input[TERM_INPUT_MAX];
static int  term_input_len = 0;

static int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void k_strncpy(char* dst, const char* src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void term_set_input(const char* str) {
    k_strncpy(term_input, str, TERM_INPUT_MAX);
    term_input[TERM_INPUT_MAX - 1] = '\0';
    
    term_input_len = 0;
    while (term_input[term_input_len] != '\0') {
        term_input_len++;
    }
}

static void term_history_add(const char* cmd) {
    if (cmd[0] == '\0') return;

    if (history_count > 0 && k_strcmp(cmd_history[history_count - 1], cmd) == 0) {
        history_index = history_count;
        return;
    }

    if (history_count == TERM_HISTORY_MAX) {
        for (int i = 1; i < TERM_HISTORY_MAX; i++) {
            k_strncpy(cmd_history[i - 1], cmd_history[i], TERM_INPUT_MAX);
        }
        history_count--;
    }

    k_strncpy(cmd_history[history_count], cmd, TERM_INPUT_MAX);
    cmd_history[history_count][TERM_INPUT_MAX - 1] = '\0';
    history_count++;
    
    history_index = history_count;
}

void term_print(const char* text) {
    if (term_line_count < TERM_MAX_LINES) {
        k_strncpy(term_lines[term_line_count], text, TERM_MAX_COLS);
        term_line_count++;
    } else {
        for (int i = 1; i < TERM_MAX_LINES; i++) {
            k_strncpy(term_lines[i - 1], term_lines[i], TERM_MAX_COLS);
        }
        k_strncpy(term_lines[TERM_MAX_LINES - 1], text, TERM_MAX_COLS);
    }
}

void term_printf(const char *fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    term_print(buffer);
}

typedef void (*cmd_handler_t)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* desc;
    cmd_handler_t handler;
} term_command_t;

extern term_command_t term_commands[];
extern const int term_commands_count;

void cmd_help(int argc, char** argv) {
    term_print("Available commands:");
    for (int i = 0; i < term_commands_count; i++) {
        char buf[TERM_MAX_COLS];
        int pos = 0;
        const char* p = "  ";
        while (*p && pos < TERM_MAX_COLS - 1) buf[pos++] = *p++;
        p = term_commands[i].name;
        while (*p && pos < TERM_MAX_COLS - 1) buf[pos++] = *p++;
        p = " - ";
        while (*p && pos < TERM_MAX_COLS - 1) buf[pos++] = *p++;
        p = term_commands[i].desc;
        while (*p && pos < TERM_MAX_COLS - 1) buf[pos++] = *p++;
        buf[pos] = '\0';
        
        term_print(buf);
    }
}

void cmd_clear(int argc, char** argv) {
    term_line_count = 0;
}

void cmd_echo(int argc, char** argv) {
    char out[TERM_MAX_COLS];
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        while (*arg && pos < TERM_MAX_COLS - 1) {
            out[pos++] = *arg++;
        }
        if (i < argc - 1 && pos < TERM_MAX_COLS - 1) {
            out[pos++] = ' ';
        }
    }
    out[pos] = '\0';
    term_print(out);
}

volatile uint8_t dns_found = 0;

static void on_dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    (void)callback_arg;

    char dns_info[100];

    if (ipaddr != NULL) {
        uint32_t ip = ip4_addr_get_u32(ip_2_ip4(ipaddr));

        ksnprintf(dns_info, 100, "DNS %s -> %u.%u.%u.%u", name, ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

    } else {
        ksnprintf(dns_info, 100, "DNS failed for %s", name);
    }

    term_print(dns_info);

    dns_found = 1;

}

void cmd_nslookup(int argc, char** argv) {
    if (argc < 2) {
        term_print("Usage: nslookup <domain>");
        return;
    }
    term_print("Resolving domain...");
    
    ip_addr_t resolved_ip;
    err_t err = dns_gethostbyname(argv[1], &resolved_ip, on_dns_found, NULL);

    if (err == ERR_OK) {
        on_dns_found(argv[1], &resolved_ip, NULL);
    } else if (err == ERR_INPROGRESS) {
        
    } else {
        term_print("DNS Error.");
    }
}

static struct raw_pcb *ping_pcb = NULL;
static ip_addr_t ping_target;
static uint16_t ping_seq_num = 0;
static uint32_t ping_send_time = 0;
static int ping_in_progress = 0;

static void ping_send_next(void *arg);

static u8_t ping_recv_cb(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    (void)arg;
    (void)pcb;

    if (p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) {
        struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
        uint16_t iphdr_len = IPH_HL_BYTES(iphdr);

        if (p->tot_len >= iphdr_len + sizeof(struct icmp_echo_hdr)) {
            struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)((uint8_t *)p->payload + iphdr_len);

            if ((iecho->type == ICMP_ER) &&
                (iecho->id == lwip_htons(PING_ID)) &&
                (iecho->seqno == lwip_htons(ping_seq_num))) {

                uint32_t rtt = sys_now() - ping_send_time;
                uint32_t ip = ip4_addr_get_u32(ip_2_ip4(addr));

                term_printf("Reply from %u.%u.%u.%u: bytes=%d time=%u ms TTL=%u\n",
                                ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF,
                                PING_DATA_SIZE, rtt, IPH_TTL(iphdr));

                pbuf_free(p);
                return 1;
            }
        }
    }
    return 0;
}

static void ping_send_packet(void) {
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    struct pbuf *p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) return;

    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = lwip_htons(PING_ID);
    iecho->seqno  = lwip_htons(++ping_seq_num);

    for (size_t i = 0; i < PING_DATA_SIZE; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)('a' + (i % 23));
    }

    iecho->chksum = inet_chksum(iecho, (u16_t)ping_size);

    ping_send_time = sys_now();
    raw_sendto(ping_pcb, p, &ping_target);
    pbuf_free(p);
}

static void ping_send_next(void *arg) {
    (void)arg;

    if (ping_seq_num < PING_COUNT) {
        ping_send_packet();
        sys_timeout(1000, ping_send_next, NULL);
    } else {
        raw_remove(ping_pcb);
        ping_pcb = NULL;
        ping_in_progress = 0;
        term_printf("Ping finished.\n");
    }
}

static void ping_start_ip(const ip_addr_t *ipaddr) {
    ping_target = *ipaddr;
    ping_seq_num = 0;

    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (!ping_pcb) {
        term_printf("Ping: failed to allocate raw_pcb\n");
        ping_in_progress = 0;
        return;
    }

    raw_recv(ping_pcb, ping_recv_cb, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);

    uint32_t ip = ip4_addr_get_u32(ip_2_ip4(&ping_target));
    term_printf("Pinging %u.%u.%u.%u with %d bytes of data:\n",
                    ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF, PING_DATA_SIZE);

    ping_send_next(NULL);
}

static void ping_dns_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    (void)callback_arg;
    if (ipaddr != NULL) {
        ping_start_ip(ipaddr);
    } else {
        term_printf("Ping: could not find host %s\n", name);
        ping_in_progress = 0;
    }
}

void cmd_ping(int argc, char** argv) {

    if (argc < 2) {
        term_print("Usage: ping <domain>");
        return;
    }

    if (ping_in_progress) {
        term_print("Ping is already running...\n");
        return;
    }

    ping_in_progress = 1;
    ip_addr_t addr;
    err_t err = dns_gethostbyname(argv[1], &addr, ping_dns_cb, NULL);

    if (err == ERR_OK) {
        ping_start_ip(&addr);
    } else if (err != ERR_INPROGRESS) {
        term_printf("Ping: invalid host or DNS error (%d)\n", err);
        ping_in_progress = 0;
    }

}

static struct tcp_pcb *curl_pcb = NULL;
static char curl_host[128];
static char curl_path[128];
static int curl_in_progress = 0;
static int curl_headers_done = 0;

static void curl_close(struct tcp_pcb *tpcb) {
    if (tpcb != NULL) {
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_err(tpcb, NULL);
        tcp_close(tpcb);
    }
    curl_pcb = NULL;
    curl_in_progress = 0;
}

static void curl_err_cb(void *arg, err_t err) {
    (void)arg;
    term_printf("\n[TCP Error: %d]\n", err);
    curl_pcb = NULL;
    curl_in_progress = 0;
}

static err_t curl_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;

    if (p == NULL) {
        term_printf("\n[Connection closed by host]\n");
        curl_close(tpcb);
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    tcp_recved(tpcb, p->tot_len);

    if (!curl_headers_done) {
        char line_buf[1024];
        int line_idx = 0;

        struct pbuf *q = p;
        while (q != NULL && !curl_headers_done) {
            const char *data = (const char *)q->payload;
            for (u16_t i = 0; i < q->len; i++) {
                char c = data[i];
                if (c == '\r') continue;

                static char prev_char = 0;
                if (c == '\n' && prev_char == '\n') {
                    if (line_idx > 0) {
                        line_buf[line_idx] = '\0';
                        term_printf("%s\n", line_buf);
                        line_idx = 0;
                    }
                    curl_headers_done = 1;
                    term_printf("[End of HTTP headers]\n");
                    break;
                }
                prev_char = c;

                if (c == '\n') {
                    line_buf[line_idx] = '\0';
                    term_printf("%s\n", line_buf);
                    line_idx = 0;
                } else {
                    if (line_idx < (int)sizeof(line_buf) - 1) {
                        line_buf[line_idx++] = c;
                    }
                }
            }
            q = q->next;
        }

        if (line_idx > 0 && !curl_headers_done) {
            line_buf[line_idx] = '\0';
            term_printf("%s", line_buf);
        }
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t curl_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;

    if (err != ERR_OK) {
        term_printf("Connect failed: %d\n", err);
        curl_close(tpcb);
        return err;
    }

    term_printf("Connected! Sending HTTP request...\n\n");

    char request[512];
    strcpy(request, "GET ");
    strcat(request, curl_path);
    strcat(request, " HTTP/1.0\r\nHost: ");
    strcat(request, curl_host);
    strcat(request, "\r\nUser-Agent: OrangeOS/1.2\r\nConnection: close\r\n\r\n");

    err_t wr_err = tcp_write(tpcb, request, (u16_t)strlen(request), TCP_WRITE_FLAG_COPY);
    if (wr_err == ERR_OK) {
        tcp_output(tpcb);
    } else {
        term_printf("tcp_write error: %d\n", wr_err);
        curl_close(tpcb);
    }

    return ERR_OK;
}

static void curl_dns_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    (void)callback_arg;

    if (ipaddr == NULL) {
        term_printf("curl: could not resolve host %s\n", name);
        curl_in_progress = 0;
        return;
    }

    curl_pcb = tcp_new();
    if (curl_pcb == NULL) {
        term_printf("curl: out of memory for tcp_pcb\n");
        curl_in_progress = 0;
        return;
    }

    tcp_recv(curl_pcb, curl_recv_cb);
    tcp_err(curl_pcb, curl_err_cb);

    uint32_t ip = ip4_addr_get_u32(ip_2_ip4(ipaddr));
    term_printf("Connecting to %s (%u.%u.%u.%u:80)...\n",
                    name, ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

    err_t err = tcp_connect(curl_pcb, ipaddr, 80, curl_connected_cb);
    if (err != ERR_OK) {
        term_printf("tcp_connect error: %d\n", err);
        curl_close(curl_pcb);
    }
}

void cmd_curl(int argc, char** argv) {

    if (argc < 2) {
        term_print("Usage: curl <domain>");
        return;
    }

    if (curl_in_progress) {
        term_printf("curl is already running...\n");
        return;
    }

    if (strncmp(argv[1], "http://", 7) == 0) {
        argv[1] += 7;
    }

    const char *slash = strchr(argv[1], '/');
    if (slash != NULL) {
        size_t host_len = (size_t)(slash - argv[1]);
        if (host_len >= sizeof(curl_host)) host_len = sizeof(curl_host) - 1;
        memcpy(curl_host, argv[1], host_len);
        curl_host[host_len] = '\0';

        strncpy(curl_path, slash, sizeof(curl_path) - 1);
        curl_path[sizeof(curl_path) - 1] = '\0';
    } else {
        strncpy(curl_host, argv[1], sizeof(curl_host) - 1);
        curl_host[sizeof(curl_host) - 1] = '\0';
        strcpy(curl_path, "/");
    }

    curl_in_progress = 1;
    curl_headers_done = 0;

    ip_addr_t addr;
    err_t err = dns_gethostbyname(curl_host, &addr, curl_dns_cb, NULL);
    if (err == ERR_OK) {
        curl_dns_cb(curl_host, &addr, NULL);
    } else if (err != ERR_INPROGRESS) {
        term_printf("curl: DNS error %d\n", err);
        curl_in_progress = 0;
    }
}

term_command_t term_commands[] = {
    {"help",     "Show this help message", cmd_help},
    {"clear",    "Clear terminal screen",  cmd_clear},
    {"echo",     "Print arguments",        cmd_echo},
    {"nslookup", "DNS lookup tool",        cmd_nslookup},
    {"ping", "Ping some server", cmd_ping},
    {"curl", "Get's HTTP headers", cmd_curl}
};
const int term_commands_count = sizeof(term_commands) / sizeof(term_command_t);

static void term_execute(char* cmd_line) {
    char* argv[TERM_MAX_ARGS];
    int argc = 0;
    char* p = cmd_line;

    while (*p != '\0' && argc < TERM_MAX_ARGS) {
        while (*p == ' ') p++;
        if (*p == '\0') break;

        argv[argc++] = p;

        while (*p != '\0' && *p != ' ') p++;
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    if (argc == 0) return;

    for (int i = 0; i < term_commands_count; i++) {
        if (k_strcmp(argv[0], term_commands[i].name) == 0) {
            term_commands[i].handler(argc, argv);
            return;
        }
    }

    term_print("Unknown command. Type 'help'.");
}

void on_terminal_draw(const struct window* win, int body_x, int body_y, int body_w, int body_h) {

    draw_window_rect(win, 0, 0, win->width, body_h, 18, 18, 22, 255);

    int line_height = 15;

    for (int i = 0; i < term_line_count; i++) {
        win_printf(win, 6, 6 + (i * line_height),
                   220, 220, 220, 255, "%s", term_lines[i]);
    }

    int input_y = 6 + (term_line_count * line_height);
    win_printf(win, 6, input_y,
               80, 250, 123, 255, "> %s_", term_input);
}

void on_terminal_key(const struct window* win, char key) {
    unsigned char ukey = (unsigned char)key;

    if (!is_extended_key && (ukey == 0x00 || ukey == 0xE0)) {
        is_extended_key = 1;
        return;
    }

    if (is_extended_key) {
        is_extended_key = 0;

        if (ukey == 0x48) {
            if (history_index > 0) {
                history_index--;
                term_set_input(cmd_history[history_index]);
            }
        } else if (ukey == 0x50) {
            if (history_index < history_count) {
                history_index++;
                if (history_index == history_count) {
                    term_set_input("");
                } else {
                    term_set_input(cmd_history[history_index]);
                }
            }
        }
        return;
    }

    if (key == '\n' || key == '\r') {
        term_input[term_input_len] = '\0';

        term_history_add(term_input);

        char echo_line[TERM_MAX_COLS];
        echo_line[0] = '>';
        echo_line[1] = ' ';
        k_strncpy(&echo_line[2], term_input, TERM_MAX_COLS - 2);
        term_print(echo_line);
        term_execute(term_input);

        term_input_len = 0;
        term_input[0] = '\0';
        return;
    }

    if (key == '\b' || key == 127) {
        if (term_input_len > 0) {
            term_input_len--;
            term_input[term_input_len] = '\0';
        }
        return;
    }

    if (ukey >= 32 && ukey <= 126) {
        if (term_input_len < TERM_INPUT_MAX - 1) {
            term_input[term_input_len++] = key;
            term_input[term_input_len] = '\0';
        }
    }
}

void open_terminal_app() {
    term_line_count = 0;
    term_input_len = 0;
    term_input[0] = '\0';

    term_printf("OrangeOS Version Alpha 1.3 (Build %d, %s)", ORANGEOS_BUILD, ORANGEOS_BUILD_DATE);

    char equipment_info[100];
    ksnprintf(equipment_info, 100, "%d System Processors [%d MB Memory] MultiProcessor Kernel", boot_info->cpu_count, mb_memory);
    term_print(equipment_info);

    int win_id = createWindow(80, 60, 520, 320, "Terminal", on_terminal_draw, NULL);
    set_window_key_callback(win_id, on_terminal_key);
}