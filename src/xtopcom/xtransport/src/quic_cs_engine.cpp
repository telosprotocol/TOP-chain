#include "xtransport/xquic_node/quic_cs_engine.h"

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"

#include <event2/thread.h>
#include <xquic/xquic.h>

#include <cassert>
#include <cerrno>
#include <set>
#include <string>

#define XQC_PACKET_TMP_BUF_LEN 1500
#define g_conn_check_timeout 1                  // 1s each check
#define g_conn_timeout 25                       // 25s
#define g_stream_handle_timeout 24              // 24s
#define g_client_engine_idle_timeout 30 * 1000  // 30s
#define g_server_engine_idle_timeout 30 * 1000  // 30s
#define MAX_BUF_SIZE (100 * 1024 * 1024)

#define DO_SEND_INTERVAL_MIN 4
#define DO_SEND_INTERVAL_MAX 32

#define SEND_RECV_BUFF_SIZE 32 * 1024  // 32KB send && recv buffer

/**
 * @brief get system last errno
 *
 * @return int
 */
static int get_last_sys_errno() {
    int err = 0;
    err = errno;
    return err;
}
static void set_last_sys_errno(int err) {
    errno = err;
}

const static std::string PINGPACKETVERSION = "V1";

static top::xbytes_t xquic_client_generate_ping_packet_data(std::size_t server_inbound_port) {
    return top::to_bytes(std::string{PINGPACKETVERSION + std::to_string(server_inbound_port)});
}

static bool xquic_server_handle_ping_packet_data(top::xbytes_t const & ping_packet, std::size_t & peer_inbound_port) {
    std::string ping_str{ping_packet.begin(), ping_packet.end()};
    if (ping_str.size() <= PINGPACKETVERSION.size() || ping_str.substr(0, PINGPACKETVERSION.size()) != PINGPACKETVERSION) {
        xwarn("[xquic_server_engine] recv wrong ping packet data. size:%zu, data:%s", ping_str.size(), ping_str.c_str());
        return false;
    }
    peer_inbound_port = static_cast<std::size_t>(std::atoi(ping_str.substr(PINGPACKETVERSION.size()).c_str()));
    return true;
}

static top::xbytes_t size_to_bytes(std::size_t len) {
    // auto f = len;
    top::xbytes_t res(4, (top::xbyte_t)0);
    for (std::size_t index = 0; index < 4; index++) {
        res[3 - index] = (top::xbyte_t)(len % 256);
        len /= 256;
        if (len == 0)
            break;
    }
    // printf("size_to_bytes %zu %d %d %d %d \n",f, res[0], res[1], res[2], res[3]);
    return res;
}

static std::size_t four_bytes_to_size(top::xbytes_t head) {
    assert(head.size() == 4);
    std::size_t res = 0;
    for (std::size_t index = 0; index < 4; index++) {
        res *= 256;
        res += head[index];
    }
    // printf("four_bytes_to_size %d %d %d %d \n", head[0], head[1], head[2], head[3]);
    return res;
}

// log
void xquic_server_write_log(xqc_log_level_t lvl, const void * buf, size_t count, void * engine_user_data) {
    xinfo("[XQUIC_LOG] %s", (char *)buf);
    return;
}

// user_data as xquic_server_t
void xquic_server_set_event_timer(xqc_msec_t wake_after, void * user_data) {
    // printf("server wake after: %" PRIu64 "  now: %" PRIu64 " \n", wake_after, xqc_now());
    xquic_server_t * server = (xquic_server_t *)user_data;
    struct timeval tv;
    tv.tv_sec = wake_after / 1000000;
    tv.tv_usec = wake_after % 1000000;
    event_add(server->ev_engine, &tv);
}

// user_data as xquic_client_t
void xquic_client_set_event_timer(xqc_msec_t wake_after, void * user_data) {
    // printf("client wake after: %" PRIu64 "  now: %" PRIu64 " \n", wake_after, xqc_now());
    xquic_client_t * client = (xquic_client_t *)user_data;
    struct timeval tv;
    tv.tv_sec = wake_after / 1000000;
    tv.tv_usec = wake_after % 1000000;
    event_add(client->ev_engine, &tv);
}

void xquic_client_alive_timer(int fd, short what, void * user_data) {
    // printf("client try send alive at : %" PRIu64 " \n", xqc_now());
    xquic_client_t * client = (xquic_client_t *)user_data;
    client->quic_engine_do_connect();
    client->quic_engine_do_send();
}

