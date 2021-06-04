// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xpbase/base/kad_key/kadmlia_key.h"

// #include "common/xxhash.h"
// #include "common/xdfcurve.h"
// #include "common/xaes.h"
// #include "common/secp256k1.h"
// #include "common/sha2.h"

#include "xbase/xutl.h"
#include "xutility/xhash.h"

#include <ostream>
#include <sstream>
#include <assert.h>
// #include "xcrypto/xckey.h"
// #include "xutility/xhash.h"
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


ServiceType CreateServiceType(common::xip2_t const &xip) {
    uint64_t res{0};
    set_kad_network_id(res, xip.network_id().value());
    set_kad_zone_id(res, xip.zone_id().value());
    set_kad_cluster_id(res, xip.cluster_id().value());
    set_kad_group_id(res, xip.group_id().value());
    // todo charles test no height in xip.
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
    std::string node_id_hash_32((char *)v.data(), v.size());
    uint64_t high, low;
    std::string _h = node_id_hash_32.substr(0, 16);
    std::string _l = node_id_hash_32.substr(16, node_id_hash_32.size() - 16);
    memcpy(&high, _h.c_str(), _h.size());
    memcpy(&low, _l.c_str(), _l.size());

    auto _xvip = xvip2_t();
    _xvip.low_addr = low;
    _xvip.high_addr = high;
    reset_network_id_to_xip2(_xvip);
    set_network_id_to_xip2(_xvip, kRootNetworkId);
    common::xip2_t xip(_xvip);
    assert(xip.network_id().value() == kRootNetworkId);
    xdbg("Charles Debug get root kad key: xip:%s node_id: %s", xip.to_string().c_str(),node_id.c_str());
    return GetKadmliaKey(xip);
}

KadmliaKey::KadmliaKey(common::xip2_t const &xip) : xip_(xip) {}

std::vector<uint64_t> split(std::string s, char sep) {
    std::istringstream iss(s);
    std::vector<uint64_t> res;
    std::string buffer;
    while (getline(iss, buffer, sep)) {
        uint64_t tmp;
        std::istringstream b(buffer);
        b >> std::hex >> tmp;
        res.push_back(tmp);
    }
    return res;
}

KadmliaKey::KadmliaKey(std::string const &from_str) {
    xdbg("Charles Debug KadmliaKey %s",from_str.c_str());
    assert(from_str.size() == 33 && from_str[16] == '.');
    auto low_str = from_str.substr(0,16);
    auto high_str = from_str.substr(17,32);
    std::istringstream low_sstr(low_str);
    std::istringstream high_sstr(high_str);
    uint64_t low_part,high_part; 
    low_sstr>>std::hex>>low_part;
    high_sstr>>std::hex>>high_part;
    xdbg("Charles Debug KadmliaKey %s,%s", low_str.c_str(), high_str.c_str());
    xip_ = common::xip2_t{low_part,high_part};
}

std::string KadmliaKey::Get() {
    xdbg("Charles Debug KadmliaKey size: 2 %zu : %s", xip_.to_string().size(), xip_.to_string().c_str());
    return xip_.to_string();
}

ServiceType KadmliaKey::GetServiceType() { return CreateServiceType(xip_); }

} // namespace base

} // namespace top