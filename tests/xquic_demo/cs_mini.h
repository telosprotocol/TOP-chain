#pragma once

#include "common.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"

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

#include <deque>
#include <functional>
#include <string>

class xquic_server_t;
class xquic_client_t;

using xquic_message_ready_callback = std::function<void(top::xbytes_t const &)>;

class user_stream_t;
class cli_user_stream_t;

struct srv_user_conn_t {
    xquic_server_t * server;

    xqc_cid_t cid;
    struct sockaddr peer_addr;
    socklen_t peer_addrlen;
};

struct cli_user_conn_t {
    xquic_client_t * client;

    event * conn_timeout_event;

    int fd;

    event * ev_socket;

    uint64_t last_sock_op_time;

    xqc_cid_t cid;

    struct sockaddr peer_addr;
    socklen_t peer_addrlen;
    struct sockaddr * local_addr;
    socklen_t local_addrlen;
    xqc_flag_t get_local_addr;

    unsigned char * token;
    unsigned token_len;

    cli_user_stream_t * cli_user_stream;
};

struct cli_user_stream_t {
    xqc_stream_t * stream;

    std::deque<top::xbyte_t> send_queue;

    top::xbytes_t send_buffer;

    std::size_t send_offset{0};  // buffer_l;

    std::size_t buffer_right_index{0};

    xqc_msec_t latest_update_time;

    struct event * stream_timeout_event;

    cli_user_conn_t * cli_user_conn;
};

struct srv_user_stream_t {
    xqc_stream_t * stream;
    top::xbytes_t recv_buffer;
    srv_user_conn_t * srv_user_conn;

    bool has_recv_block_len{false};
    std::size_t block_len{0};
};

struct client_send_buffer_t {
    top::xbytes_t send_data;
    cli_user_conn_t * cli_user_conn{nullptr};
};

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
    constexpr static std::size_t max_send_queue_size{100000};
    top::threading::xthreadsafe_queue<client_send_buffer_t *, std::vector<client_send_buffer_t *>> m_send_queue{max_send_queue_size};

private:
    unsigned char * m_token;
    unsigned m_token_len;
    char * m_session;
    unsigned m_session_len;
    char * m_tp_para;
    unsigned m_tp_para_len;

private:
    uint64_t do_send_interval{1}; // should be 128us - 2048us

public:
    /// main function from client
    bool init();

    cli_user_conn_t * connect(char server_addr[64], uint32_t server_port);

    void quic_engine_do_send();
    bool send(cli_user_conn_t * cli_user_conn, top::xbytes_t send_data);

    void release_connection(cli_user_conn_t * cli_user_conn);

public:
    xqc_engine_t * engine;
    struct event_base * eb;
    struct event * client_alive_timer;
    struct event * ev_engine;
};