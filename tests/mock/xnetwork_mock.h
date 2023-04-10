#pragma once

#include "xbase/xutl.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xunit_service/xcons_utl.h"
#include <atomic>
#include <mutex>
#include <random>
#include <string>
#include <vector>
#include <cinttypes>

namespace top {
namespace mock {
using namespace xunit_service;
using vnetwork::xmessage_t;
using vnetwork::xvhost_face_t;
using vnetwork::xvnetwork_message_ready_callback_t;
class network_mock : public vnetwork::xtop_vnetwork_driver_face {
public:
    using msg_map = std::map<common::xmessage_category_t, xvnetwork_message_ready_callback_t>;
    msg_map m_msg_cbs;
    using nodes = std::map<xvip2_t, network_mock *, xvip2_compare>;
    static nodes                s_nodes;
    static std::recursive_mutex s_mutex;
    top::common::xnode_address_t            m_address;
    std::recursive_mutex        m_mutex;

public:
    explicit network_mock(top::common::xnode_address_t address) : m_address(address) {
        {
            std::lock_guard<std::recursive_mutex> guard(s_mutex);
            auto vip = xcons_utl::to_xip2(address);
            s_nodes[vip] = this;
        }
    }

    ~network_mock() {
        // remove
    }

public:
    virtual void
    start() {
    }

    virtual void
    stop() {
    }

    /**
     * \brief Register the message for a specified message category.
     * \param message_category Message category for the callback registered.
     * \param cb The message callback.
     */
    virtual void
    register_message_ready_notify(common::xmessage_category_t const  message_category,
                                  xvnetwork_message_ready_callback_t cb) {
        std::lock_guard<std::recursive_mutex> guard(m_mutex);
        xinfo("[xunitservice] register_mg_cb %x %p %s", message_category, this, address().to_string().c_str());
        auto msg_it = m_msg_cbs.find(message_category);
        if (msg_it == m_msg_cbs.end()) {
            m_msg_cbs[message_category] = cb;
        }
    }

    /**
     * \brief Un-register the message notify for a message category.
     * \param message_category Message category to un-register.
     */
    virtual void
    unregister_message_ready_notify(common::xmessage_category_t const message_category) {
        std::lock_guard<std::recursive_mutex> guard(m_mutex);
        xinfo("[xunitservice] unregister_mg_cb %x %p %s", message_category, this, address().to_string().c_str());
        auto msg_it = m_msg_cbs.find(message_category);
        if (msg_it != m_msg_cbs.end()) {
            m_msg_cbs.erase(msg_it);
        }
    }

    virtual
    common::xnetwork_id_t
    network_id() const noexcept {
        return m_address.network_id();
    }

    virtual top::common::xnode_address_t
    address() const {
        return m_address;
    }

    void recv(top::common::xnode_address_t const &from, xmessage_t const &message) {
        auto category = get_message_category(message.id());
        std::lock_guard<std::recursive_mutex> guard(m_mutex);
        auto cb = m_msg_cbs.find(category);
        auto time = base::xtime_utl::gmttime_ms();
        if (cb != m_msg_cbs.end()) {
            if (from == address()) {
                return;
            }
            xinfo("[recv] %x from: %s to %s", message.id(), from.to_string().c_str(), address().to_string().c_str());
            cb->second(from, message, time);
        } else {
            xwarn("[recv] invalid %x from: %s to %s", message.id(), from.to_string().c_str(), address().to_string().c_str());
        }
    }

    virtual void send_to(top::common::xnode_address_t const & to, xmessage_t const & message) {
        {
            std::lock_guard<std::recursive_mutex> guard(s_mutex);
            auto vip = xcons_utl::to_xip2(to);
            auto net_it = s_nodes.find(vip);
            if (net_it != s_nodes.end()) {
                auto from = xcons_utl::to_xip2(address());
                if (from.low_addr != vip.low_addr) {
                    xinfo("[send_to] %x from: %s to %s", message.id(), address().to_string().c_str(), to.to_string().c_str());
                    net_it->second->recv(address(), message);
                }
            } else {
                xwarn("[send_to] send_to invalid %x from: %s to %" PRIu64 "", message.id(), address().to_string().c_str(), vip.low_addr);
            }
        }
    }

    virtual void
    broadcast(xmessage_t const &message) {
        {
            std::lock_guard<std::recursive_mutex> guard(s_mutex);
            auto                                  from = address();
            for (auto pair : s_nodes) {
                if (xcons_utl::to_xip2(from).low_addr != pair.first.low_addr) {
                    xinfo("[broadcast] %x from: %s to %" PRIu64 "", message.id(), address().to_string().c_str(), pair.first.low_addr);
                    pair.second->recv(address(), message);
                }
            }
        }
    }

