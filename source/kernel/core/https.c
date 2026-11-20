#include "https.h"
#include "core.h"

#include "trust_anchors.c"

https_client_t https_client;

static int rdrand64(uint64_t *val) {
    unsigned char ok;
    __asm__ volatile (
        "rdrand %0\n\t"
        "setc %1"
        : "=r" (*val), "=qm" (ok)
    );
    return ok;
}

void get_entropy(void *dst, size_t len) {
    uint8_t *p = (uint8_t *)dst;
    uint64_t rnd;
    
    while (len > 0) {
        int retries = 10;
        while (retries > 0 && !rdrand64(&rnd)) {
            retries--;
        }
        
        if (retries == 0) {
            uint32_t lo, hi;
            __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
            rnd = ((uint64_t)hi << 32) | lo;
        }

        size_t chunk = (len < sizeof(rnd)) ? len : sizeof(rnd);
        for (size_t i = 0; i < chunk; i++) {
            p[i] = (rnd >> (i * 8)) & 0xFF;
        }
        p += chunk;
        len -= chunk;
    }
}

void https_init(uint32_t days, uint32_t seconds) {
    uint8_t seed[32];
    get_entropy(seed, sizeof(seed));

    br_ssl_client_init_full(&https_client.sc, &https_client.xc, TAs, TAs_NUM);
    br_ssl_engine_set_buffer(&https_client.sc.eng, https_client.iobuf, sizeof(https_client.iobuf), 1);
    br_x509_minimal_set_time(&https_client.xc, days, seconds);
    br_ssl_engine_inject_entropy(&https_client.sc.eng, seed, sizeof(seed));
}

static size_t append_str(char *dst, size_t offset, size_t max_len, const char *str) {
    size_t slen = strlen(str);
    if (offset + slen >= max_len) {
        return offset;
    }
    memcpy(dst + offset, str, slen);
    return offset + slen;
}

static void https_finish(https_client_t *client, int status) {

    if (!client->is_active) {
        return;
    }
    client->is_active = 0;

    if (client->pcb != NULL) {
        tcp_arg(client->pcb, NULL);
        tcp_recv(client->pcb, NULL);
        tcp_err(client->pcb, NULL);
        tcp_close(client->pcb);
        client->pcb = NULL;
    }

    if (client->on_done != NULL) {
        client->on_done(status, client->response_buf, client->response_len, client->user_data);
    }
}

err_t https_on_tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)tpcb;
    https_client_t *client = (https_client_t *)arg;
    
    if (err != ERR_OK) {
        return err;
    }

    client->is_active = 1;
    br_ssl_client_reset(&client->sc, client->current_host, 0);
    return ERR_OK;
}

err_t https_on_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    https_client_t *client = (https_client_t *)arg;

    if (p == NULL) {
        tcp_close(tpcb);
        client->pcb = NULL;
        https_finish(client, 0);
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    size_t space_left = sizeof(client->rx_queue) - client->rx_len;
    if (p->tot_len <= space_left) {
        pbuf_copy_partial(p, client->rx_queue + client->rx_len, p->tot_len, 0);
        client->rx_len += p->tot_len;
        
        tcp_recved(tpcb, p->tot_len);
    } else {
        return ERR_MEM;
    }

    pbuf_free(p);
    return ERR_OK;
}

void https_on_tcp_err(void *arg, err_t err) {

    https_client_t *client = (https_client_t *)arg;
    if (client) {
        client->pcb = NULL;
        https_finish(client, (int)err);
    }
}

void https_poll(https_client_t *client) {

    if (!client->is_active || client->pcb == NULL) {
        return;
    }

    unsigned state = br_ssl_engine_current_state(&client->sc.eng);

    if ((state & BR_SSL_SENDAPP) && !client->request_sent) {
        size_t max_len;
        unsigned char *buf = br_ssl_engine_sendapp_buf(&client->sc.eng, &max_len);
        char *req = (char *)buf;
        size_t pos = 0;

        pos = append_str(req, pos, max_len, "GET ");
        pos = append_str(req, pos, max_len, client->current_path ? client->current_path : "/");
        pos = append_str(req, pos, max_len, " HTTP/1.1\r\nHost: ");
        pos = append_str(req, pos, max_len, client->current_host);
        pos = append_str(req, pos, max_len, "\r\nUser-Agent: OrangeOS/1.2\r\nConnection: close\r\n\r\n");

        if (pos > 0) {
            br_ssl_engine_sendapp_ack(&client->sc.eng, pos);
            
            br_ssl_engine_flush(&client->sc.eng, 0);
            
            client->request_sent = 1;
        }
    }

    if (state & BR_SSL_SENDREC) {
        size_t len;
        
        unsigned char *buf = br_ssl_engine_sendrec_buf(&client->sc.eng, &len);
        
        u16_t sndbuf = tcp_sndbuf(client->pcb);
        if (sndbuf > 0) {
            size_t to_write = (len < sndbuf) ? len : sndbuf;
            if (tcp_write(client->pcb, buf, to_write, TCP_WRITE_FLAG_COPY) == ERR_OK) {
                tcp_output(client->pcb);
                br_ssl_engine_sendrec_ack(&client->sc.eng, to_write);
            }
        }
    }

    if ((state & BR_SSL_RECVREC) && client->rx_len > 0) {
        size_t len;
        unsigned char *buf = br_ssl_engine_recvrec_buf(&client->sc.eng, &len);
        size_t to_copy = (client->rx_len < len) ? client->rx_len : len;
        
        memcpy(buf, client->rx_queue, to_copy);
        br_ssl_engine_recvrec_ack(&client->sc.eng, to_copy);
        
        memmove(client->rx_queue, client->rx_queue + to_copy, client->rx_len - to_copy);
        client->rx_len -= to_copy;
    }

    if (state & BR_SSL_RECVAPP) {
        size_t len;
        unsigned char *buf = br_ssl_engine_recvapp_buf(&client->sc.eng, &len);

        size_t space_left = (client->response_capacity - 1) - client->response_len;
        
        if (space_left > 0) {
            size_t to_copy = (len < space_left) ? len : space_left;
            memcpy(client->response_buf + client->response_len, buf, to_copy);
            client->response_len += to_copy;
            client->response_buf[client->response_len] = '\0';
        }

        br_ssl_engine_recvapp_ack(&client->sc.eng, len);
    }

    if (state & BR_SSL_CLOSED) {
        int tls_err = br_ssl_engine_last_error(&client->sc.eng);
        https_finish(client, tls_err);
    }
}