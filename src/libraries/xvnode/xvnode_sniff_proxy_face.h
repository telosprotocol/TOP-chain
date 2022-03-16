// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xvnode/xcomponents/xblock_sniffing/xsniffer_config.h"

NS_BEG2(top, vnode)

class xtop_vnode_sniff_proxy_face {
public:
    xtop_vnode_sniff_proxy_face() = default;
    xtop_vnode_sniff_proxy_face(xtop_vnode_sniff_proxy_face const &) = delete;
    xtop_vnode_sniff_proxy_face & operator=(xtop_vnode_sniff_proxy_face const &) = delete;
    xtop_vnode_sniff_proxy_face(xtop_vnode_sniff_proxy_face &&) = default;
    xtop_vnode_sniff_proxy_face & operator=(xtop_vnode_sniff_proxy_face &&) = default;
    virtual ~xtop_vnode_sniff_proxy_face() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reg(common::xnode_address_t const & address, components::sniffing::xsniffer_config_t const & config) = 0;
    virtual void unreg(common::xnode_address_t const & address) = 0;
};
using xvnode_sniff_proxy_face_t = xtop_vnode_sniff_proxy_face;

NS_END2
