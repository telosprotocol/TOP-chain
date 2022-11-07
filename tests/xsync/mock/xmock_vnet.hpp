// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xvnetwork_driver_face.h"
#include "xvnetwork/xvhost_face_fwd.h"
#include "xcommon/xversion.h"
#include "tests/xvnetwork/xdummy_vnetwork_driver.h"
#include <mutex>
#include "xmock_vhost.hpp"
#include "../../mock/xmock_network.hpp"

namespace top { namespace mock {

using namespace vnetwork;
using namespace data;

class xmock_vnet_t : public top::tests::vnetwork::xtop_dummy_vnetwork_driver {
public:
    xmock_vnet_t(const vnetwork::xvnode_address_t &addr, const std::vector<xmock_node_info_t*> &vector_neighbor_info,
                const std::vector<xmock_node_info_t*> &vector_parent_info, const std::vector<xmock_node_info_t*> &vector_child_info,
                const std::vector<vnetwork::xvnode_address_t> &all_addr, const std::vector<vnetwork::xvnode_address_t> &all_archive_addr,
                std::shared_ptr<xmock_vhost_t> &vhost):
    m_address(addr),
    m_all_addr(all_addr),
    m_all_archive_addr(all_archive_addr),
    m_vhost(vhost) {
        for (auto it: vector_neighbor_info) {
            const vnetwork::xvnode_address_t &addr = it->m_addr;
            m_neighbor_addr.push_back(addr);
        }

        for (auto it: vector_parent_info) {
            const vnetwork::xvnode_address_t &addr = it->m_addr;
            m_parent_addr.push_back(addr);
        }

        if (m_parent_addr.size() > 0) {
            const vnetwork::xvnode_address_t &tmp = m_parent_addr[0];
            m_parent_group_address = top::vnetwork::address_cast<top::common::xnode_type_t::group>(tmp);
        }

        for (auto it: vector_child_info) {
            const vnetwork::xvnode_address_t &addr = it->m_addr;
            m_child_addr.push_back(addr);
        }

        if (m_child_addr.size() > 0) {
            const vnetwork::xvnode_address_t &tmp = m_child_addr[0];
            m_child_group_address = top::vnetwork::address_cast<top::common::xnode_type_t::group>(tmp);
        }

        // must bigger than addr.ver
        std::size_t id = 2;
        common::xelection_round_t ver{id};

        //printf("%s %d %d\n", addr.to_string().c_str(), m_neighbor_key.size(), m_parent_key.size());
    }

    ~xmock_vnet_t() override {}

    /**
     * \brief Register the message for a specified message category.
     * \param message_category Message category for the callback registered.
     * \param cb The message callback.
     */
    void
    register_message_ready_notify(common::xmessage_category_t const message_category,
                                  xvnetwork_message_ready_callback_t cb) override {
        //m_cb = cb;
        m_cbs.insert(std::make_pair(message_category, cb));
    }

    /**
     * \brief Un-register the message notify for a message category.
     * \param message_category Message category to un-register.
     */
    void
    unregister_message_ready_notify(common::xmessage_category_t const message_category) override {}

    common::xnetwork_id_t
    network_id() const noexcept override {
        return m_network_id;
    }

    xvnode_address_t
    address() const override {
        return m_address;
    }

    void send_to(xvnode_address_t const & to, xmessage_t const & message) override {
        m_vhost->send(message, m_address, to);
    }

    void
    broadcast(xmessage_t const & message) override {
        m_vhost->broadcast(message, m_address);
    }

    common::xnode_id_t const &
    host_node_id() const noexcept override {
        return m_node_id;
    }

    std::map<common::xslot_id_t, data::xnode_info_t> 
    neighbors_info2() const override {

        std::map<common::xslot_id_t, data::xnode_info_t> infos;
        for (auto &it: m_neighbor_addr) {
            const common::xnode_address_t &addr = it;
            data::xnode_info_t info;
            info.address = addr;
            infos.insert(std::make_pair(addr.slot_id(), info));
        }

        return infos;
    }

    std::map<common::xslot_id_t, data::xnode_info_t> 
    parents_info2() const override {

        std::map<common::xslot_id_t, data::xnode_info_t> infos;
        for (auto &it: m_parent_addr) {
            const common::xnode_address_t &addr = it;
            data::xnode_info_t info;
            info.address = addr;
            infos.insert(std::make_pair(addr.slot_id(), info));
        }

        return infos;
    }

    std::vector<xvnode_address_t>
    archive_addresses() const override {
        return m_all_archive_addr;
    }

    /**
     * \brief Get the working type of this virtual node.
     * \return The virtual node type.
     */
    common::xnode_type_t
    type() const noexcept override {
        return real_part_type(m_address.cluster_address().type());
    }

    std::vector<std::uint16_t>
    table_ids() const override {

        std::vector<std::uint16_t> ids;
        for (uint16_t i=0; i<enum_vbucket_has_tables_count; i++) {
            ids.push_back(i);
        }
        return ids;
    }

public:

    void on_message(const vnetwork::xvnode_address_t &addr, const vnetwork::xmessage_t &msg) {
        auto const message_id = msg.id();
        auto const message_cagegory = get_message_category(message_id);

        auto const it = m_cbs.find(message_cagegory);
        if (it != m_cbs.end()) {
            vnetwork::xvnetwork_message_ready_callback_t cb = it->second;
            cb(addr, msg, 0);
        } else {
            assert(0);
        }
    }

private:
    common::xnetwork_id_t m_network_id;
    vnetwork::xvnode_address_t m_address;
    common::xnode_id_t m_node_id;
    std::vector<vnetwork::xvnode_address_t> m_neighbor_addr;
    std::map<xvnode_address_t, xcrypto_key_t<pub>> m_neighbor_key;
    std::vector<vnetwork::xvnode_address_t> m_parent_addr;
    std::map<xvnode_address_t, xcrypto_key_t<pub>> m_parent_key;

    std::vector<vnetwork::xvnode_address_t> m_child_addr;
    std::map<xvnode_address_t, xcrypto_key_t<pub>> m_child_key;

    std::vector<vnetwork::xvnode_address_t> m_all_addr;
    std::vector<vnetwork::xvnode_address_t> m_all_archive_addr;

    vnetwork::xvnode_address_t m_parent_group_address;
    vnetwork::xvnode_address_t m_child_group_address;

    std::map<common::xmessage_category_t, vnetwork::xvnetwork_message_ready_callback_t> m_cbs;

    std::shared_ptr<xmock_vhost_t> m_vhost{};
};

}
}