/// server transport cbs
int xquic_server_accept(xqc_engine_t * engine, xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data) {
    xquic_server_t * server = (xquic_server_t *)user_data;
    srv_user_conn_t * srv_user_conn = new srv_user_conn_t;
    srv_user_conn->server = server;
    xqc_conn_set_transport_user_data(conn, srv_user_conn);

    xqc_int_t ret = xqc_conn_get_peer_addr(conn, (struct sockaddr *)&srv_user_conn->peer_addr, sizeof(srv_user_conn->peer_addr), &srv_user_conn->peer_addrlen);
    if (ret != XQC_OK) {
        xwarn("[xquic_server_engine]xquic_server_accept failed get peer addr, ret: %d", ret);
        return -1;
    }

    memcpy(&srv_user_conn->cid, cid, sizeof(*cid));
    xinfo("[xquic_server_engine]xquic_server_accept success get peer cid: %s, ip: %s",
          xqc_scid_str(&srv_user_conn->cid),
          inet_ntoa(((struct sockaddr_in *)&srv_user_conn->peer_addr)->sin_addr));

    return 0;
}
ssize_t xquic_server_write_socket(const unsigned char * buf, size_t size, const struct sockaddr * peer_addr, socklen_t peer_addrlen, void * user_data) {
    srv_user_conn_t * srv_user_conn = (srv_user_conn_t *)user_data;  // user_data may be empty when "reset" is sent
    ssize_t res = 0;
    if (srv_user_conn == nullptr || srv_user_conn->server == nullptr) {
        return XQC_SOCKET_ERROR;
    }

    static ssize_t snd_sum = 0;
    int fd = srv_user_conn->server->fd;

    /* COPY to run corruption test cases */
    unsigned char send_buf[XQC_PACKET_TMP_BUF_LEN];
    size_t send_buf_size = 0;

    if (size > XQC_PACKET_TMP_BUF_LEN) {
        xwarn("[xquic_server_engine]xquic_server_write_socket err: size=%zu is too long", size);
        return XQC_SOCKET_ERROR;
    }
    send_buf_size = size;
    memcpy(send_buf, buf, send_buf_size);

    do {
        set_last_sys_errno(0);
        res = sendto(fd, send_buf, send_buf_size, 0, peer_addr, peer_addrlen);
        if (res < 0) {
            xwarn("[xquic_server_engine]xquic_server_write_socket err %zd %s", res, strerror(get_last_sys_errno()));
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
    srv_user_conn_t * srv_user_conn = (srv_user_conn_t *)user_data;
    xquic_server_t * server = (xquic_server_t *)srv_user_conn->server;

    memcpy(&srv_user_conn->cid, new_cid, sizeof(*new_cid));

    xinfo("[xquic_server_engine]xquic_server_conn_update_cid_notify: RETIRE SCID:%s, SCID:%s, DCID:%s",
          xqc_scid_str(retire_cid),
          xqc_scid_str(new_cid),
          xqc_dcid_str_by_scid(server->engine, new_cid));
}

static void xquic_server_engine_callback(int fd, short what, void * arg) {
    xquic_server_t * server = (xquic_server_t *)arg;

    xqc_engine_main_logic(server->engine);
}
static void xquic_client_engine_callback(int fd, short what, void * arg) {
    xquic_client_t * client = (xquic_client_t *)arg;

    xqc_engine_main_logic(client->engine);
}

/// client tranport cbs
ssize_t xquic_client_write_socket(const unsigned char * buf, size_t size, const struct sockaddr * peer_addr, socklen_t peer_addrlen, void * user) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user;
    ssize_t res = 0;
    if (cli_user_conn == nullptr || cli_user_conn->client == nullptr) {
        return XQC_SOCKET_ERROR;
    }

    int fd = cli_user_conn->fd;

    unsigned char send_buf[XQC_PACKET_TMP_BUF_LEN];
    size_t send_buf_size = 0;

    if (size > XQC_PACKET_TMP_BUF_LEN) {
        xwarn("[xquic_client_engine]xquic_client_write_socket err: size=%zu is too long", size);
        return XQC_SOCKET_ERROR;
    }
    send_buf_size = size;
    memcpy(send_buf, buf, send_buf_size);

    do {
        set_last_sys_errno(0);

        cli_user_conn->last_sock_op_time = xqc_now();

        res = sendto(fd, send_buf, send_buf_size, 0, peer_addr, peer_addrlen);
        if (res < 0) {
            xwarn("[xquic_client_engine]xqc_client_write_socket err %zd %s\n", res, strerror(get_last_sys_errno()));
            if (get_last_sys_errno() == EAGAIN) {
                res = XQC_SOCKET_EAGAIN;
            }
        }
    } while ((res < 0) && (get_last_sys_errno() == EINTR));

    return res;
}
void xquic_client_save_token(const unsigned char * token, unsigned token_len, void * user_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    // char *ip = inet_ntoa(((struct sockaddr_in *)&cli_user_conn->peer_addr)->sin_addr);// might add connection address key
    cli_user_conn->client->xclient_save_token_cb(token, token_len);
}
void xquic_client_save_session_cb(const char * data, size_t data_len, void * user_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    cli_user_conn->client->xclient_save_session_ticket_cb(data, data_len);
}
void xquic_client_save_tp_cb(const char * data, size_t data_len, void * user_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    cli_user_conn->client->xclient_save_transport_parameter_cb(data, data_len);
}
int xquic_client_cert_verify(const unsigned char * certs[], const size_t cert_len[], size_t certs_len, void * conn_user_data) {
    /* self-signed cert used in test cases, return >= 0 means success */
    return 0;
}

xqc_int_t xquic_client_conn_closing_notify(xqc_connection_t * conn, const xqc_cid_t * cid, xqc_int_t err_code, void * conn_user_data) {
    xdbg("[xquic_client_engine]xquic_client_conn_closing_notify: client conn closing: %d", err_code);
    return XQC_OK;
}

/// server app callbacks
/// conns: create/handshake_finish/close
int xquic_server_conn_create_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    return 0;
}
int xquic_server_conn_close_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    srv_user_conn_t * srv_user_conn = (srv_user_conn_t *)user_data;
    xquic_server_t * server = (xquic_server_t *)srv_user_conn->server;
