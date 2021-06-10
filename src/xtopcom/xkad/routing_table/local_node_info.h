// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xkad/nat_detect/nat_defines.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xip_parser.h"

#include <assert.h>

#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace top {

namespace kadmlia {

class LocalNodeInfo {
public:
    LocalNodeInfo();
    ~LocalNodeInfo();

    bool Init(const std::string & local_ip, uint16_t local_port, bool first_node, base::KadmliaKeyPtr kadmlia_key);
    void Reset();
    bool IsPublicNode();
    std::string kad_key();
    std::string xid();
    std::string id();
    base::XipParser & GetXipParser();
    std::string local_ip() {
        return local_ip_;
    }
    uint16_t local_port() {
        return local_port_;
    }
    void set_local_port(uint16_t local_port) {
        local_port_ = local_port;
    }
    bool first_node() {
        return first_node_;
    }
    void set_first_node(bool first_node) {
        first_node_ = first_node;
    }
    const std::string & private_key() {
        return private_key_;
    }
    const std::string & public_key() {
        return public_key_;
    }
    std::string public_ip() {
        std::unique_lock<std::mutex> lock(public_mutex_);
        return public_ip_;
    }

    uint16_t public_port() {
        std::unique_lock<std::mutex> lock(public_mutex_);
        return public_port_;
    }
    // int32_t nat_type() { return nat_type_; }
    void set_public_ip(const std::string & ip);
    void set_public_port(uint16_t port);
    base::ServiceType service_type() {
        return service_type_;
    }
    void set_service_type(base::ServiceType service_type) {
        service_type_ = service_type;
    }
    uint32_t routing_table_id() {
        return routing_table_id_;
    }
    void set_routing_table_id(uint32_t routing_table_id) {
        routing_table_id_ = routing_table_id;
    }

    base::KadmliaKeyPtr kadmlia_key() {
        std::lock_guard<std::mutex> lock(kadkey_mutex_);
        return kadmlia_key_;
    }
    void set_kadmlia_key(base::KadmliaKeyPtr kadmlia_key) {
        std::lock_guard<std::mutex> lock(kadkey_mutex_);
        kadmlia_key_ = kadmlia_key;
    }

    inline bool use_kad_key() {
        if (kadmlia_key_) {
            return true;
        }
        return false;
    }
    uint16_t rpc_http_port() {
        return rpc_http_port_;
    }
    uint16_t rpc_ws_port() {
        return rpc_ws_port_;
    }
    void set_rpc_http_port(uint16_t http_port) {
        rpc_http_port_ = http_port;
    }
    void set_rpc_ws_port(uint16_t ws_port) {
        rpc_ws_port_ = ws_port;
    }
    bool is_root() {
        return is_root_;
    }
    void set_is_root(bool root) {
        is_root_ = root;
    }
    uint64_t hash64() {
        return hash64_;
    }

private:
    std::string local_ip_;
    uint16_t local_port_{0};
    uint16_t rpc_http_port_{0};
    uint16_t rpc_ws_port_{0};
    bool first_node_{false};
    std::string private_key_;
    std::string public_key_;
    std::mutex public_mutex_;
    std::string public_ip_;
    uint16_t public_port_{0};
    // int32_t nat_type_{kNatTypeUnknown};
    base::ServiceType service_type_{kInvalidType};
    uint32_t routing_table_id_{0};

    std::mutex kadkey_mutex_;
    base::KadmliaKeyPtr kadmlia_key_{nullptr};

    // key is node_id, value is dynamicxip distribute by node_id
    std::map<std::string, std::string> node_dxip_map_;
    std::mutex node_dxip_map_mutex_;
    std::map<std::string, std::string> dxip_node_map_;
    std::mutex dxip_node_map_mutex_;
    bool is_root_{false};
    uint64_t hash64_{0};
    // kadmlia::NatManagerIntf* nat_manager_{kadmlia::NatManagerIntf::Instance()};

    DISALLOW_COPY_AND_ASSIGN(LocalNodeInfo);
};

typedef std::shared_ptr<LocalNodeInfo> LocalNodeInfoPtr;

}  // namespace kadmlia

}  // namespace top
