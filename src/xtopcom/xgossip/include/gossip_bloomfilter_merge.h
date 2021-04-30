// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#pragma once
#include <map>
#include <chrono>
#include <mutex>
#include "xtransport/transport.h"
#include "xgossip/gossip_interface.h"
#include "xpbase/base/top_timer.h"

namespace top {

namespace gossip {

// notice: no dtor!!
class GossipBloomfilterMerge : public GossipInterface {
public:
    explicit GossipBloomfilterMerge(transport::TransportPtr transport_ptr);
    virtual ~GossipBloomfilterMerge();
    virtual void Broadcast(
            uint64_t local_hash64,
            transport::protobuf::RoutingMessage& message,
            std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors);

    // just for performance test
    virtual void BroadcastWithNoFilter(
            const std::string& local_id,
            transport::protobuf::RoutingMessage& message,
            const std::vector<kadmlia::NodeInfoPtr>& neighbors);

    void Start();

private:
    void CleanTimerProc();

private:
    std::mutex m_mutex;
    struct GossipInfo {
        std::chrono::steady_clock::time_point timepoint;
        std::string bloomfilter;
    };
	std::map<uint32_t, GossipInfo> m_gossip_map;  // record gossip msg_id
    base::TimerRepeated m_clean_timer{base::TimerManager::Instance(), "GossipBloomfilterMerge_cleaner"};
    DISALLOW_COPY_AND_ASSIGN(GossipBloomfilterMerge);
};

}  // namespace gossip

}  // namespace top
#endif