#if defined(DEBUG)
    xqc_conn_stats_t stats = xqc_conn_get_stats(server->engine, cid);
    xdbg("[xquic_server_engine]send_count:%u, lost_count:%u, tlp_count:%u, recv_count:%u, srtt:%" PRIu64 " early_data_flag:%d, conn_err:%d, ack_info:%s",
         stats.send_count,
         stats.lost_count,
         stats.tlp_count,
         stats.recv_count,
         stats.srtt,
         stats.early_data_flag,
         stats.conn_err,
         stats.ack_info);
#endif

    delete srv_user_conn;
    return 0;
}
void xquic_server_conn_handshake_finished(xqc_connection_t * conn, void * user_data, void * conn_proto_data) {
    srv_user_conn_t * srv_user_conn = (srv_user_conn_t *)user_data;
}

/// server app callbacks
/// stream: create/read/close
int xquic_server_stream_create_notify(xqc_stream_t * stream, void * user_data) {
    int ret = 0;

    // get srv_user_conn by stream;
    // so stream read notify could find cb function.
    srv_user_conn_t * srv_user_conn = (srv_user_conn_t *)xqc_get_conn_user_data_by_stream(stream);

    srv_user_stream_t * srv_user_stream = new srv_user_stream_t;
    srv_user_stream->stream = stream;
    srv_user_stream->srv_user_conn = srv_user_conn;
    xqc_stream_set_user_data(stream, srv_user_stream);

    return 0;
}
int xquic_server_stream_read_notify(xqc_stream_t * stream, void * user_data) {
    unsigned char fin = 0;

    srv_user_stream_t * srv_user_stream = (srv_user_stream_t *)user_data;

    top::xbytes_t & result = srv_user_stream->recv_buffer;
    bool & has_recv_block_len = srv_user_stream->has_recv_block_len;
    std::size_t & block_len = srv_user_stream->block_len;

    unsigned char buff[SEND_RECV_BUFF_SIZE] = {0};
    size_t buff_size = SEND_RECV_BUFF_SIZE;
    ssize_t read;
    do {
        read = xqc_stream_recv(stream, buff, buff_size, &fin);
        // printf("recv from stream, size: %zd\n", read);
        if (read == -XQC_EAGAIN) {
            break;
        } else if (read < 0) {
            xdbg("[xquic_server_engine]xqc_stream_recv error %zd", read);
            return 0;
        }

        result.insert(result.end(), buff, buff + read);
        bool no_more_cb{true};
        do {
            no_more_cb = true;
            if (!has_recv_block_len && result.size() > 4) {
                no_more_cb = false;
                block_len = four_bytes_to_size(top::xbytes_t{result.begin(), result.begin() + 4});
                // printf("data block size: %zu\n", block_len);
                has_recv_block_len = true;
                result.erase(result.begin(), result.begin() + 4);
            }

            if (has_recv_block_len && result.size() >= block_len) {
                no_more_cb = false;
                top::xbytes_t data_buffer = top::xbytes_t{result.begin(), result.begin() + block_len};
                if (srv_user_stream->has_recv_ping_packet == false) {
                    // handle ping packet
                    bool res = xquic_server_handle_ping_packet_data(data_buffer, srv_user_stream->peer_inbound_port);
                    if (!res) {
                        xwarn("[xquic_server_engine] ping packet error, close stream");
                        // need to close connection
                        int rc = xqc_stream_close(srv_user_stream->stream);
                        if (rc) {
                            printf("xqc_stream_close error %d \n", rc);
                        }
                        return 0;
                    }
                    srv_user_stream->peer_inbound_addr = std::string(inet_ntoa(((struct sockaddr_in *)&srv_user_stream->srv_user_conn->peer_addr)->sin_addr));
                    xdbg("[xquic_server_engine]get ping packet from ip: %s, port: %zu", srv_user_stream->peer_inbound_addr.c_str(), srv_user_stream->peer_inbound_port);
                    srv_user_stream->has_recv_ping_packet = true;
                } else {
                    // handle module data
                    srv_user_stream->srv_user_conn->server->m_cb(data_buffer, srv_user_stream->peer_inbound_addr, srv_user_stream->peer_inbound_port);
                }
                result.erase(result.begin(), result.begin() + block_len);
                has_recv_block_len = false;
                block_len = 0;
            }
            // printf("result.size(): %zu block_len: %zu \n", result.size(), block_len);
        } while (!no_more_cb);

    } while (read > 0 && !fin);

    // if (fin) {
    //     char * ip = inet_ntoa(((struct sockaddr_in *)&srv_user_stream->srv_user_conn->peer_addr)->sin_addr);
    // }
    return 0;
}
int xquic_server_stream_close_notify(xqc_stream_t * stream, void * user_data) {
    srv_user_stream_t * srv_user_stream = (srv_user_stream_t *)user_data;
    delete srv_user_stream;

    return 0;
}

