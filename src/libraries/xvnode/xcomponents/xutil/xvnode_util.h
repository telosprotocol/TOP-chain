// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xstore/xstore_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"

NS_BEG4(top, vnode, components, util)

class xtop_vnode_util {
public:
    static void call(observer_ptr<store::xstore_face_t> store,
                     observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                     common::xaccount_address_t const & address,
                     std::string const & action_name,
                     std::string const & action_params,
                     const uint64_t timestamp);
    static void call(observer_ptr<store::xstore_face_t> store,
                     observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool,
                     common::xaccount_address_t const & source_address,
                     common::xaccount_address_t const & target_address,
                     std::string const & action_name,
                     std::string const & action_params,
                     uint64_t timestamp);
    static void broadcast(observer_ptr<vnetwork::xvnetwork_driver_face_t> const & driver, xblock_ptr_t const & block_ptr, common::xnode_type_t types);
};

using xvnode_util_t = xtop_vnode_util;

NS_END4
