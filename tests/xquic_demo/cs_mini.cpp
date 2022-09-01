#include "cs_mini.h"

#include <string>

#define XQC_PACKET_TMP_BUF_LEN 1500
#define g_conn_check_timeout 1                  // 1s each check
#define g_conn_timeout 5                        // 5s
#define g_client_engine_idle_timeout 30 * 1000  // 10s
#define g_server_engine_idle_timeout 30 * 1000  // 30s
#define MAX_BUF_SIZE (100 * 1024 * 1024)

// log
void xquic_server_write_log(xqc_log_level_t lvl, const void * buf, size_t count, void * engine_user_data) {
    // printf("xquic_log: %s\n", (char *)buf);
    return;
    // unsigned char log_buf[XQC_MAX_LOG_LEN + 1];

    // xqc_server_ctx_t * ctx = (xqc_server_ctx_t *)engine_user_data;
    // if (ctx->log_fd <= 0) {
    //     printf("xqc_server_write_log fd err\n");
    //     return;
    // }

    // int log_len = snprintf(log_buf, XQC_MAX_LOG_LEN + 1, "%s\n", (char *)buf);
    // if (log_len < 0) {
    //     printf("xqc_server_write_log err\n");
    //     return;
    // }

    // int write_len = write(ctx->log_fd, log_buf, log_len);
    // if (write_len < 0) {
    //     printf("xqc_server_write_log write failed, errno: %d\n", get_last_sys_errno());
    // }
}

// user_data as xquic_server_t
void xquic_server_set_event_timer(xqc_msec_t wake_after, void * user_data) {
    xquic_server_t * server = (xquic_server_t *)user_data;
    struct timeval tv;
    tv.tv_sec = wake_after / 1000000;
    tv.tv_usec = wake_after % 1000000;
    event_add(server->ev_engine, &tv);
}

// user_data as xquic_server_t
void xquic_client_set_event_timer(xqc_msec_t wake_after, void * user_data) {
    xquic_client_t * client = (xquic_client_t *)user_data;
    struct timeval tv;
    tv.tv_sec = wake_after / 1000000;
    tv.tv_usec = wake_after % 1000000;
    event_add(client->ev_engine, &tv);
}

void xquic_client_alive_timer(int fd, short what, void * user_data) {
    xquic_client_t * client = (xquic_client_t *)user_data;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    event_add(client->client_alive_timer, &tv);
}

/// server transport cbs
int xquic_server_accept(xqc_engine_t * engine, xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data) {
    DEBUG_INFO;
    xquic_server_t * server = (xquic_server_t *)user_data;
    user_conn_t * user_conn = (user_conn_t *)calloc(1, sizeof(*user_conn));
    user_conn->server = server;
    xqc_conn_set_transport_user_data(conn, user_conn);

    xqc_int_t ret = xqc_conn_get_peer_addr(conn, (struct sockaddr *)&user_conn->peer_addr, sizeof(user_conn->peer_addr), &user_conn->peer_addrlen);
    if (ret != XQC_OK) {
        return -1;
    }

    memcpy(&user_conn->cid, cid, sizeof(*cid));

    return 0;
}
ssize_t xquic_server_write_socket(const unsigned char * buf, size_t size, const struct sockaddr * peer_addr, socklen_t peer_addrlen, void * user_data) {
    user_conn_t * user_conn = (user_conn_t *)user_data;  // user_data may be empty when "reset" is sent
    ssize_t res = 0;
    if (user_conn == nullptr || user_conn->server == nullptr) {
        DEBUG_INFO;
        return XQC_SOCKET_ERROR;
    }

    static ssize_t snd_sum = 0;
    int fd = user_conn->server->fd;

    /* COPY to run corruption test cases */
    unsigned char send_buf[XQC_PACKET_TMP_BUF_LEN];
    size_t send_buf_size = 0;

    if (size > XQC_PACKET_TMP_BUF_LEN) {
        printf("xquic_server_write_socket err: size=%zu is too long\n", size);
        return XQC_SOCKET_ERROR;
    }
    send_buf_size = size;
    memcpy(send_buf, buf, send_buf_size);

    do {
        set_last_sys_errno(0);
        res = sendto(fd, send_buf, send_buf_size, 0, peer_addr, peer_addrlen);
        // printf("xqc_server_send write %zd, %s\n", res, strerror(get_last_sys_errno()));
        if (res < 0) {
            printf("xquic_server_write_socket err %zd %s\n", res, strerror(get_last_sys_errno()));
            if (get_last_sys_errno() == EAGAIN) {
                res = XQC_SOCKET_EAGAIN;
            }

        } else {
            snd_sum += res;
        }
    } while ((res < 0) && (EINTR == get_last_sys_errno()));

    return res;
}
void xquic_server_conn_update_cid_notify(xqc_connection_t * conn, const xqc_cid_t * retire_cid, const xqc_cid_t * new_cid, void * user_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    xquic_server_t * server = (xquic_server_t *)user_conn->server;

    memcpy(&user_conn->cid, new_cid, sizeof(*new_cid));

    printf("====>RETIRE SCID:%s\n", xqc_scid_str(retire_cid));
    printf("====>SCID:%s\n", xqc_scid_str(new_cid));
    printf("====>DCID:%s\n", xqc_dcid_str_by_scid(server->engine, new_cid));
}

