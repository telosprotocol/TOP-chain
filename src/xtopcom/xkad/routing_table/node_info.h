// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <memory>
#include <chrono>

#include "xkad/routing_table/routing_utils.h"
#include "xtransport/transport_fwd.h"

namespace top {
namespace kadmlia {

const int kInvalidBucketIndex = -1;
const int kSelfBucketIndex = 0;

struct NodeInfo {
public:
    NodeInfo();
    NodeInfo(const NodeInfo& other);
    NodeInfo(const std::string& id);
    ~NodeInfo();
    NodeInfo& operator=(const NodeInfo& other);
    bool operator < (const NodeInfo& other) const;
    bool IsPublicNode() const;
    std::string string();

public:
    std::string node_id;
    int bucket_index{ kInvalidBucketIndex };
    std::string public_ip;
    uint16_t public_port{ 0 };
    std::string local_ip;
    uint16_t local_port{ 0 };
    int32_t connection_id{ 0 };
    int32_t detection_count{ 0 };
    int32_t detection_delay_count{ 0 };
    base::ServiceType service_type{};
    std::string xid;
    uint64_t hash64{ 0 };
	transport::UdpPropertyPtr udp_property;
};

typedef std::shared_ptr<NodeInfo> NodeInfoPtr;

}  // namespace kadmlia
}  // namespace top
