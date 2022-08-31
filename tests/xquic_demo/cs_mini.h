#pragma once

#include "common.h"
#include "platform.h"
#include "xbasic/xbyte_buffer.h"

#include <errno.h>
#include <event2/event.h>
#include <fcntl.h>
#include <inttypes.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <xquic/xqc_http3.h>
#include <xquic/xquic.h>
#include <xquic/xquic_typedef.h>

#include <functional>
#include <string>

class xquic_server_t;
class xquic_client_t;

using xquic_message_ready_callback = std::function<void(top::xbytes_t const &)>;

typedef struct user_conn_s {
    int fd;
    struct event * ev_socket;
    int rebinding_fd;
    struct event * rebinding_ev_socket;

    struct event * ev_timeout;

    unsigned char * token;
    unsigned token_len;

    struct sockaddr peer_addr;
    socklen_t peer_addrlen;
    struct sockaddr * local_addr;
    socklen_t local_addrlen;
    xqc_flag_t get_local_addr;
    xqc_cid_t cid;
    xquic_server_t * server;
    xquic_client_t * client;
    uint64_t last_sock_op_time;
} user_conn_t;

typedef struct user_stream_s {
    xqc_stream_t * stream;
    uint64_t send_offset;
    char * send_body;
    size_t send_body_len;
    size_t send_body_max;
    char * recv_body;
    size_t recv_body_len;

    user_conn_t * user_conn;
} user_stream_t;

class xquic_server_t {
private:
    /// struct
    typedef struct xqc_quic_lb_ctx_s {
        uint8_t sid_len;
        uint8_t sid_buf[XQC_MAX_CID_LEN];
        uint8_t conf_id;
        uint8_t cid_len;
        uint8_t cid_buf[XQC_MAX_CID_LEN];
        uint8_t lb_cid_key[XQC_LB_CID_KEY_LEN];
        int lb_cid_enc_on;
    } xqc_quic_lb_ctx_t;

public:
    /// members:
    struct event_base * eb;
    char g_sid[XQC_MAX_CID_LEN];
    char g_lb_cid_enc_key[XQC_LB_CID_KEY_LEN];
    size_t g_sid_len = 0;
    size_t g_lb_cid_enc_key_len = 0;
    uint64_t last_snd_ts;

    xqc_engine_t * engine;
    struct event * ev_socket;
    struct event * ev_engine;
    int fd;
    xqc_quic_lb_ctx_t quic_lb_ctx;
    struct sockaddr_in6 local_addr;
    socklen_t local_addrlen;

public:
    xquic_message_ready_callback m_cb;
    /// main function from server
    bool init(xquic_message_ready_callback cb, unsigned int const server_port);
};

class xquic_client_t {
public:
    void debug_token(bool save) {
        if (save) {
            printf(" ======= save token:");
            for (unsigned i = 0; i < m_token_len; ++i) {
                printf(GREEN " 0x%02x ", m_token[i]);
            }
            printf(RESET "\n");
        } else {
            printf(" ======= get token:");
            for (unsigned i = 0; i < m_token_len; ++i) {
                printf(BLUE " 0x%02x ", m_token[i]);
            }
            printf(RESET "\n");
        }
    }
    void xclient_read_token(unsigned char ** token, unsigned * token_len) {
        if (m_token_len) {
            *token = (unsigned char *)malloc(m_token_len);
            memcpy(*token, m_token, m_token_len);
            *token_len = m_token_len;
            debug_token(false);
        }
    }

    void xclient_save_token_cb(const unsigned char * token, unsigned token_len) {
        if (token_len) {
            m_token_len = token_len;
            m_token = (unsigned char *)malloc(m_token_len);
            memcpy(m_token, token, m_token_len);
            debug_token(true);
        }
    }

    void xclient_read_session_ticket(char ** session, unsigned * session_len) {
        if (m_session_len) {
            *session = (char *)malloc(m_session_len);
            memcpy(*session, m_session, m_session_len);
            *session_len = m_session_len;
        }
    }
    void xclient_save_session_ticket_cb(const char * session, unsigned session_len) {
        if (session_len) {
            m_session_len = session_len;
            m_session = (char *)malloc(m_session_len);
            memcpy(m_session, session, m_session_len);
        }
    }

    void xclient_read_transport_parameter(char ** tp_para, unsigned * tp_para_len) {
        if (m_tp_para_len) {
            *tp_para = (char *)malloc(m_tp_para_len);
            memcpy(*tp_para, m_tp_para, m_tp_para_len);
            *tp_para_len = m_tp_para_len;
        }
    }
    void xclient_save_transport_parameter_cb(const char * tp_para, unsigned tp_para_len) {
        if (tp_para_len) {
            m_tp_para_len = tp_para_len;
            m_tp_para = (char *)malloc(m_tp_para_len);
            memcpy(m_tp_para, tp_para, m_tp_para_len);
        }
    }

private:
    unsigned char * m_token;
    unsigned m_token_len;
    char * m_session;
    unsigned m_session_len;
    char * m_tp_para;
    unsigned m_tp_para_len;

public:
    /// main function from client
    bool init();

    user_conn_t * connect(char server_addr[64], uint32_t server_port);

    bool send(user_conn_t * user_conn, top::xbytes_t send_data);

    void release_connection(user_conn_t * user_conn);

public:
    xqc_engine_t * engine;
    struct event_base * eb;
    struct event * ev_engine;
};