static void xquic_server_engine_callback(int fd, short what, void * arg) {
    DEBUG_INFO;
    printf("timer wakeup now:%" PRIu64 "\n", xqc_now());
    xquic_server_t * server = (xquic_server_t *)arg;

    xqc_engine_main_logic(server->engine);
}
static void xquic_client_engine_callback(int fd, short what, void * arg) {
    DEBUG_INFO;
    printf("timer wakeup now:%" PRIu64 "\n", xqc_now());
    xquic_client_t * client = (xquic_client_t *)arg;

    xqc_engine_main_logic(client->engine);
}

/// client tranport cbs
ssize_t xquic_client_write_socket(const unsigned char * buf, size_t size, const struct sockaddr * peer_addr, socklen_t peer_addrlen, void * user) {
    // DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user;
    ssize_t res = 0;
    if (user_conn == nullptr || user_conn->client == nullptr) {
        DEBUG_INFO;
        return XQC_SOCKET_ERROR;
    }

    int fd = user_conn->fd;

    /* COPY to run corruption test cases */
    unsigned char send_buf[XQC_PACKET_TMP_BUF_LEN];
    size_t send_buf_size = 0;

    if (size > XQC_PACKET_TMP_BUF_LEN) {
        printf("xqc_client_write_socket err: size=%zu is too long\n", size);
        return XQC_SOCKET_ERROR;
    }
    send_buf_size = size;
    memcpy(send_buf, buf, send_buf_size);

    do {
        set_last_sys_errno(0);

        user_conn->last_sock_op_time = xqc_now();

        res = sendto(fd, send_buf, send_buf_size, 0, peer_addr, peer_addrlen);
        if (res < 0) {
            printf("xqc_client_write_socket err %zd %s\n", res, strerror(get_last_sys_errno()));
            if (get_last_sys_errno() == EAGAIN) {
                res = XQC_SOCKET_EAGAIN;
            }
        }
    } while ((res < 0) && (get_last_sys_errno() == EINTR));

    return res;
}
void xquic_client_save_token(const unsigned char * token, unsigned token_len, void * user_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    // char *ip = inet_ntoa(((struct sockaddr_in *)&user_conn->peer_addr)->sin_addr);// might add connection address key
    user_conn->client->xclient_save_token_cb(token, token_len);
}
void xquic_client_save_session_cb(const char * data, size_t data_len, void * user_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    user_conn->client->xclient_save_session_ticket_cb(data, data_len);
}
void xquic_client_save_tp_cb(const char * data, size_t data_len, void * user_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    user_conn->client->xclient_save_transport_parameter_cb(data, data_len);
}
int xquic_client_cert_verify(const unsigned char * certs[], const size_t cert_len[], size_t certs_len, void * conn_user_data) {
    /* self-signed cert used in test cases, return >= 0 means success */
    // DEBUG_INFO;
    // user_conn_t * user_conn = (user_conn_t *)conn_user_data;
    return 0;
}

xqc_int_t xquic_client_conn_closing_notify(xqc_connection_t * conn, const xqc_cid_t * cid, xqc_int_t err_code, void * conn_user_data) {
    printf("client conn closing: %d\n", err_code);
    return XQC_OK;
}

/// server app callbacks
/// conns: create/handshake_finish/close
int xquic_server_conn_create_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;
    return 0;
}
int xquic_server_conn_close_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    xquic_server_t * server = (xquic_server_t *)user_conn->server;
    xqc_conn_stats_t stats = xqc_conn_get_stats(server->engine, cid);
    printf("server: send_count:%u, lost_count:%u, tlp_count:%u, recv_count:%u, srtt:%" PRIu64 " early_data_flag:%d, conn_err:%d, ack_info:%s\n",
           stats.send_count,
           stats.lost_count,
           stats.tlp_count,
           stats.recv_count,
           stats.srtt,
           stats.early_data_flag,
           stats.conn_err,
           stats.ack_info);

    free(user_conn);
    return 0;
}
void xquic_server_conn_handshake_finished(xqc_connection_t * conn, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
}

/// server app callbacks
/// stream: create/read/close
int xquic_server_stream_create_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    int ret = 0;

    // get user_conn by user_stream;
    // so stream read notify could find cb function.
    user_conn_t * user_conn = (user_conn_t *)xqc_get_conn_user_data_by_stream(stream);

    user_stream_t * user_stream = (user_stream_t *)calloc(1, sizeof(*user_stream));
    user_stream->stream = stream;
    user_stream->user_conn = user_conn;
    xqc_stream_set_user_data(stream, user_stream);

    return 0;
}
int xquic_server_stream_read_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    unsigned char fin = 0;
    user_stream_t * user_stream = (user_stream_t *)user_data;

    std::string debug_result;
    top::xbytes_t result;

    unsigned char buff[4096] = {0};
    size_t buff_size = 4096;
    ssize_t read;
    ssize_t read_sum = 0;
    do {
        read = xqc_stream_recv(stream, buff, buff_size, &fin);
        if (read == -XQC_EAGAIN) {
            break;
        } else if (read < 0) {
            printf("xqc_stream_recv error %zd\n", read);
            return 0;
        }
        read_sum += read;

        user_stream->recv_body_len += read;

        result.insert(result.end(), buff, buff + read);

        debug_result += std::string{(char *)buff, read};
        printf("stream recv size:%zu", debug_result.size());

    } while (read > 0 && !fin);

    printf("xqc_stream_recv read:%zd, offset:%zu, fin:%d\n", read_sum, user_stream->recv_body_len, fin);

    if (fin) {
        char * ip = inet_ntoa(((struct sockaddr_in *)&user_stream->user_conn->peer_addr)->sin_addr);
        // std::size_t port = (std::size_t)((struct sockaddr_in *)&user_stream->user_conn->peer_addr)->sin_port; // meaningless
        printf("stream recv finish size:%zu\n recvdata: from " BOLDBLUE " %s " BOLDYELLOW " %s" RESET "\n", debug_result.size(), ip, debug_result.c_str());
        user_stream->user_conn->server->m_cb(result);
    }
    return 0;
}
int xquic_server_stream_close_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    user_stream_t * user_stream = (user_stream_t *)user_data;
    free(user_stream->send_body);
    free(user_stream->recv_body);
    free(user_stream);

    return 0;
}

