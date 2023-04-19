// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

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

    bool Init(const std::string & local_ip, uint16_t local_port, base::KadmliaKeyPtr kadmlia_key);
    void Reset();
    std::string kad_key();
    std::string root_kad_key();

    std::string local_ip() {
        return local_ip_;
    }
    uint16_t local_port() {
        return local_port_;
    }
    std::string public_ip() {
        std::unique_lock<std::mutex> lock(public_mutex_);
        return public_ip_;
    }

    uint16_t public_port() {
        std::unique_lock<std::mutex> lock(public_mutex_);
        return public_port_;
    }

    void set_public_ip(const std::string & ip);
    void set_public_port(uint16_t port);
    base::ServiceType service_type() {
        return service_type_;
    }
    void set_service_type(base::ServiceType service_type) {
        service_type_ = service_type;
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

    uint64_t hash64() {
        return hash64_;
    }

private:
    std::string local_ip_;
    uint16_t local_port_{0};
    uint16_t rpc_http_port_{0};
    uint16_t rpc_ws_port_{0};
    std::mutex public_mutex_;
    std::string public_ip_;
    uint16_t public_port_{0};
    // int32_t nat_type_{kNatTypeUnknown};
    base::ServiceType service_type_{};

    std::mutex kadkey_mutex_;
    base::KadmliaKeyPtr kadmlia_key_{nullptr};

    uint64_t hash64_{0};

    DISALLOW_COPY_AND_ASSIGN(LocalNodeInfo);
};

typedef std::shared_ptr<LocalNodeInfo> LocalNodeInfoPtr;

}  // namespace kadmlia

}  // namespace top
