// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tests/xvnetwork/xdummy_vhost.h"
#include "xvnetwork/xaddress.h"

namespace top { namespace mock {

using namespace vnetwork;

using xmock_transport_unicast_callback = std::function<int32_t(vnetwork::xvnode_address_t const &, vnetwork::xvnode_address_t const &, const vnetwork::xmessage_t &)>;
using xmock_transport_broadcast_callback = std::function<int32_t(vnetwork::xvnode_address_t const &, const vnetwork::xmessage_t &)>;

using xmock_send_filter_callback = std::function<int32_t(const vnetwork::xmessage_t &, vnetwork::xmessage_t &)>;
using xmock_recv_filter_callback = std::function<int32_t(const vnetwork::xmessage_t &, vnetwork::xmessage_t &)>;

using xmock_vhost_message_callback = std::function<void(const vnetwork::xvnode_address_t &, const vnetwork::xmessage_t &)>;

class xmock_vhost_t : public top::tests::vnetwork::xtop_dummy_vhost {
public:

    xmock_vhost_t(const std::string &str_node_id, const vnetwork::xvnode_address_t &addr, xmock_transport_unicast_callback unicast_cb, xmock_transport_broadcast_callback broadcast_cb):
    m_node_id(str_node_id),
    m_address(addr),
    m_unicast_cb(unicast_cb),
    m_broadcast_cb(broadcast_cb) {
    }

    void attach(xmock_vhost_message_callback cb) {
        m_message_cb = cb;
    }

    xvnode_address_t const &
    address() const noexcept {
        return m_address;
    }

    common::xnode_id_t const & host_node_id() const noexcept override { return m_node_id; }

    void send(vnetwork::xmessage_t const & message, xvnode_address_t const & src, xvnode_address_t const & dst) override {
        m_unicast_cb(src, dst, message);
    }

    void broadcast(common::xnode_address_t const & src, common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) override {
    }

public:

    void set_send_filter(xmock_send_filter_callback cb) {
        m_lock.lock();
        m_send_filter_cb = cb;
        m_lock.unlock();
    }

    void get_send_filter(xmock_send_filter_callback &cb) {
        m_lock.lock();
        cb = m_send_filter_cb;
        m_lock.unlock();
    }

    void set_recv_filter(xmock_recv_filter_callback cb) {
        m_lock.lock();
        m_recv_filter_cb = cb;
        m_lock.unlock();
    }

    void get_recv_filter(xmock_recv_filter_callback &cb) {
        m_lock.lock();
        cb = m_recv_filter_cb;
        m_lock.unlock();
    }

    void on_message(const vnetwork::xvnode_address_t &addr, const vnetwork::xmessage_t &msg) {
        m_message_cb(addr, msg);
    }

private:
    common::xnode_id_t m_node_id;
    vnetwork::xvnode_address_t m_address;

    xmock_vhost_message_callback m_message_cb;

    std::mutex m_lock;
    xmock_send_filter_callback m_send_filter_cb;
    xmock_recv_filter_callback m_recv_filter_cb;

    xmock_transport_unicast_callback m_unicast_cb;
    xmock_transport_broadcast_callback m_broadcast_cb;
};

}
}