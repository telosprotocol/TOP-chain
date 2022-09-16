#include "xquic_node.h"

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xmetrics/xmetrics.h"
#include "xtransport/xquic_node/quic_cs_engine.h"

#include <xquic/xquic.h>

#include <cassert>

NS_BEG3(top, transport, quic)

xquic_node_t::xquic_node_t(unsigned int const _server_port)
  : m_server_port{_server_port}, m_server_ptr{std::make_shared<xquic_server_t>()}, m_client_ptr{std::make_shared<xquic_client_t>()} {
}

void xquic_node_t::start() {
    assert(!running());

    auto self = shared_from_this();
    top::threading::xbackend_thread::spawn(
        [this, self] { m_server_ptr->init(std::bind(&xquic_node_t::on_quic_message_ready, shared_from_this(), std::placeholders::_1), m_server_port); });

    top::threading::xbackend_thread::spawn([this, self] { m_client_ptr->init(); });

    xinfo("xquic_node_t::start quic node start.");

    running(true);
    assert(running());
}

void xquic_node_t::stop() {
    assert(running());
    running(false);
    xinfo("xquic_node_t::stop quic node stop.");
    assert(!running());
}

void xquic_node_t::register_on_receive_callback(on_receive_callback_t cb) {
    m_cb = cb;
}

void xquic_node_t::unregister_on_receive_callback() {
    m_cb = nullptr;
}

/// NOTED: this API will be used by any thread. need to handle multi-thread issue.
int xquic_node_t::send_data(std::string const & data, std::string const & addr, uint16_t port) {
    XMETRICS_TIME_RECORD("xquic_node_send");
    auto addr_port = addr + std::to_string(port);
    auto bytes_data = top::to_bytes(data);

    std::unique_lock<std::mutex> lock(m_conn_map_mutex);

    // erase closed connection:
    if (m_conn_map.find(addr_port) != m_conn_map.end() && m_conn_map.at(addr_port)->conn_status == cli_conn_status_t::after_connected) {
        delete m_conn_map.at(addr_port);
        xdbg("xquic_node_t::send_data close connection to %s", addr_port.c_str());
        m_conn_map.erase(addr_port);
    }

    if (m_conn_map.find(addr_port) == m_conn_map.end()) {
        xdbg("xquic_node_t::send_data try connect to %s", addr_port.c_str());
        cli_user_conn_t * new_conn = m_client_ptr->connect((char *)addr.c_str(), port);
        if (new_conn == nullptr) {
            xwarn("xquic_node_t::send_data error when try connect to %s", addr_port.c_str());
            return 1;
        }
        assert(new_conn);
        xdbg("xquic_node_t::send_data new connection to %s ptr: %p %p", addr_port.c_str(), new_conn, new_conn->cli_user_stream);
        m_conn_map.insert({addr_port, new_conn});
    }
    cli_user_conn_t * conn = m_conn_map.at(addr_port);
    if (conn == nullptr) {
        assert(false);
        return 1;
    }
    if (conn->conn_status == cli_conn_status_t::before_connected && xqc_now() - conn->conn_create_time > BEFORE_WELL_CONNECTED_KEEP_MSG_TIMER) {
        xwarn("xquic_node_t::send_data connection not well establish to %s", addr_port.c_str());
        return 1;
    }
    if (m_client_ptr->send_queue_full()) {
        xwarn("xquic_node_t::send_data send_queue full");
        return 1;
    }

    assert(conn != nullptr);
    m_client_ptr->send(conn, std::move(bytes_data));

    return 0;  // kadmlia::kKadSuccess
}

static std::size_t global_recv_cnt = 0;

void xquic_node_t::on_quic_message_ready(top::xbytes_t const & bytes) {
    transport::protobuf::RoutingMessage proto_message;
    if (proto_message.ParseFromArray((const char *)bytes.data(), bytes.size()) == false) {
        xwarn("xquic_node_t::on_quic_message_ready parse data to proto message failed, data.size():%zu", bytes.size());
        return;
    }
    proto_message.set_hop_num(proto_message.hop_num() + 1);
    assert(m_cb);

    base::xpacket_t packet;
    // todo
    // packet.set_from_ip_addr();
    // packet.set_from_ip_port(); // this port is meaningless ... but root message need this to reply ...

    assert(m_cb);
    m_cb(proto_message, packet);

    global_recv_cnt++;
    if (global_recv_cnt % 1000 == 0) {
        xdbg("quic node recv: %zu num:%zu at %" PRIu64, bytes.size(), global_recv_cnt, xqc_now());
    }
}

NS_END3