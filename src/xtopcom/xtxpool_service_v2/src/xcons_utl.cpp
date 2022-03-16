// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xcons_utl.h"
NS_BEG2(top, xtxpool_service_v2)
static xvip2_t broadcast_ip{(uint64_t)-1, (uint64_t)-1};

xvip2_t xcons_utl::to_xip2(const common::xnode_address_t & address, bool bwith_version) {
    xvip2_t xip = address.xip2();
    if (!bwith_version) {
        xip = address.xip2().group_xip2();
    }
    return xip;
}

xvip2_t xcons_utl::erase_version(const xvip2_t & xip) {
    xvip2_t xip_copy{xip.low_addr, (uint64_t)-1};
    assert(get_network_ver_from_xip2(xip_copy) == 0);
    assert(get_network_ver_from_xip2(xip) == 0);
    // We don't use network version field for now.
    // but in case some peer node modifies this field, any correct node should force obey this rule.
    reset_network_ver_to_xip2(xip_copy);
    set_node_id_to_xip2(xip_copy, 0xFFF);
    return xip_copy;
}

bool xcons_utl::xip_equals(const xvip2_t & left, const xvip2_t & right) {
    return is_xip2_equal(left, right);
}

bool xcons_utl::is_broadcast_address(const xvip2_t & addr) {
    return xip_equals(broadcast_ip, addr);
}

NS_END2
