// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xBFT/xconsobj.h"
#include "xbase/xbase.h"
#include "xbase/xthread.h"
#include "xcommon/xmessage_category.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_driver_face.h"
NS_BEG2(top, xtxpool_service_v2)
// #define     set_network_id_to_xip2_ex(_xip2,id)            ( (_xip2.low_addr) |= (((uint64_t)(id)  & 0xFFFFFF)  << 32) )
// #define     set_zone_id_to_xip2_ex(_xip2,id)               (  (_xip2.low_addr) |= (((uint64_t)(id) & 0x7F)       << 25) )
// #define     set_cluster_id_to_xip2_ex(_xip2,id)            (  (_xip2.low_addr) |= (((uint64_t)(id) & 0x7F)       << 18) )
// #define     set_group_id_to_xip2_ex(_xip2,id)              (  (_xip2.low_addr) |= (((uint64_t)(id) & 0xFF)       << 10) )
// #define     set_node_id_to_xip2_ex(_xip2,id)               (  (_xip2.low_addr) |= (((uint64_t)(id) & 0x3FF)           ) )
// #define     set_network_ver_to_xip2_ex(_xip2,ver)          ( (_xip2.low_addr) |= (((uint64_t)(ver) & 0xFF)      << 56) )
// #define     set_xip2_type_to_xip2_ex(_xip2,type)           (  (_xip2.high_addr) |= (((uint64_t)(type)   & 0x03)   << 61) )

// #define     get_network_ver_from_xip2_ex(_xip2)            (int32_t)(((_xip2.low_addr) >> 56) & 0xFF)
// #define     get_network_id_from_xip2_ex(_xip2)             (int32_t)(((_xip2.low_addr) >> 32) & 0xFFFFFF)   //24bit
// #define     get_zone_id_from_xip2_ex(_xip2)                (int32_t)(((_xip2.low_addr) >> 25) & 0x7F)            //7bit
// #define     get_cluster_id_from_xip2_ex(_xip2)             (int32_t)(((_xip2.low_addr) >> 18) & 0x7F)            //7bit
// #define     get_group_id_from_xip2_ex(_xip2)               (int32_t)(((_xip2.low_addr) >> 10) & 0xFF)            //8bit
// #define     get_node_id_from_xip2_ex(_xip2)                (int32_t)(((_xip2.low_addr)      ) & 0x3FF)           //10bit
// #define     get_xip2_type_from_xip2_ex(_xip2)              (int32_t)(((_xip2.high_addr) >> 61) & 0x03)      //2bit

class xcons_utl {
public:
    static xvip2_t                 to_xip2(const common::xnode_address_t & address, bool bwith_version = true);
    static xvip2_t                 erase_version(const xvip2_t & xip);
    static bool                    xip_equals(const xvip2_t & left, const xvip2_t & right);
    static bool                    is_broadcast_address(const xvip2_t & addr);
};

NS_END2
