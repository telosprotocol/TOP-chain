// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <chrono>
#include <vector>

#include "xcommon/xip.h"
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

namespace top {

namespace wrouter {

typedef struct WrouterTableNodes{
    common::xip2_t m_xip2;
    std::string node_id;
} WrouterTableNodes;

typedef struct NetNode {
    std::string m_account;
    std::string m_public_key;
    base::XipParser m_xip;
    std::string m_node_id;    // service node id
    uint8_t m_associated_gid; // for shard, associtaed with cluster
    uint64_t m_version;
} NetNode;

typedef struct NetNodeCmp {
    bool operator()(const NetNode& lh, const NetNode& rh) const {
        return lh.m_account < rh.m_account;
    }
} NetNodeCmp;

typedef struct MarkExpiredNetNode {
    std::string node_id;
    std::chrono::steady_clock::time_point time_point;
} MarkExpiredNetNode;

}

}
