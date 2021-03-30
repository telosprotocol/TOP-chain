// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <set>
#include "xvnetwork/xaddress.h"

NS_BEG2(top, sync)

class xdeceit_node_manager_t {
protected:
    using check_deceit_node_cb = std::function<void(const std::set<vnetwork::xaccount_address_t>&)>;
public:
    xdeceit_node_manager_t() {
    }
    virtual ~xdeceit_node_manager_t() {}

    void add_deceit_node(const vnetwork::xvnode_address_t& addr) {
        std::unique_lock<std::mutex> lock(m_lock);
        auto it = m_deceit_nodes.find(addr.account_address());
        if (it == m_deceit_nodes.end()) {
            m_deceit_nodes.insert(addr.account_address());
        }
    }

    void filter_deceit_nodes(check_deceit_node_cb cb) {
        std::unique_lock<std::mutex> lock(m_lock);
        cb(m_deceit_nodes);
    }

    bool is_deceit_node(const vnetwork::xvnode_address_t& addr) {
        std::unique_lock<std::mutex> lock(m_lock);
        auto it = m_deceit_nodes.find(addr.account_address());
        if (it == m_deceit_nodes.end()) {
            return false;
        }

        return true;
    }

protected:
    std::mutex m_lock;
    std::set<vnetwork::xaccount_address_t> m_deceit_nodes;
};

NS_END2