/// client send stream
int xquic_client_stream_send(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    ssize_t ret;
    user_stream_t * user_stream = (user_stream_t *)user_data;

    int fin = 1;

    if (user_stream->send_offset < user_stream->send_body_len) {
        ret = xqc_stream_send(stream, (unsigned char *)user_stream->send_body + user_stream->send_offset, user_stream->send_body_len - user_stream->send_offset, fin);
        if (ret < 0) {
            printf("client xqc_stream_send error %zd\n", ret);
            return 0;

        } else {
            user_stream->send_offset += ret;
            printf("client xqc_stream_send offset=%" PRIu64 "\n", user_stream->send_offset);
        }
    }

    return 0;
}

/// client app callbacks
/// conns: create/close/handshake_finish/ping_acked
int xquic_client_conn_create_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;

    user_conn_t * user_conn = (user_conn_t *)user_data;
    // xqc_conn_set_alp_user_data(conn, user_conn);

    printf("xqc_conn_is_ready_to_send_early_data:%d\n", xqc_conn_is_ready_to_send_early_data(conn));
    return 0;
}
int xquic_client_conn_close_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;

    user_conn_t * user_conn = (user_conn_t *)user_data;
    xquic_client_t * client = (xquic_client_t *)user_conn->client;

    xqc_conn_stats_t stats = xqc_conn_get_stats(client->engine, cid);
    printf("client: send_count:%u, lost_count:%u, tlp_count:%u, recv_count:%u, srtt:%" PRIu64 " early_data_flag:%d, conn_err:%d, ack_info:%s\n",
           stats.send_count,
           stats.lost_count,
           stats.tlp_count,
           stats.recv_count,
           stats.srtt,
           stats.early_data_flag,
           stats.conn_err,
           stats.ack_info);

    event_del(user_conn->ev_socket);
    event_del(user_conn->conn_timeout_event);
    event_del(user_conn->rebinding_ev_socket);
    client->release_connection(user_conn);
    return 0;
}
void xquic_client_conn_handshake_finished(xqc_connection_t * conn, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)user_data;
    xquic_client_t * client = (xquic_client_t *)user_conn->client;
    xqc_conn_send_ping(client->engine, &user_conn->cid, NULL);
    // xqc_conn_send_ping(client->engine, &user_conn->cid, &g_ping_id);

    printf("====>DCID:%s\n", xqc_dcid_str_by_scid(client->engine, &user_conn->cid));
    printf("====>SCID:%s\n", xqc_scid_str(&user_conn->cid));
}
void xquic_client_conn_ping_acked_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * ping_user_data, void * user_data, void * conn_proto_data) {
    DEBUG_INFO;
    if (ping_user_data) {
        printf("====>ping_id:%d\n", *(int *)ping_user_data);

    } else {
        printf("====>no ping_id\n");
    }
}

/// client app callbacks
/// stream write/read/close
int xquic_client_stream_read_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    unsigned char fin = 0;
    user_stream_t * user_stream = (user_stream_t *)user_data;

    unsigned char buff[4096] = {0};
    size_t buff_size = 4096;
    ssize_t read;
    ssize_t read_sum = 0;

    do {
        read = xqc_stream_recv(stream, buff, buff_size, &fin);
        if (read == -XQC_EAGAIN) {
            break;

        } else if (read < 0) {
            printf("xqc_stream_recv error %zd\n", read);
            return 0;
        }

        read_sum += read;
        user_stream->recv_body_len += read;

    } while (read > 0 && !fin);

    printf("xqc_stream_recv read:%zd, offset:%zu, fin:%d\n", read_sum, user_stream->recv_body_len, fin);

    // if (fin) {
    // user_stream->recv_fin = 1;
    // xqc_msec_t now_us = xqc_now();
    // printf("\033[33m>>>>>>>> request time cost:%" PRIu64 " us, speed:%" PRIu64
    //        " K/s \n"
    //        ">>>>>>>> send_body_size:%zu, recv_body_size:%zu \033[0m\n",
    //        now_us - user_stream->start_time,
    //        (user_stream->send_body_len + user_stream->recv_body_len) * 1000 / (now_us - user_stream->start_time),
    //        user_stream->send_body_len,
    //        user_stream->recv_body_len);
    // }
    return 0;
}
int xquic_client_stream_write_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    int ret = 0;
    user_stream_t * user_stream = (user_stream_t *)user_data;
    ret = xquic_client_stream_send(stream, user_stream);
    return ret;
}
int xquic_client_stream_close_notify(xqc_stream_t * stream, void * user_data) {
    DEBUG_INFO;
    user_stream_t * user_stream = (user_stream_t *)user_data;

    free(user_stream->send_body);
    free(user_stream->recv_body);
    free(user_stream);
    return 0;
}

