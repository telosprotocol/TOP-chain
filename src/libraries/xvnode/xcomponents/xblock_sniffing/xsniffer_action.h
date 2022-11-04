// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"

#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnode/xvnode_face.h"

NS_BEG4(top, vnode, components, sniffing)

class xtop_sniffer_action {
public:
    static void call(observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                     common::xaccount_address_t const & address,
                     std::string const & action_name,
                     std::string const & action_params,
                     const uint64_t timestamp);
    static void call(observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                     common::xaccount_address_t const & source_address,
                     common::xaccount_address_t const & target_address,
                     std::string const & action_name,
                     std::string const & action_params,
                     uint64_t timestamp);
    static void broadcast(observer_ptr<vnode::xvnode_face_t> const & vnode, data::xblock_ptr_t const & block_ptr, common::xnode_type_t types);
};

using xsniffer_action_t = xtop_sniffer_action;

NS_END4
