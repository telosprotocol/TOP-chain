#pragma once
#include "quic_cs_engine.h"
#include "xbase/xpacket.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xrunnable.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport_fwd.h"

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

NS_BEG3(top, transport, quic)

class xquic_node_t
  : public top::xbasic_runnable_t<xquic_node_t>
  , public std::enable_shared_from_this<xquic_node_t> {
public:
    xquic_node_t(unsigned int const _server_port);

public:
    void start() override;

    void stop() override;

    void register_on_receive_callback(on_receive_callback_t cb);

    void unregister_on_receive_callback();

public:
    int send_data(std::string const & data, std::string const & addr, uint16_t port);

    void on_quic_message_ready(top::xbytes_t const & bytes);

private:
    /// configs:
    unsigned int m_server_port;

    on_receive_callback_t m_cb;

private:
    xquic_server_t m_server;
    xquic_client_t m_client;

    std::mutex m_conn_map_mutex;
    std::unordered_map<std::string, cli_user_conn_t *> m_conn_map;
};

NS_END3