    /**
     * \brief Forward a broadcast to dst address. dst address must be a cluster address.
     */
    virtual void
    forward_broadcast_message(xmessage_t const &      message,
                              top::common::xnode_address_t const &dst) {
        broadcast(message);
    }

    virtual void
    broadcast_to(top::common::xnode_address_t const &dst, xmessage_t const &message) {
        broadcast(message);
    }

    virtual common::xnode_id_t const &
    host_node_id() const noexcept {
        return m_address.node_id();
    }

    virtual top::common::xnode_address_t
    parent_group_address() const {
        return m_address;
    }

    virtual std::vector<top::common::xnode_address_t>
    child_group_address(common::xelection_round_t const &ver) const {
        std::vector<top::common::xnode_address_t> groups;
        return groups;
    }

    virtual std::vector<top::common::xnode_address_t>
    neighbor_addresses() const {
        std::vector<top::common::xnode_address_t> groups;
        return groups;
    }

    virtual std::map<common::xslot_id_t, data::xnode_info_t>
    neighbors_info2() const {
        std::map<common::xslot_id_t, data::xnode_info_t> slot_map;
        return slot_map;
    }

    virtual std::map<common::xslot_id_t, data::xnode_info_t>
    parents_info2() const {
        std::map<common::xslot_id_t, data::xnode_info_t> parents_info2;
        return parents_info2;
    }

    virtual std::map<common::xslot_id_t, data::xnode_info_t>
    children_info2(common::xgroup_id_t const &gid, common::xelection_round_t const &version) const {
        std::map<common::xslot_id_t, data::xnode_info_t> child;
        return child;
    }

    virtual observer_ptr<xvhost_face_t>
    virtual_host() const noexcept {
        return nullptr;
    }

    /**
     * \brief Get the working type of this virtual node.
     * \return The virtual node type.
     */
    virtual common::xnode_type_t
    type() const noexcept {
        return m_address.type();
    }

    virtual top::common::xnode_address_t
    archive_address() const {
        return m_address;
    }

    virtual std::vector<top::common::xnode_address_t>
    archive_addresses(top::common::xnode_type_t) const {
        std::vector<top::common::xnode_address_t> archive_addresses;
        return archive_addresses;
    }

    virtual std::vector<std::uint16_t>
    table_ids() const {
        std::vector<std::uint16_t> tables{1};
        return tables;
    }

    virtual std::vector<top::common::xnode_address_t>
    parent_addresses() const {
        std::vector<top::common::xnode_address_t> parent_addresses;
        return parent_addresses;
    }

    virtual void send_to(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) {

    }
    virtual void broadcast(common::xip2_t const & to, xmessage_t const & message, std::error_code & ec) {

    }
};

using std::default_random_engine;
// random discard message netwo TODO(add random factor and discard implement)
class xrandom_discard_network_mock : public network_mock {
public:
    explicit xrandom_discard_network_mock(top::common::xnode_address_t address, uint rate) : network_mock(address), m_rate(rate) {}
protected:
    inline bool discard() {
        auto random = m_random_engine();
        return (random % 100) <= m_rate;
    }

public:
    virtual void send_to(top::common::xnode_address_t const & to, xmessage_t const & message) {
        {
            std::lock_guard<std::recursive_mutex> guard(s_mutex);
            auto net_it = s_nodes.find(xcons_utl::to_xip2(to));
            auto from = address();
            if (net_it != s_nodes.end()) {
                if (xcons_utl::to_xip2(from).low_addr != xcons_utl::to_xip2(to).low_addr) {
                    xinfo("[sendto] %x from: %s to %s", message.id(), from.to_string().c_str(), to.to_string().c_str());
                    if (!discard()) {
                        net_it->second->recv(from, message);
                    } else {
                        xinfo("[sendto] discard %x from: %s to %s", message.id(), from.to_string().c_str(), to.to_string().c_str());
                    }
                }
            } else {
                xwarn("[sendto] mismatch %x from: %s to %s", message.id(), from.to_string().c_str(), to.to_string().c_str());
            }
        }
    }

    virtual void
    broadcast(xmessage_t const &message) {
        {
            std::lock_guard<std::recursive_mutex> guard(s_mutex);
            auto                                  from = address();
            for (auto pair : s_nodes) {
                if (xcons_utl::to_xip2(from).low_addr != pair.first.low_addr) {
                    xinfo("[broadcast] %x from: %s to %" PRIu64 "", message.id(), from.to_string().c_str(), pair.first.low_addr);
                    if (!discard()) {
                        pair.second->recv(from, message);
                    } else {
                        xinfo("[broadcast] discard %x from: %s to %" PRIu64 "", message.id(), from.to_string().c_str(), pair.first.low_addr);
                    }
                }
            }
        }
    }

private:
    uint                  m_rate;
    default_random_engine m_random_engine;
};

} // namespace mock
} // namespace top
