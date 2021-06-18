// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpbase/base/error_code.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/top_utils.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace top {

namespace kadmlia {

class LocalNodeInfo;
typedef std::shared_ptr<LocalNodeInfo> LocalNodeInfoPtr;

enum HandshakeType {
    kHandshakeRequest = 1,
    kHandshakeResponse = 2,
};

static const int kKadParamK = 8;
static const int kKadParamAlpha = 2;
static const int kKadParamAlphaRandom = 1;  // 0 if no random node
static const int kRoutingMaxNodesSize = kNodeIdSize * 8 * kKadParamK;
static const int kDetectionTimes = 4;
static const int kHopToLive = 20;
static const int kJoinRetryTimes = 5;
// static const uint32_t kFindNodesBloomfilterBitSize = 4096;
static const uint32_t kFindNodesBloomfilterBitSize = 2048;  // for 256 nodes, error_rate = 4%
static const uint32_t kFindNodesBloomfilterHashNum = 11;


void GetPublicEndpointsConfig(const top::base::Config & config, std::set<std::pair<std::string, uint16_t>> & boot_endpoints);

bool CreateGlobalXid(const base::Config & config);
LocalNodeInfoPtr CreateLocalInfoFromConfig(const base::Config & config, base::KadmliaKeyPtr kad_key);

}  // namespace kadmlia

}  // namespace top