/// server create socket
static int xquic_server_create_socket(xquic_server_t * server, unsigned int port) {
    int fd;
    int type = AF_INET;
    server->local_addrlen = sizeof(struct sockaddr_in);
    struct sockaddr * saddr = (struct sockaddr *)&(server->local_addr);
    int size;
    int optval;

    fd = socket(type, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("create socket failed, errno: %d\n", get_last_sys_errno());
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        printf("set socket nonblock failed, errno: %d\n", errno);
        close(fd);
        return -1;
    }

    optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        printf("setsockopt failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    size = 1 * 1024 * 1024;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) < 0) {
        printf("setsockopt failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) < 0) {
        printf("setsockopt failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    memset(saddr, 0, sizeof(struct sockaddr_in));
    struct sockaddr_in * addr_v4 = (struct sockaddr_in *)saddr;
    addr_v4->sin_family = type;
    addr_v4->sin_port = htons(port);
    addr_v4->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, saddr, server->local_addrlen) < 0) {
        printf("bind socket failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    return fd;
}

/// server socket event
void xqc_server_socket_write_handler(xquic_server_t * server) {
    DEBUG_INFO;
}
void xqc_server_socket_read_handler(xquic_server_t * server) {
    DEBUG_INFO;
    ssize_t recv_sum = 0;
    struct sockaddr_in6 peer_addr;
    socklen_t peer_addrlen = sizeof(struct sockaddr_in);
    ssize_t recv_size = 0;
    unsigned char packet_buf[XQC_PACKET_TMP_BUF_LEN];
    uint64_t recv_time;

    do {
        recv_size = recvfrom(server->fd, packet_buf, sizeof(packet_buf), 0, (struct sockaddr *)&peer_addr, &peer_addrlen);
        if (recv_size < 0 && get_last_sys_errno() == EAGAIN) {
            break;
        }

        if (recv_size < 0) {
            printf("!!!!!!!!!recvfrom: recvmsg = %zd err=%s\n", recv_size, strerror(get_last_sys_errno()));
            break;
        }

        recv_sum += recv_size;

        recv_time = xqc_now();

        xqc_int_t result = xqc_engine_packet_process(server->engine,
                                                     packet_buf,
                                                     recv_size,
                                                     (struct sockaddr *)(&server->local_addr),
                                                     server->local_addrlen,
                                                     (struct sockaddr *)(&peer_addr),
                                                     peer_addrlen,
                                                     (xqc_msec_t)recv_time,
                                                     server);

        if (result != XQC_OK) {
            printf("xqc_server_read_handler: packet process err\n");
            return;
        }
    } while (recv_size > 0);

    printf("server socket recv size:%zu\n", recv_sum);
    xqc_engine_finish_recv(server->engine);
}
static void xqc_server_socket_event_callback(int fd, short what, void * arg) {
    // DEBUG_INFO;

    xquic_server_t * server = (xquic_server_t *)arg;

    if (what & EV_WRITE) {
        xqc_server_socket_write_handler(server);

    } else if (what & EV_READ) {
        xqc_server_socket_read_handler(server);

    } else {
        printf("event callback: what=%d\n", what);
        exit(1);
    }
}

/// client create socket
static int xquic_client_create_socket(int type, user_conn_t * conn) {
    int size;
    int fd = -1;
    int flags;

    /* create fd & set socket option */
    fd = socket(type, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("create socket failed, errno: %d\n", get_last_sys_errno());
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        printf("set socket nonblock failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    size = 1 * 1024 * 1024;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) < 0) {
        printf("setsockopt failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) < 0) {
        printf("setsockopt failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    conn->last_sock_op_time = xqc_now();  // g_last_sock_op_time

    if (connect(fd, (struct sockaddr *)&conn->peer_addr, conn->peer_addrlen) < 0) {
        printf("connect socket failed, errno: %d\n", get_last_sys_errno());
        close(fd);
        return -1;
    }

    return fd;
}

/// client conn timeout callback
static void xquic_client_conn_timeout_callback(int fd, short what, void * arg) {
    DEBUG_INFO_MSG("client: connection timeout callback check");
    user_conn_t * user_conn = (user_conn_t *)arg;

    static int restart_after_a_while = 1;

    if (xqc_now() - user_conn->last_sock_op_time < (uint64_t)g_conn_timeout * 1000000) {
        struct timeval tv;
        tv.tv_sec = g_conn_check_timeout;
        tv.tv_usec = 0;
        event_add(user_conn->conn_timeout_event, &tv);
        return;
    }
    DEBUG_INFO_MSG("client: connection do timeout close");
    int rc = xqc_conn_close(user_conn->client->engine, &user_conn->cid);
    if (rc) {
        printf("xqc_conn_close error %d \n", rc);
        return;
    }
}

/// client stream timeout callback
static void xquic_client_stream_timeout_callback(int fd, short what, void * arg) {
    DEBUG_INFO_MSG("client: stream timeout callback check");
    user_stream_t * user_stream = (user_stream_t *)arg;

    if (user_stream->send_offset == 0 || user_stream->send_offset < user_stream->send_body_len) {
        // send not over.
        struct timeval tv;
        tv.tv_sec = g_conn_check_timeout;
        tv.tv_usec = 0;
        event_add(user_stream->stream_timeout_event, &tv);
        return;
    }

    DEBUG_INFO_MSG("client: stream do timeout close");
    int rc = xqc_stream_close(user_stream->stream);
    if (rc) {
        printf("xqc_stream_close error %d \n", rc);
        return;
    }
}

/// client socket event
void xquic_client_socket_write_handler(user_conn_t * user_conn) {
    xqc_conn_continue_send(user_conn->client->engine, &user_conn->cid);
}
void xquic_client_socket_read_handler(user_conn_t * user_conn, int fd) {
    ssize_t recv_size = 0;
    ssize_t recv_sum = 0;

    unsigned char packet_buf[XQC_PACKET_TMP_BUF_LEN];

    // static ssize_t last_rcv_sum = 0;
    static ssize_t rcv_sum = 0;

    do {
        recv_size = recvfrom(fd, packet_buf, sizeof(packet_buf), 0, &user_conn->peer_addr, &user_conn->peer_addrlen);
        if (recv_size < 0 && get_last_sys_errno() == EAGAIN) {
            break;
        }

        if (recv_size < 0) {
            printf("recvfrom: recvmsg = %zd(%s)\n", recv_size, strerror(get_last_sys_errno()));
            break;
        }

        /* if recv_size is 0, break while loop, */
        if (recv_size == 0) {
            break;
        }

        recv_sum += recv_size;
        rcv_sum += recv_size;

        if (user_conn->get_local_addr == 0) {
            user_conn->get_local_addr = 1;
            socklen_t tmp = sizeof(struct sockaddr_in6);
            int ret = getsockname(user_conn->fd, (struct sockaddr *)user_conn->local_addr, &tmp);
            if (ret < 0) {
                printf("getsockname error, errno: %d\n", get_last_sys_errno());
                break;
            }
            user_conn->local_addrlen = tmp;
        }

        uint64_t recv_time = xqc_now();
        user_conn->last_sock_op_time = recv_time;

        static char copy[XQC_PACKET_TMP_BUF_LEN];

        if (xqc_engine_packet_process(user_conn->client->engine,
                                      packet_buf,
                                      recv_size,
                                      user_conn->local_addr,
                                      user_conn->local_addrlen,
                                      &user_conn->peer_addr,
                                      user_conn->peer_addrlen,
                                      (xqc_msec_t)recv_time,
                                      user_conn) != XQC_OK) {
            printf("xqc_client_read_handler: packet process err\n");
            return;
        }

    } while (recv_size > 0);

    // if ((xqc_now() - last_recv_ts) > 200000) {
    //     printf("recving rate: %.3lf Kbps\n", (rcv_sum - last_rcv_sum) * 8.0 * 1000 / (xqc_now() - last_recv_ts));
    //     last_recv_ts = xqc_now();
    //     last_rcv_sum = rcv_sum;
    // }

    printf("client socket recv size:%zu\n", recv_sum);
    xqc_engine_finish_recv(user_conn->client->engine);
}
static void xquic_client_socket_event_callback(int fd, short what, void * arg) {
    // DEBUG_INFO;
    user_conn_t * user_conn = (user_conn_t *)arg;

    if (what & EV_WRITE) {
        xquic_client_socket_write_handler(user_conn);

    } else if (what & EV_READ) {
        xquic_client_socket_read_handler(user_conn, fd);

    } else {
        printf("event callback: what=%d\n", what);
        exit(1);
    }
}

/// client create user conn
void xquic_convert_addr_text_to_sockaddr(int type, const char * addr_text, unsigned int port, struct sockaddr * saddr, socklen_t * saddr_len) {
    memset(saddr, 0, sizeof(struct sockaddr_in));
    struct sockaddr_in * addr_v4 = (struct sockaddr_in *)(saddr);
    inet_pton(type, addr_text, &(addr_v4->sin_addr.s_addr));
    addr_v4->sin_family = type;
    addr_v4->sin_port = htons(port);
    *saddr_len = sizeof(struct sockaddr_in);
}
void xquic_client_init_addr(user_conn_t * user_conn, const char * server_addr, int server_port) {
    int ip_type = AF_INET;
    xquic_convert_addr_text_to_sockaddr(ip_type, server_addr, server_port, &user_conn->peer_addr, &user_conn->peer_addrlen);

    user_conn->local_addr = (struct sockaddr *)calloc(1, sizeof(struct sockaddr_in));
    memset(user_conn->local_addr, 0, sizeof(struct sockaddr_in));
    user_conn->local_addrlen = sizeof(struct sockaddr_in);
}
user_conn_t * xquic_client_user_conn_create(const char * server_addr, int server_port, xquic_client_t * client) {
    user_conn_t * user_conn = (user_conn_t *)calloc(1, sizeof(user_conn_t));
    user_conn->client = client;

    user_conn->conn_timeout_event = event_new(client->eb, -1, 0, xquic_client_conn_timeout_callback, user_conn);
    /* set connection timeout */
    struct timeval tv;
    tv.tv_sec = g_conn_check_timeout;
    tv.tv_usec = 0;
    event_add(user_conn->conn_timeout_event, &tv);

    int ip_type = AF_INET;
    xquic_client_init_addr(user_conn, server_addr, server_port);

    user_conn->fd = xquic_client_create_socket(ip_type, user_conn);
    if (user_conn->fd < 0) {
        printf("xqc_create_socket error\n");
        return NULL;
    }

    user_conn->ev_socket = event_new(client->eb, user_conn->fd, EV_READ | EV_PERSIST, xquic_client_socket_event_callback, user_conn);
    event_add(user_conn->ev_socket, NULL);

    user_conn->rebinding_fd = xquic_client_create_socket(ip_type, user_conn);
    if (user_conn->rebinding_fd < 0) {
        printf("|rebinding|xqc_create_socket error\n");
        return NULL;
    }

    user_conn->rebinding_ev_socket = event_new(client->eb, user_conn->rebinding_fd, EV_READ | EV_PERSIST, xquic_client_socket_event_callback, user_conn);
    event_add(user_conn->rebinding_ev_socket, NULL);

    return user_conn;
}

bool xquic_server_t::init(xquic_message_ready_callback cb, unsigned int const server_port) {
    m_cb = cb;

    xqc_engine_ssl_config_t engine_ssl_config;
    memset(&engine_ssl_config, 0, sizeof(engine_ssl_config));
    engine_ssl_config.private_key_file = (char *)"./server.key";
    engine_ssl_config.cert_file = (char *)"./server.crt";
    engine_ssl_config.ciphers = (char *)XQC_TLS_CIPHERS;
    engine_ssl_config.groups = (char *)XQC_TLS_GROUPS;
    engine_ssl_config.session_ticket_key_data = NULL;
    engine_ssl_config.session_ticket_key_len = 0;

    xqc_engine_callback_t callback = {
        .set_event_timer = xquic_server_set_event_timer,
        .log_callbacks =
            {
                .xqc_log_write_err = xquic_server_write_log,
                .xqc_log_write_stat = xquic_server_write_log,
            },
        // .keylog_cb = xqc_keylog_cb,
    };

    xqc_transport_callbacks_t tcbs;
    memset(&tcbs, 0, sizeof(tcbs));
    tcbs.server_accept = xquic_server_accept;
    tcbs.write_socket = xquic_server_write_socket;
    tcbs.conn_update_cid_notify = xquic_server_conn_update_cid_notify;
    // xqc_transport_callbacks_t tcbs = {
    //     .server_accept = xquic_server_accept,
    //     .write_socket = xquic_server_write_socket,
    //     .conn_update_cid_notify = xquic_server_conn_update_cid_notify,
    // };

    xqc_cong_ctrl_callback_t cong_ctrl;
    uint32_t cong_flags = 0;
    cong_ctrl = xqc_bbr_cb;
    cong_flags = XQC_BBR_FLAG_NONE;

    xqc_cc_params_t cc_params;
    memset(&cc_params, 0, sizeof(cc_params));
    cc_params.customize_on = 1;
    cc_params.init_cwnd = 32;
    cc_params.cc_optimization_flags = cong_flags;

    xqc_conn_settings_t conn_settings;
    memset(&conn_settings, 0, sizeof(conn_settings));
    conn_settings.pacing_on = 0;
    conn_settings.cong_ctrl_callback = cong_ctrl;
    conn_settings.spurious_loss_detect_on = 0;
    conn_settings.cc_params = cc_params;
    conn_settings.idle_time_out = g_server_engine_idle_timeout;
    // xqc_conn_settings_t conn_settings = {
    //     .pacing_on = 0,
    //     .cong_ctrl_callback = cong_ctrl,
    //     .cc_params = {.customize_on = 1, .init_cwnd = 32, .cc_optimization_flags = cong_flags},
    //     .spurious_loss_detect_on = 0,
    // };

    xqc_server_set_conn_settings(&conn_settings);

    xqc_config_t config;
    if (xqc_engine_get_default_config(&config, XQC_ENGINE_SERVER) < 0) {
        return false;
    }
    config.cfg_log_level = XQC_LOG_DEBUG;

    eb = event_base_new();
    ev_engine = event_new(eb, -1, 0, xquic_server_engine_callback, this);
    engine = xqc_engine_create(XQC_ENGINE_SERVER, &config, &engine_ssl_config, &callback, &tcbs, this);

    if (engine == nullptr) {
        printf("error create engine\n");
        return false;
    }

    /* register transport callbacks */
    xqc_conn_callbacks_t conn_cbs;
    memset(&conn_cbs, 0, sizeof(conn_cbs));
    conn_cbs.conn_create_notify = xquic_server_conn_create_notify;
    conn_cbs.conn_close_notify = xquic_server_conn_close_notify;
    conn_cbs.conn_handshake_finished = xquic_server_conn_handshake_finished;

    xqc_stream_callbacks_t stream_cbs;
    memset(&stream_cbs, 0, sizeof(stream_cbs));
    stream_cbs.stream_read_notify = xquic_server_stream_read_notify;
    stream_cbs.stream_create_notify = xquic_server_stream_create_notify;
    stream_cbs.stream_close_notify = xquic_server_stream_close_notify;

    xqc_app_proto_callbacks_t ap_cbs;
    memset(&ap_cbs, 0, sizeof(ap_cbs));
    ap_cbs.conn_cbs = conn_cbs;
    ap_cbs.stream_cbs = stream_cbs;

    // xqc_app_proto_callbacks_t ap_cbs = {.conn_cbs =
    //                                         {
    //                                             .conn_create_notify = xquic_server_conn_create_notify,
    //                                             .conn_close_notify = xquic_server_conn_close_notify,
    //                                             .conn_handshake_finished = xquic_server_conn_handshake_finished,
    //                                         },
    //                                     .stream_cbs = {
    //                                         // .stream_write_notify = xquic_server_stream_write_notify, // unused
    //                                         .stream_read_notify = xquic_server_stream_read_notify,
    //                                         .stream_create_notify = xquic_server_stream_create_notify,
    //                                         .stream_close_notify = xquic_server_stream_close_notify,
    //                                     }};

    xqc_engine_register_alpn(engine, "transport", 9, &ap_cbs);

    /* for lb cid generate */
    memcpy(quic_lb_ctx.sid_buf, g_sid, g_sid_len);
    memcpy(quic_lb_ctx.lb_cid_key, g_lb_cid_enc_key, XQC_LB_CID_KEY_LEN);
    quic_lb_ctx.lb_cid_enc_on = 0;  // g_lb_cid_encryption_on;;
    quic_lb_ctx.sid_len = g_sid_len;
    quic_lb_ctx.conf_id = 0;
    quic_lb_ctx.cid_len = XQC_MAX_CID_LEN;

    fd = xquic_server_create_socket(this, server_port);
    if (fd < 0) {
        printf("xqc_create_socket error\n");
        return false;
    }

    ev_socket = event_new(eb, fd, EV_READ | EV_PERSIST, xqc_server_socket_event_callback, this);

    event_add(ev_socket, NULL);
    last_snd_ts = 0;
    DEBUG_INFO_MSG("init server");
    event_base_dispatch(eb);  // block at here
    DEBUG_INFO_MSG("!!!SERVER DESTROY!!!");

    xqc_engine_destroy(engine);
    // xqc_server_close_keylog_file(&ctx);
    // xqc_server_close_log_file(&ctx);

    return true;
}

bool xquic_client_t::init() {
    xqc_engine_ssl_config_t engine_ssl_config;
    memset(&engine_ssl_config, 0, sizeof(engine_ssl_config));
    /* client does not need to fill in private_key_file & cert_file */
    engine_ssl_config.ciphers = (char *)XQC_TLS_CIPHERS;
    engine_ssl_config.groups = (char *)XQC_TLS_GROUPS;

    xqc_engine_callback_t callback = {
        .set_event_timer = xquic_client_set_event_timer,
        .log_callbacks =
            {
                .xqc_log_write_err = xquic_server_write_log,
                .xqc_log_write_stat = xquic_server_write_log,
            },
        // .keylog_cb = xqc_keylog_cb,
    };

    xqc_transport_callbacks_t tcbs;
    memset(&tcbs, 0, sizeof(tcbs));
    tcbs.write_socket = xquic_client_write_socket;
    tcbs.save_token = xquic_client_save_token;
    tcbs.save_session_cb = xquic_client_save_session_cb;
    tcbs.save_tp_cb = xquic_client_save_tp_cb;
    tcbs.cert_verify_cb = xquic_client_cert_verify;
    tcbs.conn_closing = xquic_client_conn_closing_notify;

    // xqc_transport_callbacks_t tcbs = {
    //     .write_socket = xquic_client_write_socket,
    //     .save_token = xquic_client_save_token,
    //     .save_session_cb = xquic_client_save_session_cb,
    //     .save_tp_cb = xquic_client_save_tp_cb,
    //     .cert_verify_cb = xquic_client_cert_verify,
    //     .conn_closing = xquic_client_conn_closing_notify,
    // };

    xqc_config_t config;
    if (xqc_engine_get_default_config(&config, XQC_ENGINE_CLIENT) < 0) {
        return -1;
    }
    config.cfg_log_level = XQC_LOG_DEBUG;

    eb = event_base_new();
    ev_engine = event_new(eb, -1, 0, xquic_client_engine_callback, this);
    engine = xqc_engine_create(XQC_ENGINE_CLIENT, &config, &engine_ssl_config, &callback, &tcbs, this);

    if (engine == NULL) {
        printf("xqc_engine_create error\n");
        return false;
    }

    xqc_conn_callbacks_t conn_cbs;
    memset(&conn_cbs, 0, sizeof(conn_cbs));
    conn_cbs.conn_create_notify = xquic_client_conn_create_notify;
    conn_cbs.conn_close_notify = xquic_client_conn_close_notify;
    conn_cbs.conn_handshake_finished = xquic_client_conn_handshake_finished;
    conn_cbs.conn_ping_acked = xquic_client_conn_ping_acked_notify;

    xqc_stream_callbacks_t stream_cbs;
    memset(&stream_cbs, 0, sizeof(stream_cbs));
    stream_cbs.stream_read_notify = xquic_client_stream_read_notify;
    stream_cbs.stream_write_notify = xquic_client_stream_write_notify;
    stream_cbs.stream_close_notify = xquic_client_stream_close_notify;

    xqc_app_proto_callbacks_t ap_cbs;
    memset(&ap_cbs, 0, sizeof(ap_cbs));
    ap_cbs.conn_cbs = conn_cbs;
    ap_cbs.stream_cbs = stream_cbs;

    // xqc_app_proto_callbacks_t ap_cbs = {
    //     .conn_cbs =
    //         {
    //             .conn_create_notify = xquic_client_conn_create_notify,
    //             .conn_close_notify = xquic_client_conn_close_notify,
    //             .conn_handshake_finished = xquic_client_conn_handshake_finished,
    //             .conn_ping_acked = xquic_client_conn_ping_acked_notify,
    //         },
    //     .stream_cbs =
    //         {
    //             .stream_read_notify = xquic_client_stream_read_notify,
    //             .stream_write_notify = xquic_client_stream_write_notify,
    //             .stream_close_notify = xquic_client_stream_close_notify,
    //         },
    // };

    xqc_engine_register_alpn(engine, "transport", 9, &ap_cbs);

    client_alive_timer = event_new(eb, -1, 0, xquic_client_alive_timer, this);
    /* set connection timeout */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_add(client_alive_timer, &tv);

    DEBUG_INFO_MSG("init client");
    event_base_dispatch(eb);
    DEBUG_INFO_MSG("!!!CLIENT DESTROY!!!");

#if 0
    // todo move it where?
    event_free(user_conn->ev_socket);
    event_free(user_conn->conn_timeout_event);
    event_free(user_conn->rebinding_ev_socket);

    // free(user_conn->peer_addr);
    free(user_conn->local_addr);
    free(user_conn);

    // xqc_engine_destroy(engine);
#endif

    // xqc_client_close_keylog_file(&ctx);
    // xqc_client_close_log_file(&ctx);
    return true;
}

user_conn_t * xquic_client_t::connect(char server_addr[64], uint32_t server_port) {
    DEBUG_INFO_MSG("new connection");
    xqc_cong_ctrl_callback_t cong_ctrl;
    uint32_t cong_flags = 0;
    cong_ctrl = xqc_bbr_cb;
    cong_flags = XQC_BBR_FLAG_NONE;

    xqc_cc_params_t cc_params;
    memset(&cc_params, 0, sizeof(cc_params));
    cc_params.customize_on = 1;
    cc_params.init_cwnd = 32;
    cc_params.cc_optimization_flags = cong_flags;

    xqc_conn_settings_t conn_settings;
    memset(&conn_settings, 0, sizeof(conn_settings));
    conn_settings.pacing_on = 0;
    conn_settings.ping_on = 0;
    conn_settings.cong_ctrl_callback = cong_ctrl;
    conn_settings.cc_params = cc_params;
    conn_settings.proto_version = XQC_VERSION_V1;
    conn_settings.spurious_loss_detect_on = 0;
    conn_settings.keyupdate_pkt_threshold = 0;
    conn_settings.idle_time_out = g_client_engine_idle_timeout;

    // xqc_conn_settings_t conn_settings = {
    //     .pacing_on = 0,
    //     .ping_on = 0,
    //     .cong_ctrl_callback = cong_ctrl,
    //     .cc_params = {.customize_on = 1, .init_cwnd = 32, .cc_optimization_flags = cong_flags},
    //     //.so_sndbuf  =   1024*1024,
    //     .proto_version = XQC_VERSION_V1,
    //     .spurious_loss_detect_on = 0,
    //     .keyupdate_pkt_threshold = 0,
    // };

    user_conn_t * user_conn = xquic_client_user_conn_create(server_addr, server_port, this);
    if (user_conn == nullptr) {
        printf("xquic_client_user_conn_create error\n");
        // todo add ec?
        return nullptr;
    }

    // perf token/st/td 's key
    unsigned char * token;
    unsigned token_len = 0;
    xclient_read_token(&token, &token_len);

    if (token_len != 0) {
        user_conn->token = token;
        user_conn->token_len = token_len;
    }

    xqc_conn_ssl_config_t conn_ssl_config;
    memset(&conn_ssl_config, 0, sizeof(conn_ssl_config));

    char * session_ticket_data;
    unsigned session_ticket_data_len = 0;
    char * tp_data;
    unsigned tp_data_len = 0;

    xclient_read_session_ticket(&session_ticket_data, &session_ticket_data_len);
    xclient_read_transport_parameter(&tp_data, &tp_data_len);

    // open ssl verify.
    // conn_ssl_config.cert_verify_flag |= XQC_TLS_CERT_FLAG_NEED_VERIFY;
    // conn_ssl_config.cert_verify_flag |= XQC_TLS_CERT_FLAG_ALLOW_SELF_SIGNED;

    if (session_ticket_data_len == 0 || tp_data_len == 0) {
        conn_ssl_config.session_ticket_data = NULL;
        conn_ssl_config.transport_parameter_data = NULL;
    } else {
        conn_ssl_config.session_ticket_data = session_ticket_data;
        conn_ssl_config.session_ticket_len = session_ticket_data_len;
        conn_ssl_config.transport_parameter_data = tp_data;
        conn_ssl_config.transport_parameter_data_len = tp_data_len;
    }

    const xqc_cid_t * cid;
    cid = xqc_connect(
        engine, &conn_settings, user_conn->token, user_conn->token_len, server_addr, 0, &conn_ssl_config, &user_conn->peer_addr, user_conn->peer_addrlen, "transport", user_conn);
    if (cid == nullptr) {
        printf("xqc_connect error\n");
        free(user_conn->local_addr);
        free(user_conn);
        return nullptr;
    }
    memcpy(&user_conn->cid, cid, sizeof(*cid));

    return user_conn;
}

bool xquic_client_t::send(user_conn_t * user_conn, top::xbytes_t send_data) {
    DEBUG_INFO;
    DEBUG_INFO_MSG(xqc_scid_str(&user_conn->cid));
    user_stream_t * user_stream = (user_stream_t *)calloc(1, sizeof(user_stream_t));
    memset(user_stream, 0, sizeof(user_stream_t));
    user_stream->user_conn = user_conn;
    user_stream->stream = xqc_stream_create(engine, &user_conn->cid, user_stream);

    if (user_stream->stream == nullptr) {
        // todo ec
        DEBUG_INFO_MSG("create stream error");
        return false;
    } else {
        /// set stream timeout
        user_stream->stream_timeout_event = event_new(user_conn->client->eb, -1, 0, xquic_client_stream_timeout_callback, user_stream);
        struct timeval tv;
        tv.tv_sec = g_conn_check_timeout;
        tv.tv_usec = 0;
        event_add(user_stream->stream_timeout_event, &tv);

        user_stream->send_body_max = MAX_BUF_SIZE;
        user_stream->send_body = (char *)malloc(send_data.size());
        user_stream->send_body_len = send_data.size();
        std::copy(send_data.begin(), send_data.end(), user_stream->send_body);
        // strncpy(user_stream->send_body, send_data.begin(), send_data.size());
        xquic_client_stream_send(user_stream->stream, user_stream);
    }
    return true;
}

void xquic_client_t::release_connection(user_conn_t * user_conn) {
    DEBUG_INFO_MSG(xqc_scid_str(&user_conn->cid));
    event_free(user_conn->ev_socket);
    event_free(user_conn->conn_timeout_event);
    event_free(user_conn->rebinding_ev_socket);

    if (user_conn->token) {
        free(user_conn->token);
    }
    free(user_conn->local_addr);
    memset(&user_conn->cid, 0, sizeof(user_conn->cid));
}
