// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xvnode/xvnode_factory_face.h"
#include "xvnetwork/xvhost_face.h"

NS_BEG3(top, tests, vnode)

class xdummy_vnode_factory : public top::vnode::xvnode_factory_face_t {
protected:
    top::observer_ptr<top::vnetwork::xvhost_face_t> vhost_;

public:

    xdummy_vnode_factory(xdummy_vnode_factory &&) = default;
    xdummy_vnode_factory & operator=(xdummy_vnode_factory &&) = default;

    explicit xdummy_vnode_factory(observer_ptr<top::vnetwork::xvhost_face_t> vhost) noexcept : vhost_{std::move(vhost)} {
    }

    std::shared_ptr<top::vnode::xvnode_face_t> create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const &) const override {
        return nullptr;
    }
};

extern xdummy_vnode_factory dummy_vnode_factory;

NS_END3
