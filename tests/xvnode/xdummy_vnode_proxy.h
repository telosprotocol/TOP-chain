// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xvnetwork/xvhost_face.h"
#include "xvnode/xvnode_role_proxy_face.h"
#include "xvnode/xvnode_sniff_proxy_face.h"

NS_BEG3(top, tests, vnode)

class xdummy_vnode_proxy : public top::vnode::xvnode_role_proxy_face_t {
public:
    xdummy_vnode_proxy(xdummy_vnode_proxy &&) = default;
    xdummy_vnode_proxy & operator=(xdummy_vnode_proxy &&) = default;

    explicit xdummy_vnode_proxy() noexcept {
    }

    void create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) override {
    }
    void change(common::xnode_address_t address, common::xlogic_time_t start_time) override {
    }
    void unreg(common::xnode_address_t address) override {
    }
    void destroy(xvip2_t xip2) override {
    }
};

class xdummy_vnode_sniff_proxy : public top::vnode::xvnode_sniff_proxy_face_t {
public:
    xdummy_vnode_sniff_proxy(xdummy_vnode_sniff_proxy &&) = default;
    xdummy_vnode_sniff_proxy & operator=(xdummy_vnode_sniff_proxy &&) = default;

    explicit xdummy_vnode_sniff_proxy() noexcept {
    }
    virtual void start() {
    }
    virtual void stop() {
    }
    virtual void reg(common::xnode_address_t const & address, top::vnode::components::sniffing::xvnode_sniff_config_t const & config) {
    }
    virtual void unreg(common::xnode_address_t const & address) {
    }
};

extern xdummy_vnode_proxy dummy_vnode_proxy;
extern xdummy_vnode_sniff_proxy dummy_vnode_sniff_proxy;

NS_END3