/// client send stream
int xquic_client_stream_send(xqc_stream_t * stream, void * user_data) {
    ssize_t ret;
    cli_user_stream_t * cli_user_stream = (cli_user_stream_t *)user_data;

    // printf(" cli_user_stream quque size: %zu\n", cli_user_stream->send_queue.size());
    while (!cli_user_stream->send_queue.empty()) {
        // update lastest stream time each time try do send.
        cli_user_stream->latest_update_time = xqc_now();

        // fill data from queue to buffer
        while (!cli_user_stream->send_queue.empty() && cli_user_stream->buffer_right_index < cli_user_stream->send_buffer.capacity() - 1) {
            std::size_t move_size = std::min(cli_user_stream->send_buffer.capacity() - cli_user_stream->buffer_right_index, cli_user_stream->send_queue.size());
            std::copy(cli_user_stream->send_queue.begin(),  // NOLINT
                      cli_user_stream->send_queue.begin() + move_size,
                      cli_user_stream->send_buffer.begin() + cli_user_stream->buffer_right_index);
            cli_user_stream->buffer_right_index += move_size;
            cli_user_stream->send_queue.erase(cli_user_stream->send_queue.begin(), cli_user_stream->send_queue.begin() + move_size);
        }

        auto send_len = cli_user_stream->buffer_right_index - cli_user_stream->send_offset;
        while (send_len) {
            ret = xqc_stream_send(stream,
                                  static_cast<unsigned char *>(&*(cli_user_stream->send_buffer.begin() + cli_user_stream->send_offset)),
                                  send_len,
                                  0);  // fin = 0; normal will never end stream.

            if (ret < 0) {
                // send fail .
                return 0;
            } else {
                cli_user_stream->send_offset += ret;
                // printf(BOLDYELLOW ">>>> send data len: %zu/%zu bri:%zu,offset:%zu \n" RESET, ret, send_len, cli_user_stream->buffer_right_index, cli_user_stream->send_offset);
            }
            send_len = cli_user_stream->buffer_right_index - cli_user_stream->send_offset;
        };
        assert(send_len == 0);
        cli_user_stream->buffer_right_index = 0;
        cli_user_stream->send_offset = 0;
    }
    return 0;
}

/// client app callbacks
/// conns: create/close/handshake_finish/ping_acked
int xquic_client_conn_create_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    // cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    // xqc_conn_set_alp_user_data(conn, cli_user_conn);

    // printf("xqc_conn_is_ready_to_send_early_data:%d\n", xqc_conn_is_ready_to_send_early_data(conn));
    return 0;
}
int xquic_client_conn_close_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * user_data, void * conn_proto_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    xquic_client_t * client = (xquic_client_t *)cli_user_conn->client;
#if defined(DEBUG)
    xqc_conn_stats_t stats = xqc_conn_get_stats(client->engine, cid);
    xdbg("[xquic_client_engine]send_count:%u, lost_count:%u, tlp_count:%u, recv_count:%u, srtt:%" PRIu64 " early_data_flag:%d, conn_err:%d, ack_info:%s",
         stats.send_count,
         stats.lost_count,
         stats.tlp_count,
         stats.recv_count,
         stats.srtt,
         stats.early_data_flag,
         stats.conn_err,
         stats.ack_info);
#endif

    client->release_connection(cli_user_conn);
    return 0;
}
void xquic_client_conn_handshake_finished(xqc_connection_t * conn, void * user_data, void * conn_proto_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    xquic_client_t * client = (xquic_client_t *)cli_user_conn->client;
    xqc_conn_send_ping(client->engine, &cli_user_conn->cid, NULL);
    // xqc_conn_send_ping(client->engine, &cli_user_conn->cid, &g_ping_id);

    xinfo("[xquic_client_engine]xquic_client_conn_handshake_finished: SCID:%s, DCID:%s",
          xqc_scid_str(&cli_user_conn->cid),
          xqc_dcid_str_by_scid(client->engine, &cli_user_conn->cid));
}
void xquic_client_conn_ping_acked_notify(xqc_connection_t * conn, const xqc_cid_t * cid, void * ping_user_data, void * user_data, void * conn_proto_data) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)user_data;
    cli_user_conn->conn_status = cli_conn_status_t::well_connected;
    if (ping_user_data) {
        // printf("====>ping_id:%d\n", *(int *)ping_user_data);
    } else {
        // printf("====>no ping_id\n");
    }
}

