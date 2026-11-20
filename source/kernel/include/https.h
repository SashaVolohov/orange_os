#include "lwip/tcp.h"
#include "bearssl.h"

typedef void (*https_done_cb_t)(int status, const char *response, size_t len, void *user_data);

typedef struct {
    struct tcp_pcb *pcb;
    br_ssl_client_context sc;
    br_x509_minimal_context xc;
    unsigned char iobuf[BR_SSL_BUFSIZE_BIDI]; 
    uint8_t rx_queue[16384];
    size_t rx_len;

    const char *current_host;
    const char *current_path;
    int request_sent;
    int is_active;

    char *response_buf;
    size_t response_len;
    size_t response_capacity;
    int is_finished;

    https_done_cb_t on_done;
    void *user_data;

} https_client_t;

void https_poll(https_client_t *client);

extern https_client_t https_client;

err_t https_on_tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t https_on_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void https_init(uint32_t days, uint32_t seconds);
void https_on_tcp_err(void *arg, err_t err);