//
//  chain_kadmlia_key.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/kad_key/chain_kadmlia_key.h"

#include <cassert>
#include <iostream>

#include "common/xxhash.h"
#include "common/xdfcurve.h"
#include "common/xaes.h"
#include "common/secp256k1.h"
#include "common/sha2.h"
#include "xpbase/base/top_log.h"
#include "xbasic/xobject_ptr.h"
#include "xutility/xhash.h"

namespace top {

namespace base {

ChainKadmliaKey::ChainKadmliaKey()
        : KadmliaKey(),
          kadmlia_key_(),
          reserve_(0) {
    assert(!global_node_id_hash.empty());
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    node_id_hash_ = global_node_id_hash;
}

ChainKadmliaKey::ChainKadmliaKey(const base::XipParser& xip)
        : KadmliaKey(xip),
          kadmlia_key_(),
          reserve_(0) {
    assert(!global_node_id_hash.empty());
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    reserve_ = 0;
    node_id_hash_ = global_node_id_hash;
}

ChainKadmliaKey::ChainKadmliaKey(const base::XipParser& xip, const std::string& node_id)
        : KadmliaKey(xip),
          kadmlia_key_(),
          reserve_(0) {
    auto node_id_hash = GetStringSha128(node_id);  // NOLINT
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            node_id_hash.c_str(),
            node_id_hash.size());
    node_id_hash_ = node_id_hash;
}

// for kroot
ChainKadmliaKey::ChainKadmliaKey(const std::string& node_id, bool hash_tag)
        : KadmliaKey(),
          kadmlia_key_(),
          reserve_(0) {
    if (!hash_tag) {
        auto node_id_hash = GetStringSha128(node_id);  // NOLINT
        kadmlia_key_.kad_key_detail.reserve = reserve_;
        memcpy(
                kadmlia_key_.kad_key_detail.sha128,
                node_id_hash.c_str(),
                node_id_hash.size());
        node_id_hash_ = node_id_hash;
        return;
    }

    // create xip
    top::utl::xsha2_256_t h;
    top::uint256_t v;
    h.reset();
    h.update(node_id);
    h.get_hash(v);
	std::string node_id_hash_32((char*) v.data(), v.size());
//    std::string node_id_hash_32 = GetStringSha256(node_id);
    base::XipParser xip(node_id_hash_32.substr(16, node_id_hash_32.size() - 16)); // pos start from 16, size is 16
    SetXip(xip);
    set_xnetwork_id(kRoot);

    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            node_id_hash_32.c_str(),
            16);
    node_id_hash_ = node_id_hash_32.substr(0,16);

}

ChainKadmliaKey::ChainKadmliaKey(const std::string& str_key)
        : KadmliaKey(),
          kadmlia_key_(),
          reserve_(0) {
    InitWithStringKey(str_key);
    node_id_hash_ = std::string(kadmlia_key_.kad_key_detail.sha128, 16);
}

ChainKadmliaKey::ChainKadmliaKey(uint64_t service_type)
        : KadmliaKey(GetXipFromServiceType(service_type)),
          kadmlia_key_(),
          reserve_(0) {
    assert(!global_node_id_hash.empty());
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    reserve_ = 0;
    node_id_hash_ = global_node_id_hash;
}

ChainKadmliaKey::ChainKadmliaKey(uint32_t reserve)
        : KadmliaKey(),
          kadmlia_key_(),
          reserve_(reserve) {
    assert(!global_node_id_hash.empty());
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    kadmlia_key_.kad_key_high_low.reserve = reserve;
    node_id_hash_ = global_node_id_hash;
}

ChainKadmliaKey::~ChainKadmliaKey() {}

const std::string& ChainKadmliaKey::GetNodeIdHash() {
    return  node_id_hash_;
}

const std::string& ChainKadmliaKey::Get() {
    if (str_.empty()) {
        SetDefaultXipValue();
        str_ = std::string(kadmlia_key_.c_str, sizeof(kadmlia_key_.c_str));
    }
    return str_;
}

std::string ChainKadmliaKey::GetPrivateKey() {
    return "";
}

std::string ChainKadmliaKey::GetPublicKey() {
    return "";
}

uint64_t ChainKadmliaKey::GetServiceType() {
    return CreateServiceType(xip_);
}

uint64_t ChainKadmliaKey::GetServiceType(uint8_t network_type) {
    XipParser xip = xip_;
    xip.set_network_type(network_type);
    return CreateServiceType(xip);
}

void ChainKadmliaKey::InitWithStringKey(const std::string& str_key) {
    assert(str_key.size() == sizeof(kadmlia_key_.c_str));
    memcpy(kadmlia_key_.c_str, str_key.c_str(), str_key.size());
    set_xnetwork_id(kadmlia_key_.kad_key_detail.xnetwork_id);
    set_zone_id(kadmlia_key_.kad_key_detail.zone_id);
    set_cluster_id(kadmlia_key_.kad_key_detail.cluster_id);
    set_group_id(kadmlia_key_.kad_key_detail.group_id);
    set_node_id(kadmlia_key_.kad_key_detail.node_id);
    set_network_type(kadmlia_key_.kad_key_detail.network_type);
    set_xaddress_domain_xip(kadmlia_key_.kad_key_detail.xaddress_domain_xip);
    set_xip_type(kadmlia_key_.kad_key_detail.xip_type);
    set_xnetwork_version(kadmlia_key_.kad_key_detail.xnetwork_version);
    set_xinterface_id(kadmlia_key_.kad_key_detail.xinterface_id);
    set_process_id(kadmlia_key_.kad_key_detail.process_id);
    set_router_id(kadmlia_key_.kad_key_detail.router_id);
    set_switch_id(kadmlia_key_.kad_key_detail.switch_id);
    set_local_id(kadmlia_key_.kad_key_detail.local_id);
}

void base::ChainKadmliaKey::SetDefaultXipValue() {
    kadmlia_key_.kad_key_detail.xnetwork_id = xip_.xnetwork_id();
    kadmlia_key_.kad_key_detail.zone_id = xip_.zone_id();
    kadmlia_key_.kad_key_detail.cluster_id = xip_.cluster_id();
    kadmlia_key_.kad_key_detail.group_id = xip_.group_id();
    kadmlia_key_.kad_key_detail.node_id = xip_.node_id();
    kadmlia_key_.kad_key_detail.network_type = xip_.network_type();
    kadmlia_key_.kad_key_detail.xaddress_domain_xip = xip_.xaddress_domain_xip();
    kadmlia_key_.kad_key_detail.xip_type = xip_.xip_type();
    kadmlia_key_.kad_key_detail.xnetwork_version = xip_.xnetwork_version();
    kadmlia_key_.kad_key_detail.xinterface_id = xip_.xinterface_id();
    kadmlia_key_.kad_key_detail.process_id = xip_.process_id();
    kadmlia_key_.kad_key_detail.router_id = xip_.router_id();
    kadmlia_key_.kad_key_detail.switch_id = xip_.switch_id();
    kadmlia_key_.kad_key_detail.local_id = xip_.local_id();
}

}  // namespase base

}  // namespace top