/// client app callbacks
/// stream write/read/close
int xquic_client_stream_read_notify(xqc_stream_t * stream, void * user_data) {
    // client would not recv data from server
    return 0;
}
int xquic_client_stream_write_notify(xqc_stream_t * stream, void * user_data) {
    int ret = 0;
    cli_user_stream_t * cli_user_stream = (cli_user_stream_t *)user_data;
    ret = xquic_client_stream_send(stream, cli_user_stream);
    return ret;
}
int xquic_client_stream_close_notify(xqc_stream_t * stream, void * user_data) {
    cli_user_stream_t * cli_user_stream = (cli_user_stream_t *)user_data;
    event_del(cli_user_stream->stream_timeout_event);
    event_free(cli_user_stream->stream_timeout_event);
    cli_user_stream->send_buffer.clear();
    cli_user_stream->send_queue.clear();
    delete cli_user_stream;
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
    // server reply nothing to client
    return;
}
void xqc_server_socket_read_handler(xquic_server_t * server) {
    ssize_t recv_sum = 0;
    struct sockaddr_in peer_addr;
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
            printf("xqc_server_read_handler: packet process err %d \n", result);
            return;
        }
    } while (recv_size > 0);

    // printf("server socket recv size:%zu\n", recv_sum);
    xqc_engine_finish_recv(server->engine);
}
static void xqc_server_socket_event_callback(int fd, short what, void * arg) {
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
static int xquic_client_create_socket(int type, cli_user_conn_t * conn) {
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
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)arg;

    static int restart_after_a_while = 1;

    if (xqc_now() - cli_user_conn->last_sock_op_time < (uint64_t)g_conn_timeout * 1000000) {
        struct timeval tv;
        tv.tv_sec = g_conn_check_timeout;
        tv.tv_usec = 0;
        event_add(cli_user_conn->conn_timeout_event, &tv);
        return;
    }
    xinfo("[xquic_client_engine]connection do timeout close at xqc_now(): %" PRIu64 " , cli_user_conn->last_sock_op_time: %" PRIu64, xqc_now(), cli_user_conn->last_sock_op_time);
    int rc = xqc_conn_close(cli_user_conn->client->engine, &cli_user_conn->cid);
    if (rc) {
        xinfo("[xquic_client_engine]xqc_conn_close error %d \n", rc);
        return;
    }
}

/// client stream timeout callback
static void xquic_client_stream_timeout_callback(int fd, short what, void * arg) {
    cli_user_stream_t * cli_user_stream = (cli_user_stream_t *)arg;

    if ((xqc_now() - cli_user_stream->latest_update_time < (uint64_t)g_stream_handle_timeout * 1000000) ||               // not timeout
        (!cli_user_stream->send_queue.empty() || cli_user_stream->send_offset < cli_user_stream->send_buffer.size())) {  // and not send over
        struct timeval tv;
        tv.tv_sec = g_conn_check_timeout;
        tv.tv_usec = 0;
        event_add(cli_user_stream->stream_timeout_event, &tv);
        return;
    }

    xinfo("[xquic_client_engine]stream do timeout close at xqc_now(): %" PRIu64 ", cli_user_stream->lastest_update_time: %" PRIu64, xqc_now(), cli_user_stream->latest_update_time);
    int rc = xqc_stream_close(cli_user_stream->stream);
    if (rc) {
        printf("xqc_stream_close error %d \n", rc);
        return;
    }
}

