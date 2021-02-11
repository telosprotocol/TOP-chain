// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xelection/xcache/xgroup_element.h"
#include "xvnode/xvnode_face.h"
#include "xvnode/xvnode_factory_face_fwd.h"

#include <memory>

NS_BEG2(top, vnode)

class xtop_vnode_factory_face {
public:
    xtop_vnode_factory_face() = default;
    xtop_vnode_factory_face(xtop_vnode_factory_face const &) = delete;
    xtop_vnode_factory_face & operator=(xtop_vnode_factory_face const &) = delete;
    xtop_vnode_factory_face(xtop_vnode_factory_face &&) = default;
    xtop_vnode_factory_face & operator=(xtop_vnode_factory_face &&) = default;
    virtual ~xtop_vnode_factory_face() = default;

    virtual std::shared_ptr<xvnode_face_t> create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group) const = 0;
};
using xvnode_factory_face_t = xtop_vnode_factory_face;

NS_END2
