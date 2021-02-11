// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xcommon/xrotation_aware.h"
#include "xcommon/xfadable.h"
#include "xcommon/xenable_synchronization.h"

NS_BEG2(top, vnode)

class xtop_vnode_face : public xbasic_runnable_t<xtop_vnode_face>
                      , public common::xbasic_fadable_t<xtop_vnode_face>
                      , public common::xbasic_rotation_aware_t<xtop_vnode_face>
                      , public common::xenable_synchronization_t<xtop_vnode_face> {
public:
    xtop_vnode_face() = default;
    xtop_vnode_face(xtop_vnode_face const &) = delete;
    xtop_vnode_face & operator=(xtop_vnode_face const &) = delete;
    xtop_vnode_face(xtop_vnode_face &&) = default;
    xtop_vnode_face & operator=(xtop_vnode_face &&) = default;
    ~xtop_vnode_face() override = default;

    virtual common::xnode_type_t type() const = 0;
    virtual common::xversion_t version() const = 0;
    virtual common::xnode_address_t address() const = 0;
    // virtual std::vector<common::xnode_address_t> neighbors() const = 0;
};
using xvnode_face_t = xtop_vnode_face;

NS_END2
