// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xvnetwork/xvnetwork_driver_face.h"


NS_BEG3(top, vnode, details)

class xtop_vnode_role_proxy_face /*: public xbasic_runnable_t<xtop_vnode_role_proxy_face>*/ {
public:
    xtop_vnode_role_proxy_face() = default;
    xtop_vnode_role_proxy_face(xtop_vnode_role_proxy_face const &) = delete;
    xtop_vnode_role_proxy_face & operator=(xtop_vnode_role_proxy_face const &) = delete;
    xtop_vnode_role_proxy_face(xtop_vnode_role_proxy_face &&) = default;
    xtop_vnode_role_proxy_face & operator=(xtop_vnode_role_proxy_face &&) = default;
    virtual ~xtop_vnode_role_proxy_face() = default;


    virtual void create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) = 0;
    virtual void change(common::xnode_address_t const & address, common::xlogic_time_t start_time) = 0;
    virtual void fade(common::xnode_address_t const & address) = 0;
    virtual void unreg(common::xnode_address_t const & address) = 0;
    virtual void destroy(common::xnode_address_t const & address) = 0;
};

NS_END3

NS_BEG2(top, vnode)
using xvnode_role_proxy_face_t = details::xtop_vnode_role_proxy_face;
NS_END2
