// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xpacket.h"
#include "xgossip/include/header_block_data.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_timer.h"
#include "xtransport/proto/transport.pb.h"

#include <mutex>
#include <queue>
#include <set>

namespace top {

namespace kadmlia {

class RootRoutingTable;
typedef std::shared_ptr<RootRoutingTable> RootRoutingTablePtr;

}  // namespace kadmlia

namespace gossip {

struct SyncBlockItem {
    base::ServiceType routing_service_type;
    std::string header_hash;
    std::chrono::steady_clock::time_point time_point;
};

typedef struct SyncAskFilter {
    std::string node_id;
    uint32_t ask_count;
} SyncAskFilter;
typedef std::shared_ptr<SyncAskFilter> SyncAskFilterPtr;

class BlockSyncManager {
public:
    static BlockSyncManager * Instance();
    void SetRoutingTablePtr(kadmlia::RootRoutingTablePtr & routing_table);
    void NewBroadcastMessage(transport::protobuf::RoutingMessage & message);

private:
    BlockSyncManager();
    ~BlockSyncManager();

    bool HeaderHashExists(const std::string & header_hash);
    uint32_t GetBlockMsgType(const std::string & header_hash);
    void AddHeaderHashToQueue(const std::string & header_hash, base::ServiceType service_type);
    void CheckHeaderHashQueue();
    base::ServiceType GetRoutingServiceType(const std::string & des_node_id);
    void SendSyncAsk(std::shared_ptr<SyncBlockItem> & sync_item);
    void HandleSyncAsk(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void HandleSyncAck(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void HandleSyncRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void HandleSyncResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    bool HeaderRequested(const std::string & header_hash);
    void RemoveHeaderBlock(const std::string & header_hash);
    bool DataExists(const std::string & header_hash);
    bool CheckSyncFilterMap(const std::string & header_hash, const std::string & node_id);
    void RemoveSyncFilterMap(const std::string & header_hash);

    std::map<std::string, std::shared_ptr<SyncBlockItem>> block_map_;
    std::mutex block_map_mutex_;
    base::TimerRepeated timer_{base::TimerManager::Instance(), "BlockSyncManager"};
    std::map<std::string, std::chrono::steady_clock::time_point> requested_headers_;
    std::mutex requested_headers_mutex_;
    std::shared_ptr<HeaderBlockData> header_block_data_{nullptr};
    kadmlia::RootRoutingTablePtr routing_table_;

    std::mutex sync_ask_filter_map_mutex_;
    std::unordered_map<std::string, std::vector<SyncAskFilterPtr>> sync_ask_filter_map_;

    DISALLOW_COPY_AND_ASSIGN(BlockSyncManager);
};

}  // namespace gossip

}  // namespace top
