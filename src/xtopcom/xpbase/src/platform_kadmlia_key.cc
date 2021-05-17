//
//  platform_kadmlia_key.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/kad_key/platform_kadmlia_key.h"

#include <assert.h>
#include <stdlib.h>
#include <limits>

#include "common/xdfcurve.h"
#include "common/sha2.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/rand_util.h"
#include "xpbase/base/xip_parser.h"

namespace top {

namespace base {

PlatformKadmliaKey::PlatformKadmliaKey()
        : KadmliaKey(), reserve_(0), private_key_(), public_key_() {
    assert(!global_node_id_hash.empty());
    CreatePrivateAndPublicKey();
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    node_id_hash_ = global_node_id_hash;
}

PlatformKadmliaKey::PlatformKadmliaKey(uint64_t service_type) : KadmliaKey() {
    assert(!global_node_id_hash.empty());
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    UnionPlatformServiceType tmp_type;
    tmp_type.service_type = service_type;
    set_xnetwork_id(tmp_type.type_detail.network_id);
    set_zone_id(tmp_type.type_detail.zone_id);
    set_cluster_id(tmp_type.type_detail.cluster_id);
    set_group_id(tmp_type.type_detail.group_id);
    set_node_id(tmp_type.type_detail.node_id);
    set_network_type(tmp_type.type_detail.network_type);
    node_id_hash_ = global_node_id_hash;
}

PlatformKadmliaKey::PlatformKadmliaKey(
        const std::string& str_for_hash,
        uint64_t service_type) : KadmliaKey() {
    auto sha128 = GetStringSha128(str_for_hash);  // NOLINT
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    memcpy(kadmlia_key_.kad_key_detail.sha128, sha128.c_str(), sha128.size());

    UnionPlatformServiceType tmp_type;
    tmp_type.service_type = service_type;
    set_xnetwork_id(tmp_type.type_detail.network_id);
    set_zone_id(tmp_type.type_detail.zone_id);
    set_cluster_id(tmp_type.type_detail.cluster_id);
    set_group_id(tmp_type.type_detail.group_id);
    set_node_id(tmp_type.type_detail.node_id);
    set_network_type(tmp_type.type_detail.network_type);
    node_id_hash_ = sha128;
}


PlatformKadmliaKey::PlatformKadmliaKey(uint32_t network_id, uint8_t network_type) : KadmliaKey() {
    assert(!global_node_id_hash.empty());
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    set_xnetwork_id(network_id);
    set_network_type(network_type);
    node_id_hash_ = global_node_id_hash;
}

PlatformKadmliaKey::PlatformKadmliaKey(const base::XipParser& xip) : KadmliaKey(xip) {
    assert(!global_node_id_hash.empty());
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            global_node_id_hash.c_str(),
            global_node_id_hash.size());
    node_id_hash_ = global_node_id_hash;
}

PlatformKadmliaKey::PlatformKadmliaKey(const base::XipParser& xip, const std::string& node_id) : KadmliaKey(xip) {
    auto node_id_hash = GetStringSha128(node_id);  // NOLINT
    xip_.xip(kadmlia_key_.kad_key_high_low.high, kadmlia_key_.kad_key_high_low.low);
    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            node_id_hash.c_str(),
            node_id_hash.size());
    node_id_hash_ = node_id_hash;
}

// for kroot
PlatformKadmliaKey::PlatformKadmliaKey(const std::string& node_id, bool hash_tag)
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
    std::string node_id_hash_32 = GetStringSha256(node_id);
    base::XipParser xip(node_id_hash_32.substr(16, node_id_hash_32.size() - 16));
    SetXip(xip);
    set_xnetwork_id(kRoot);

    kadmlia_key_.kad_key_detail.reserve = reserve_;
    memcpy(
            kadmlia_key_.kad_key_detail.sha128,
            node_id_hash_32.c_str(),
            16);
    node_id_hash_ = node_id_hash_32.substr(0,16);
}

PlatformKadmliaKey::PlatformKadmliaKey(const std::string& str_key) {
    InitWithStringKey(str_key);
    node_id_hash_ = std::string(kadmlia_key_.kad_key_detail.sha128, 16);
}

PlatformKadmliaKey::~PlatformKadmliaKey() {}

const std::string& PlatformKadmliaKey::GetNodeIdHash() {
    return  node_id_hash_;
}

uint64_t PlatformKadmliaKey::GetServiceType() {
    UnionPlatformServiceType tmp_type{ {
        xip_.xnetwork_id(), 0xFF, 0xFF,
        0xFF, 0xFF, xip_.network_type()
    } };
    return tmp_type.service_type;
}

uint64_t PlatformKadmliaKey::GetServiceType(uint8_t network_type) {
    UnionPlatformServiceType tmp_type{ {
        xip_.xnetwork_id(), 0xFF, 0xFF,
        0xFF, 0xFF, network_type
    } };
    return tmp_type.service_type;
}

const std::string& PlatformKadmliaKey::Get() {
    if (str_.empty()) {
        SetDefaultXipValue();
        str_ = std::string(kadmlia_key_.c_str, sizeof(kadmlia_key_.c_str));
    }
    return str_;
}

void base::PlatformKadmliaKey::SetDefaultXipValue() {
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

void PlatformKadmliaKey::InitWithStringKey(const std::string& str_key) {
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

bool PlatformKadmliaKey::CreatePrivateAndPublicKey() {
    // sha256 create private key
    SHA256_CTX* sha256_context = static_cast<SHA256_CTX*>(malloc(sizeof(SHA256_CTX)));
    if (sha256_context == NULL) {
        TOP_ERROR("create sha256 context failed!");
        return false;
    }

    key25519 prikey = { 0 };
    sha256_Init(sha256_context);
    int64_t time_seed = base::GetRandomInt64() % std::numeric_limits<int64_t>::max();
    sha256_Update(sha256_context, (const uint8_t*)&time_seed, sizeof(time_seed));
    sha256_Update(sha256_context, (const uint8_t*)prikey, sizeof(prikey));
    std::string rand_str = RandomString(kNodeIdSize);
    sha256_Update(sha256_context, (const uint8_t*)rand_str.c_str(), rand_str.size());
    uint8_t raw_uint8[32];
    sha256_Final(sha256_context, (uint8_t*)raw_uint8);  // NOLINT
    memcpy(prikey, raw_uint8, sizeof(raw_uint8));
    private_key_ = std::string((char*)prikey, sizeof(prikey)); // NOLINT

    // create public key with private key
    key25519 pubkey = { 0 };
    keygen25519(prikey, pubkey);
    public_key_ = std::string((const char*)pubkey, sizeof(key25519));
    free(sha256_context);
    return true;
}

}  // namespase base

}  // namespace top
