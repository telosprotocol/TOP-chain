// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/local_node_info.h"

// #include "common/sha2.h"
// #include "common/xaes.h"
// #include "common/xdfcurve.h"
#include "xbase/xhash.h"
#include "xpbase/base/top_log.h"

#include <limits>

namespace top {

namespace kadmlia {

LocalNodeInfo::LocalNodeInfo() {
}
LocalNodeInfo::~LocalNodeInfo() {
}

bool LocalNodeInfo::Init(const std::string & local_ip, uint16_t local_port, base::KadmliaKeyPtr kadmlia_key) {
    local_ip_ = local_ip;
    local_port_ = local_port;
    kadmlia_key_ = kadmlia_key;
    if (true) {
        public_ip_ = local_ip_;
        public_port_ = local_port_;
    }
    TOP_KINFO("local_node_start: kad_key[%s] service_type[%s] public_ip[%s] public_port[%d]",
              kad_key().c_str(),
              kadmlia_key_->GetServiceType().info().c_str(),
              public_ip_.c_str(),
              public_port_);
    service_type_ = kadmlia_key_->GetServiceType();

    hash64_ = base::xhash64_t::digest(kadmlia_key_->Get());
    return true;
}

void LocalNodeInfo::Reset() {
    kadmlia_key_ = nullptr;
    local_ip_ = "";
    local_port_ = 0;
    public_ip_ = "";
    public_port_ = 0;
}

std::string LocalNodeInfo::kad_key() {
    return kadmlia_key_->Get();
}
std::string LocalNodeInfo::root_kad_key() {
    return global_xid->Get();
}
void LocalNodeInfo::set_public_ip(const std::string & ip) {
    std::unique_lock<std::mutex> lock(public_mutex_);
    public_ip_ = ip;
    TOP_KINFO("kad_key[%s] set public_ip %s", kad_key().c_str(), public_ip_.c_str());
}

void LocalNodeInfo::set_public_port(uint16_t port) {
    std::unique_lock<std::mutex> lock(public_mutex_);
    public_port_ = port;
    TOP_KINFO("kad_key[%s] set public_port %u", kad_key().c_str(), public_port_);
}

}  // namespace kadmlia

}  // namespace top
