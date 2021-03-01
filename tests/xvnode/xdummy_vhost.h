// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tests/xvnetwork/xdummy_vhost.h"

using top::vnetwork::xcluster_address_t;
using top::vnetwork::xmessage_ready_callback_t;
using top::vnetwork::xmessage_t;
using top::vnetwork::xvnode_address_t;

NS_BEG3(top, tests, vnode)

class xtop_dummy_vhost : public top::tests::vnetwork::xdummy_vhost_t {
public:
    xtop_dummy_vhost() { reg_count = 0; }

    common::xnode_id_t const & host_node_id() const noexcept override { return node_id; }

    common::xnode_id_t const node_id{"test_node_id"};

    void register_message_ready_notify(xvnode_address_t const & vaddr, xmessage_ready_callback_t cb) override {
        reg_count++;
        return;
    }

    void unregister_message_ready_notify(xvnode_address_t const & vaddr) override {
        reg_count--;
        return;
    }

    uint16_t reg_count;
};
using xdummy_vhost_t = xtop_dummy_vhost;

extern xdummy_vhost_t xdummy_vhost;

NS_END3
