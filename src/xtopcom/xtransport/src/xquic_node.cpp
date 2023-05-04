#include "xtransport/xquic_node/xquic_node.h"

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xmetrics/xmetrics.h"
#include "xtransport/xquic_node/quic_cert_data.inc"
#include "xtransport/xquic_node/quic_cs_engine.h"

#include <xquic/xquic.h>

#include <cassert>
#include <fstream>

NS_BEG3(top, transport, quic)

// same as DEFAULT_QUIC_SERVER_PORT_DETLA in xudp_socket.cc
#if defined(DEBUG) || defined(XBUILD_CI) || defined(XBUILD_DEV)
static const std::size_t DEFAULT_QUIC_SERVER_PORT_DETLA = 1000;  // quic_port is greater than p2p_port;
#else
static const std::size_t DEFAULT_QUIC_SERVER_PORT_DETLA = 1;  // quic_port is greater than p2p_port;
#endif

void xquic_node_t::check_cert_file() {
    std::ofstream cert_file_hd;
    cert_file_hd.open("./server.crt");
    cert_file_hd << quic_cert_data::server_crt;
    cert_file_hd.close();

    std::ofstream cert_key_hd;
    cert_key_hd.open("./server.key");
    cert_key_hd << quic_cert_data::server_key;
    cert_key_hd.close();
}

xquic_node_t::xquic_node_t(std::size_t _p2p_inbound_port)
  : m_p2p_inbound_port{_p2p_inbound_port}
  , m_server_port{m_p2p_inbound_port + DEFAULT_QUIC_SERVER_PORT_DETLA}
  , m_server_ptr{std::make_shared<xquic_server_t>()}
  , m_client_ptr{std::make_shared<xquic_client_t>()} {
}

void xquic_node_t::start() {
    assert(!running());

    check_cert_file();

    auto self = shared_from_this();
    top::threading::xbackend_thread::spawn([this, self] {
        m_server_ptr->init(std::bind(&xquic_node_t::on_quic_message_ready, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), m_server_port);
    });

    top::threading::xbackend_thread::spawn([this, self] { m_client_ptr->init(m_p2p_inbound_port); });

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
    // XMETRICS_TIME_RECORD("xquic_node_send");
    auto addr_port = addr + ":" + std::to_string(port);
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
        cli_user_conn_t * new_conn = m_client_ptr->connect(addr, port);
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

void xquic_node_t::on_quic_message_ready(top::xbytes_t const & bytes, std::string const & peer_ip, std::size_t peer_inbound_port) {
    transport::protobuf::RoutingMessage proto_message;
    if (proto_message.ParseFromArray((const char *)bytes.data() + enum_xbase_header_len, bytes.size() - enum_xbase_header_len) == false) {
        xwarn("xquic_node_t::on_quic_message_ready parse data to proto message failed, data.size():%zu", bytes.size());
        return;
    }
    XMETRICS_FLOW_COUNT("xquic_message_recv_bytes", bytes.size());
    proto_message.set_hop_num(proto_message.hop_num() + 1);
    assert(m_cb);

    base::xpacket_t packet;
    packet.set_from_ip_addr(peer_ip);
    packet.set_from_ip_port(peer_inbound_port);

    assert(m_cb);
    m_cb(proto_message, packet);
}

NS_END3