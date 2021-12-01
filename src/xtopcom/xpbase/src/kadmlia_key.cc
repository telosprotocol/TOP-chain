// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xpbase/base/kad_key/kadmlia_key.h"


#include "xbase/xutl.h"
#include "xutility/xhash.h"

#include <assert.h>
#include <ostream>
#include <sstream>
#include "xmetrics/xmetrics.h"
namespace top {

namespace base {
/**
 * [network_id:21]-[zone_id:7]-[cluster_id:7]-[group_id:8]-[height:21]
 */
#define kRootNetworkId 0xFFFFFULL
#define set_kad_network_id(service_id, network_id)                             \
    ((service_id |= ((uint64_t)(network_id)&0x1FFFFFULL) << 43))
#define set_kad_zone_id(service_id, zone_id)                                   \
    ((service_id |= ((uint64_t)(zone_id)&0x7F) << 36))
#define set_kad_cluster_id(service_id, cluster_id)                             \
    ((service_id |= ((uint64_t)(cluster_id)&0x7F) << 29))
#define set_kad_group_id(service_id, group_id)                                 \
    ((service_id |= ((uint64_t)(group_id)&0xFF) << 21))
#define set_kad_height(service_id, height)                                     \
    ((service_id |= ((uint64_t)(height)&0x1FFFFFULL)))
// #define set_kad_root(service_id) ((service_id |= (0xFFFFFFULL)))

ServiceType::ServiceType(uint64_t type) : m_type{type} {
    std::string info{""};
    info += " [network " + std::to_string((type >> 43)) + "]-";
    info += "[zone " + std::to_string((type << 21) >> (21 + 36)) + "]-";
    info += "[cluster " + std::to_string((type << 28) >> (28 + 29)) + "]-";
    info += "[group " + std::to_string((type << 35) >> (35 + 21)) + "]-";
    info += "[height " + std::to_string((type << 43) >> 43) + "]";
    m_info = info;
}

#define IS_BROADCAST_HEIGHT(service_type_value)                                \
    ((service_type_value & 0x1FFFFFULL) == 0x1FFFFFULL)
#define BROADCAST_HEIGHT(service_type_value) ((service_type_value | 0x1FFFFFULL))

bool ServiceType::operator==(ServiceType const &other) const {
    if (IS_BROADCAST_HEIGHT(other.value()) || IS_BROADCAST_HEIGHT(m_type)) {
        return BROADCAST_HEIGHT(other.value()) == BROADCAST_HEIGHT(m_type);
    } else {
        return other.value() == m_type;
    }
}
bool ServiceType::operator!=(ServiceType const &other) const {
    return !(*this == other);
}
bool ServiceType::operator<(ServiceType const &other) const {
    return m_type < other.value();
}

bool ServiceType::IsNewer(ServiceType const &other, int _value) const {
    if (IS_BROADCAST_HEIGHT(other.value()) || IS_BROADCAST_HEIGHT(m_type))
        return false;
    if (BROADCAST_HEIGHT(other.value()) == BROADCAST_HEIGHT(m_type)) {
        if ((m_type & 0x1FFFFFULL) >= ((other.value() & 0x1FFFFFULL) + _value))
            return true;
    }
    return false;
}

bool ServiceType::IsBroadcastService() const {
    return IS_BROADCAST_HEIGHT(m_type);
}

uint64_t ServiceType::value() const { return m_type; }

std::string ServiceType::info() const { return m_info; }

ServiceType CreateServiceType(common::xip2_t const &xip) {
    uint64_t res{0};
    set_kad_network_id(res, xip.network_id().value());
    set_kad_zone_id(res, xip.zone_id().value());
    set_kad_cluster_id(res, xip.cluster_id().value());
    set_kad_group_id(res, xip.group_id().value());
    set_kad_height(res, xip.height());
    return ServiceType(res);
}

base::KadmliaKeyPtr GetKadmliaKey(common::xip2_t const &xip) {
    auto kad_key_ptr = std::make_shared<base::KadmliaKey>(xip);
    return kad_key_ptr;
}
base::KadmliaKeyPtr GetKadmliaKey(std::string const &node_id) {
    auto kad_key_ptr = std::make_shared<base::KadmliaKey>(node_id);
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetRootKadmliaKey(std::string const &node_id) {
    top::utl::xsha2_256_t h;
    top::uint256_t v;
    h.reset();
    h.update(node_id);
    h.get_hash(v);
    XMETRICS_GAUGE(metrics::cpu_hash_256_GetRootKadmliaKey_calc, 1);
    std::string node_id_hash_32((char *)v.data(), v.size());
    uint64_t high, low;
    std::string _h = node_id_hash_32.substr(0, 8);
    std::string _l = node_id_hash_32.substr(8, 8);
    memcpy(&high, _h.c_str(), _h.size());
    memcpy(&low, _l.c_str(), _l.size());

    auto _xvip = xvip2_t();
    _xvip.low_addr = low;
    _xvip.high_addr = high;
    reset_network_id_to_xip2(_xvip);
    set_network_id_to_xip2(_xvip, kRootNetworkId);
    common::xip2_t xip(_xvip);
    assert(xip.network_id().value() == kRootNetworkId);
    // xdbg("[GetRootKadmliaKey] get root kad key: xip:%s node_id: %s",
    //      xip.to_string().c_str(), node_id.c_str());
    return GetKadmliaKey(xip);
}

KadmliaKey::KadmliaKey(common::xip2_t const &xip) : xip_(xip) {}

KadmliaKey::KadmliaKey(std::string const &from_str) {
    // xdbg("KadmliaKey from_str %s", from_str.c_str());
    assert(from_str.size() == 33 && from_str[16] == '.');
    auto low_str = from_str.substr(0, 16);
    auto high_str = from_str.substr(17, 32);
    std::istringstream low_sstr(low_str);
    std::istringstream high_sstr(high_str);
    uint64_t low_part, high_part;
    low_sstr >> std::hex >> low_part;
    high_sstr >> std::hex >> high_part;
    // xdbg("KadmliaKey from_str %s,%s", low_str.c_str(), high_str.c_str());
    xip_ = common::xip2_t{low_part, high_part};
}

std::string KadmliaKey::Get() {
    return xip_.to_string();
}

ServiceType KadmliaKey::GetServiceType() { return CreateServiceType(xip_); }

} // namespace base

} // namespace top