/// client socket event
void xquic_client_socket_write_handler(cli_user_conn_t * cli_user_conn) {
    xqc_conn_continue_send(cli_user_conn->client->engine, &cli_user_conn->cid);
}
void xquic_client_socket_read_handler(cli_user_conn_t * cli_user_conn, int fd) {
    ssize_t recv_size = 0;
    ssize_t recv_sum = 0;

    unsigned char packet_buf[XQC_PACKET_TMP_BUF_LEN];

    // static ssize_t last_rcv_sum = 0;
    static ssize_t rcv_sum = 0;

    do {
        recv_size = recvfrom(fd, packet_buf, sizeof(packet_buf), 0, &cli_user_conn->peer_addr, &cli_user_conn->peer_addrlen);
        if (recv_size < 0 && get_last_sys_errno() == EAGAIN) {
            break;
        }

        if (recv_size < 0) {
            xwarn("[xquic_client_engine]recvfrom: recvmsg = %zd(%s)", recv_size, strerror(get_last_sys_errno()));
            break;
        }

        /* if recv_size is 0, break while loop, */
        if (recv_size == 0) {
            break;
        }

        recv_sum += recv_size;
        rcv_sum += recv_size;

        if (cli_user_conn->get_local_addr == 0) {
            cli_user_conn->get_local_addr = 1;
            socklen_t tmp = sizeof(struct sockaddr_in6);
            int ret = getsockname(cli_user_conn->fd, (struct sockaddr *)cli_user_conn->local_addr, &tmp);
            if (ret < 0) {
                printf("getsockname error, errno: %d\n", get_last_sys_errno());
                break;
            }
            cli_user_conn->local_addrlen = tmp;
        }

        uint64_t recv_time = xqc_now();
        cli_user_conn->last_sock_op_time = recv_time;

        static char copy[XQC_PACKET_TMP_BUF_LEN];

        if (xqc_engine_packet_process(cli_user_conn->client->engine,
                                      packet_buf,
                                      recv_size,
                                      cli_user_conn->local_addr,
                                      cli_user_conn->local_addrlen,
                                      &cli_user_conn->peer_addr,
                                      cli_user_conn->peer_addrlen,
                                      (xqc_msec_t)recv_time,
                                      cli_user_conn) != XQC_OK) {
            printf("xqc_client_read_handler: packet process err\n");
            return;
        }

    } while (recv_size > 0);

    // if ((xqc_now() - last_recv_ts) > 200000) {
    //     printf("recving rate: %.3lf Kbps\n", (rcv_sum - last_rcv_sum) * 8.0 * 1000 / (xqc_now() - last_recv_ts));
    //     last_recv_ts = xqc_now();
    //     last_rcv_sum = rcv_sum;
    // }

    // printf("client socket recv size:%zu\n", recv_sum);
    xqc_engine_finish_recv(cli_user_conn->client->engine);
}
static void xquic_client_socket_event_callback(int fd, short what, void * arg) {
    cli_user_conn_t * cli_user_conn = (cli_user_conn_t *)arg;

    if (what & EV_WRITE) {
        xquic_client_socket_write_handler(cli_user_conn);

    } else if (what & EV_READ) {
        xquic_client_socket_read_handler(cli_user_conn, fd);

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
void xquic_client_init_addr(cli_user_conn_t * cli_user_conn, const char * server_addr, int server_port) {
    int ip_type = AF_INET;
    xquic_convert_addr_text_to_sockaddr(ip_type, server_addr, server_port, &cli_user_conn->peer_addr, &cli_user_conn->peer_addrlen);

    cli_user_conn->local_addr = (struct sockaddr *)calloc(1, sizeof(struct sockaddr_in));
    memset(cli_user_conn->local_addr, 0, sizeof(struct sockaddr_in));
    cli_user_conn->local_addrlen = sizeof(struct sockaddr_in);
}
cli_user_conn_t * xquic_client_user_conn_create(const char * server_addr, int server_port, xquic_client_t * client) {
    cli_user_conn_t * cli_user_conn = new cli_user_conn_t;
    cli_user_conn->client = client;
    cli_user_conn->conn_status = cli_conn_status_t::before_connected;
    cli_user_conn->conn_create_time = xqc_now();

    cli_user_conn->conn_timeout_event = event_new(client->eb, -1, 0, xquic_client_conn_timeout_callback, cli_user_conn);
    /* set connection timeout */
    struct timeval tv;
    tv.tv_sec = g_conn_check_timeout;
    tv.tv_usec = 0;
    event_add(cli_user_conn->conn_timeout_event, &tv);

    int ip_type = AF_INET;
    xquic_client_init_addr(cli_user_conn, server_addr, server_port);

    cli_user_conn->fd = xquic_client_create_socket(ip_type, cli_user_conn);
    if (cli_user_conn->fd < 0) {
        printf("xqc_create_socket error\n");
        return NULL;
    }

    cli_user_conn->ev_socket = event_new(client->eb, cli_user_conn->fd, EV_READ | EV_PERSIST, xquic_client_socket_event_callback, cli_user_conn);
    event_add(cli_user_conn->ev_socket, NULL);
    return cli_user_conn;
}

bool xquic_server_t::init(xquic_message_ready_callback cb, std::size_t server_port) {
    m_cb = cb;

    xinfo("[xquic_server_engine] xquic_server init at %u", server_port);

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
    config.cfg_log_level = XQC_LOG_FATAL;

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
    xinfo("[xquic_server_engine] xquic server init success");
    event_base_dispatch(eb);  // block at here
    xwarn("[xquic_server_engine][WARN] xquic server engine destroy!!!");

    xqc_engine_destroy(engine);
    // xqc_server_close_keylog_file(&ctx);
    // xqc_server_close_log_file(&ctx);

    return true;
}

bool xquic_client_t::init(std::size_t server_inbound_port) {
    xinfo("[xquic_client_engine] xquic client init");
    evthread_use_pthreads();

    m_inbound_port = server_inbound_port;
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
    config.cfg_log_level = XQC_LOG_FATAL;

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

    xqc_engine_register_alpn(engine, "transport", 9, &ap_cbs);

    client_alive_timer = event_new(eb, -1, 0, xquic_client_alive_timer, this);
    /* set connection timeout */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_add(client_alive_timer, &tv);

    xinfo("[xquic_client_engine] xquic client init success");
    event_base_dispatch(eb);
    xwarn("[xquic_client_engine][WARN] xquic client engine destroy!!!");
    xqc_engine_destroy(engine);

    // xqc_client_close_keylog_file(&ctx);
    // xqc_client_close_log_file(&ctx);
    return true;
}

/// NOTED: API, this function is used by quic_node thread. be careful about multi-thread issus.
cli_user_conn_t * xquic_client_t::connect(std::string const & server_addr, uint32_t server_port) {
    xdbg("[xquic_client_engine] add new connection to %s:%u", server_addr.c_str(), server_port);
    cli_user_conn_t * cli_user_conn = xquic_client_user_conn_create(server_addr.c_str(), server_port, this);
    cli_user_conn->server_addr = server_addr;
    if (cli_user_conn == nullptr) {
        xwarn("[xquic_client_engine] xquic_client_user_conn_create error");
        return nullptr;
    }
    m_conn_queue.push(cli_user_conn);
    return cli_user_conn;
}

void xquic_client_t::quic_engine_do_connect() {
    auto qsize = m_conn_queue.unsafe_size();
    if (!qsize) {
        return;
    }

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

    auto all_connection = m_conn_queue.wait_and_pop_all();
    for (cli_user_conn_t * conn : all_connection) {
        assert(conn != nullptr);
        assert(conn->conn_status == cli_conn_status_t::before_connected);

        std::string read_token = xclient_read_token();
        if (!read_token.empty()) {
            conn->token = read_token;
        }

        xqc_conn_ssl_config_t conn_ssl_config;
        memset(&conn_ssl_config, 0, sizeof(conn_ssl_config));

        // perf read st/td 's key
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
        cid = xqc_connect(engine,
                          &conn_settings,
                          (unsigned char *)(conn->token.c_str()),
                          conn->token.size(),
                          conn->server_addr.c_str(),
                          0,
                          &conn_ssl_config,
                          &conn->peer_addr,
                          conn->peer_addrlen,
                          "transport",
                          conn);
        if (cid == nullptr) {
            printf("xqc_connect error\n");
            free(conn->local_addr);
            delete conn;
            return;
        }
        memcpy(&conn->cid, cid, sizeof(*cid));
    }
}

void xquic_client_t::quic_engine_do_send() {
    // must set next time do_send wake timer before return!

    /// input: eval send_queue.size()
    /// boundary: minimum rest DO_SEND_INTERVAL_MIN us (sec = 0, usec = DO_SEND_INTERVAL_MIN)
    /// boundary: maximum rest DO_SEND_INTERVAL_MAX us (sec = 0, usec = DO_SEND_INTERVAL_MAX)

    auto qsize = m_send_queue.unsafe_size();
    std::set<cli_user_conn_t *> send_set;  // hold connctions which have new data need to be send.
    if (!qsize) {
        do_send_interval = std::min((uint64_t)2 * do_send_interval, (uint64_t)DO_SEND_INTERVAL_MAX);
    } else {
        do_send_interval = std::max((uint64_t)DO_SEND_INTERVAL_MIN, (uint64_t)(max_send_queue_size - qsize) * DO_SEND_INTERVAL_MAX / max_send_queue_size);

        auto all_message = m_send_queue.wait_and_pop_all();
        xdbg("[xquic_client_engine]quic_engine_do_send: get size %zu ", all_message.size());

        for (auto & send_buffer_ptr : all_message) {
            cli_user_conn_t * cli_user_conn = send_buffer_ptr->cli_user_conn;

            if (cli_user_conn == nullptr || cli_user_conn->conn_status == cli_conn_status_t::after_connected) {
                // this connection should be clean.
                send_buffer_ptr->send_data.clear();
                continue;
            }

            if (cli_user_conn->conn_status != cli_conn_status_t::well_connected) {
                if (cli_user_conn->conn_status == cli_conn_status_t::before_connected && xqc_now() - cli_user_conn->conn_create_time < BEFORE_WELL_CONNECTED_KEEP_MSG_TIMER) {
                    xdbg("[xquic_client_engine]quic_engine_do_send: connection not well connected. messsage shoule be keep , push it back to queue");
                    m_send_queue.push(std::move(send_buffer_ptr));
                    continue;
                }
                // connection error, has not established or after_connected.
                xwarn("[xquic_client_engine]quic_engine_do_send: message(s) for %s discard at quic client engine do send queue",
                      inet_ntoa(((struct sockaddr_in *)&cli_user_conn->peer_addr)->sin_addr));
                send_buffer_ptr->send_data.clear();
                continue;
            }

            if (cli_user_conn->cli_user_stream == nullptr) {
                cli_user_stream_t * cli_user_stream = new cli_user_stream_t;
                xdbg("[xquic_client_engine]new cli_user_stream: (%p)", cli_user_stream);
                cli_user_stream->send_offset = 0;
                cli_user_stream->send_buffer.reserve(SEND_RECV_BUFF_SIZE);
                cli_user_stream->stream = xqc_stream_create(cli_user_conn->client->engine, &cli_user_conn->cid, cli_user_stream);

                if (cli_user_stream->stream == nullptr) {
                    xwarn("[xquic_client_engine]quic_engine_do_send create stream error");
                    delete cli_user_stream;
                    send_buffer_ptr->send_data.clear();
                    continue;
                }
                cli_user_conn->cli_user_stream = cli_user_stream;

                // generate ping packet to send to server
                auto ping_bytes = xquic_client_generate_ping_packet_data(m_inbound_port);
                auto ping_bytes_len_bytes = size_to_bytes(ping_bytes.size());
                cli_user_stream->send_queue.insert(cli_user_stream->send_queue.end(), ping_bytes_len_bytes.begin(), ping_bytes_len_bytes.end());
                cli_user_stream->send_queue.insert(cli_user_stream->send_queue.end(), ping_bytes.begin(), ping_bytes.end());

                // stream timeout callback
                cli_user_stream->stream_timeout_event = event_new(cli_user_conn->client->eb, -1, 0, xquic_client_stream_timeout_callback, cli_user_stream);
                struct timeval tv;
                tv.tv_sec = g_conn_check_timeout;
                tv.tv_usec = 0;
                event_add(cli_user_stream->stream_timeout_event, &tv);
            }
            cli_user_stream_t * cli_user_stream = cli_user_conn->cli_user_stream;
            cli_user_stream->latest_update_time = xqc_now();

            if (cli_user_stream->send_queue.size() < 10000000) {  // 10MB
                // todo might move this outside of quic_engine thread.
                auto len_bytes = size_to_bytes(send_buffer_ptr->send_data.size());
                cli_user_stream->send_queue.insert(cli_user_stream->send_queue.end(), len_bytes.begin(), len_bytes.end());
                cli_user_stream->send_queue.insert(cli_user_stream->send_queue.end(), send_buffer_ptr->send_data.begin(), send_buffer_ptr->send_data.end());
            } else {
                xwarn("[xquic_client_engine]quic_engine_do_send: send_queue_full at conn: %s", xqc_scid_str(&cli_user_conn->cid));
                m_send_queue.push(std::move(send_buffer_ptr));
                continue;
            }

            // ready to send
            send_set.insert(cli_user_conn);
            send_buffer_ptr->send_data.clear();
        }

        for (auto _cli_user_conn : send_set) {
            cli_user_stream_t * cli_user_stream = _cli_user_conn->cli_user_stream;
            assert(cli_user_stream != nullptr);
            xquic_client_stream_send(cli_user_stream->stream, cli_user_stream);
        }
    }
    // xdbg("[xquic_client_engine]interval set be %" PRIu64, do_send_interval);
    struct timeval tv;
    assert(do_send_interval >= DO_SEND_INTERVAL_MIN && do_send_interval <= DO_SEND_INTERVAL_MAX);
    tv.tv_sec = 0;
    tv.tv_usec = do_send_interval;
    event_add(client_alive_timer, &tv);
    return;
}

/// NOTED: API, this function is used by quic_node thread. so be careful about multi-thread issus.
/// api for quic_node , create a event (`client_send_buffer_t`) and let client thread handle this send data buffer.
bool xquic_client_t::send(cli_user_conn_t * cli_user_conn, top::xbytes_t send_data) {
    std::unique_ptr<client_send_buffer_t> send_buffer_ptr = top::make_unique<client_send_buffer_t>();
    send_buffer_ptr->send_data = std::move(send_data);
    send_buffer_ptr->cli_user_conn = cli_user_conn;

    m_send_queue.push(std::move(send_buffer_ptr));

    return true;
}

void xquic_client_t::release_connection(cli_user_conn_t * cli_user_conn) {
    xdbg("[xquic_client_engine] release connection scid: %s", xqc_scid_str(&cli_user_conn->cid));
    cli_user_conn->conn_status = cli_conn_status_t::after_connected;  // ready to be destroy by xquic_node.

    event_del(cli_user_conn->conn_timeout_event);
    event_free(cli_user_conn->conn_timeout_event);
    event_del(cli_user_conn->ev_socket);
    event_free(cli_user_conn->ev_socket);

    cli_user_conn->token.clear();

    free(cli_user_conn->local_addr);
}
