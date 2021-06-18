// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/block_sync_manager.h"

#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xutility/xhash.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/xwrouter.h"

#include <cinttypes>

namespace top {

namespace gossip {

static const uint32_t kMaxBlockQueueSize = 1024u;
static const uint32_t kCheckHeaderHashPeriod = 500 * 1000;  // 1s
static const uint32_t kSyncAskNeighborCount = 3u;           // random ask 3 neighbors who has block
static const uint32_t kHeaderSavePeriod = 30 * 1000;        // keep 30s
static const uint32_t kHeaderRequestedPeriod = 2 * 1000;    // request 3s
static const uint32_t kSyncAskMaxCount = 2;                 // ask 2 times max

BlockSyncManager::BlockSyncManager() {
    wrouter::WrouterRegisterMessageHandler(kGossipBlockSyncAsk, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
        TOP_DEBUG("HandleMessage kGossipBlockSyncAsk");
        HandleSyncAsk(message, packet);
    });
    wrouter::WrouterRegisterMessageHandler(kGossipBlockSyncAck, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
        TOP_DEBUG("HandleMessage kGossipBlockSyncAck");
        HandleSyncAck(message, packet);
    });
    wrouter::WrouterRegisterMessageHandler(kGossipBlockSyncRequest, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
        TOP_DEBUG("HandleMessage kGossipBlockSyncRequest");
        HandleSyncRequest(message, packet);
    });
    wrouter::WrouterRegisterMessageHandler(kGossipBlockSyncResponse, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
        TOP_DEBUG("HandleMessage kGossipBlockSyncResponse");
        HandleSyncResponse(message, packet);
    });

    header_block_data_ = std::make_shared<HeaderBlockDataCache>();

    timer_.Start(kCheckHeaderHashPeriod, kCheckHeaderHashPeriod, std::bind(&BlockSyncManager::CheckHeaderHashQueue, this));
}

BlockSyncManager::~BlockSyncManager() {
}

BlockSyncManager * BlockSyncManager::Instance() {
    static BlockSyncManager ins;
    return &ins;
}

void BlockSyncManager::NewBroadcastMessage(transport::protobuf::RoutingMessage & message) {
    if (!message.gossip().has_header_hash() || message.gossip().header_hash().empty()) {
        xinfo("[BlockSyncManager] NewBroadcastMessage return 1:%s", message.gossip().header_hash().c_str());
        return;
    }

    if (DataExists(message.gossip().header_hash())) {
        xinfo("[BlockSyncManager] NewBroadcastMessage return 2 data already exists:%s", message.gossip().header_hash().c_str());
        return;
    }

    if (message.gossip().has_block() && !message.gossip().block().empty()) {
        xinfo("[BlockSyncManager] NewBroadcastMessage return 3 data add now:%s", message.gossip().header_hash().c_str());
        header_block_data_->AddData(message.gossip().header_hash(), message.SerializeAsString());
        return;
    }

    if (HeaderHashExists(message.gossip().header_hash())) {
        xinfo("[BlockSyncManager] NewBroadcastMessage return 4 data head hash already add:%s", message.gossip().header_hash().c_str());
        return;
    }
    base::ServiceType des_service_type;
    // todo charles since this module only servers for root broadcast .make this service_type defaultly.
    assert(message.has_is_root() && message.is_root());
    // if (message.has_is_root() && message.is_root()) {
    des_service_type = base::ServiceType(kRoot);
    // } else {
    //     des_service_type = GetRoutingServiceType(message.des_node_id());
    // }

    AddHeaderHashToQueue(message.gossip().header_hash(), des_service_type);
}

bool BlockSyncManager::DataExists(const std::string & header_hash) {
    return header_block_data_->HasData(header_hash);
}

bool BlockSyncManager::HeaderHashExists(const std::string & header_hash) {
    std::unique_lock<std::mutex> lock(block_map_mutex_);
    auto iter = block_map_.find(header_hash);
    if (iter != block_map_.end()) {
        return true;
    }
    return false;
}

void BlockSyncManager::AddHeaderHashToQueue(const std::string & header_hash, base::ServiceType service_type) {
    xinfo("[BlockSyncManager] AddHeaderHashToQueue header block hash:%s", header_hash.c_str());
    std::unique_lock<std::mutex> lock(block_map_mutex_);
    block_map_.insert(std::make_pair(
        header_hash, std::make_shared<SyncBlockItem>(SyncBlockItem{service_type, header_hash, std::chrono::steady_clock::now() + std::chrono::milliseconds(kHeaderSavePeriod)})));
}

void BlockSyncManager::SetRoutingTablePtr(kadmlia::RootRoutingTablePtr & routing_table) {
    routing_table_ = routing_table;
}

void BlockSyncManager::SendSyncAsk(std::shared_ptr<SyncBlockItem> & sync_item) {
    TOP_DEBUG("SendSyncAsk: %llu, header_hash:%s", sync_item->routing_service_type.value(), HexEncode(sync_item->header_hash).c_str());
    // auto routing = wrouter::GetRoutingTable(sync_item->routing_service_type);
    auto routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!routing) {
        TOP_WARN("no routing table:%llu", sync_item->routing_service_type.value());
        return;
    }
    assert(routing);
    transport::protobuf::RoutingMessage pbft_message;
    routing->SetFreqMessage(pbft_message);
    pbft_message.set_type(kGossipBlockSyncAsk);
    pbft_message.set_id(kadmlia::CallbackManager::MessageId());
    pbft_message.set_data(sync_item->header_hash);
    pbft_message.set_src_service_type(sync_item->routing_service_type.value());

    std::vector<kadmlia::NodeInfoPtr> select_nodes;
    if (!(routing->GetRandomNodes(select_nodes, kSyncAskNeighborCount))) {
        TOP_WARN("SendSyncAsk failed, select empty nodes");
        return;
    }

    std::set<kadmlia::NodeInfoPtr> asked_nodes;
    for (auto & node_ptr : select_nodes) {
        if (!node_ptr) {
            continue;
        }
        if (!CheckSyncFilterMap(sync_item->header_hash, node_ptr->node_id)) {
            continue;
        }

        pbft_message.set_des_node_id(node_ptr->node_id);
        // routing->SendData(pbft_message, node_ptr->public_ip, node_ptr->public_port);
        routing->SendData(pbft_message, node_ptr);
        TOP_DEBUG("send sync ask: %s,%d", node_ptr->public_ip.c_str(), node_ptr->public_port);
    }

    TOP_DEBUG("[gossip_sync]send out ask,%d[%s].", kGossipBlockSyncAsk, HexEncode(pbft_message.data()).c_str());
}

bool BlockSyncManager::CheckSyncFilterMap(const std::string & header_hash, const std::string & node_id) {
    {
        std::unique_lock<std::mutex> lock(sync_ask_filter_map_mutex_);
        auto ifind = sync_ask_filter_map_.find(header_hash);
        if (ifind == sync_ask_filter_map_.end()) {
            std::vector<SyncAskFilterPtr> vec;
            SyncAskFilterPtr sptr = std::make_shared<SyncAskFilter>();
            sptr->node_id = node_id;
            sptr->ask_count = 1;
            vec.push_back(sptr);
            sync_ask_filter_map_[header_hash] = vec;
            TOP_DEBUG("sync_ask_filter_map first insert header_hash:%s node:%s", HexEncode(header_hash).c_str(), HexEncode(node_id).c_str());
            return true;
        }

        // find header_hash
        for (auto & item : ifind->second) {
            if (item->node_id == node_id) {
                if (item->ask_count >= kSyncAskMaxCount) {
                    TOP_DEBUG("sync_ask_filter_map beyond askmax header_hash:%s node:%s", HexEncode(header_hash).c_str(), HexEncode(node_id).c_str());
                    return false;
                }
                item->ask_count += 1;

                TOP_DEBUG("sync_ask_filter_map update header_hash:%s node:%s", HexEncode(header_hash).c_str(), HexEncode(node_id).c_str());
                return true;
            }
        }
        SyncAskFilterPtr sptr = std::make_shared<SyncAskFilter>();
        sptr->node_id = node_id;
        sptr->ask_count = 1;
        (ifind->second).push_back(sptr);

        TOP_DEBUG("sync_ask_filter_map update push header_hash:%s node:%s", HexEncode(header_hash).c_str(), HexEncode(node_id).c_str());
        return true;
    }
}

void BlockSyncManager::RemoveSyncFilterMap(const std::string & header_hash) {
    {
        std::unique_lock<std::mutex> lock(sync_ask_filter_map_mutex_);
        auto ifind = sync_ask_filter_map_.find(header_hash);
        if (ifind == sync_ask_filter_map_.end()) {
            return;
        }
        TOP_DEBUG("remove sync_filter_map header_hash:%s", HexEncode(header_hash).c_str());
        sync_ask_filter_map_.erase(ifind);
        return;
    }
}

void BlockSyncManager::CheckHeaderHashQueue() {
    std::map<std::string, std::shared_ptr<SyncBlockItem>> block_map;
    {
        std::unique_lock<std::mutex> lock(block_map_mutex_);
        if (block_map_.empty())
            return;
        block_map = block_map_;
    }

    auto tp_now = std::chrono::steady_clock::now();
    {
        for (auto iter = block_map.begin(); iter != block_map.end(); ++iter) {
            auto item = iter->second;
            if (item->time_point <= tp_now || DataExists(item->header_hash)) {
                xinfo("[BlockSyncManager] remove header block hash:%s", item->header_hash.c_str());
                RemoveHeaderBlock(item->header_hash);
                continue;
            }

            /*
            if ((item->time_point - std::chrono::milliseconds(kHeaderSavePeriod) +
                    std::chrono::milliseconds(kHeaderRequestedPeriod)) > tp_now) {
                continue;
            }
            */

            if (HeaderHashExists(item->header_hash) && !HeaderRequested(item->header_hash)) {
                SendSyncAsk(item);
                continue;
            }
        }
    }
}

base::ServiceType BlockSyncManager::GetRoutingServiceType(const std::string & des_node_id) {
    auto kad_key = base::GetRootKadmliaKey(des_node_id);
    return kad_key->GetServiceType();
}

void BlockSyncManager::HandleSyncAsk(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    TOP_DEBUG("enter HandleSyncAsk:%s", HexEncode(message.data()).c_str());
    if (!message.has_data()) {
        return;
    }

    if (message.type() != kGossipBlockSyncAsk) {
        return;
    }

    std::string message_string;
    header_block_data_->GetData(message.data(), message_string);
    if (message_string.empty()) {
        return;
    }

    // auto routing = wrouter::GetRoutingTable(base::ServiceType(message.src_service_type()));
    auto routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!routing) {
        TOP_WARN("no routing table:%llu", message.src_service_type());
        return;
    }
    assert(routing);
    transport::protobuf::RoutingMessage pbft_message;
    routing->SetFreqMessage(pbft_message);
    pbft_message.set_type(kGossipBlockSyncAck);
    pbft_message.set_id(message.id());
    pbft_message.set_data(message.data());
    pbft_message.set_des_node_id(message.src_node_id());
    pbft_message.set_src_service_type(message.src_service_type());

    routing->SendData(pbft_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
    TOP_DEBUG("[gossip_sync]handled ask[%s].", HexEncode(message.data()).c_str());
}

void BlockSyncManager::HandleSyncAck(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!message.has_data()) {
        return;
    }

    if (message.type() != kGossipBlockSyncAck) {
        return;
    }

    /*
    if (HeaderRequested(message.data())) {
        return;
    }
    */

    if (!HeaderHashExists(message.data()) || DataExists(message.data())) {
        return;
    }

    // auto routing = wrouter::GetRoutingTable(base::ServiceType(message.src_service_type()));
    auto routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!routing) {
        TOP_WARN("no routing table:%d", message.src_service_type());
        return;
    }
    assert(routing);
    transport::protobuf::RoutingMessage pbft_message;
    routing->SetFreqMessage(pbft_message);
    pbft_message.set_type(kGossipBlockSyncRequest);
    pbft_message.set_id(kadmlia::CallbackManager::MessageId());
    pbft_message.set_des_node_id(message.src_node_id());
    pbft_message.set_data(message.data());
    pbft_message.set_src_service_type(message.src_service_type());

    routing->SendData(pbft_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
    TOP_DEBUG("[gossip_sync]handled ack[%s].", HexEncode(message.data()).c_str());
}

void BlockSyncManager::HandleSyncRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!message.has_data()) {
        return;
    }

    if (message.type() != kGossipBlockSyncRequest) {
        return;
    }

    std::string message_string;
    header_block_data_->GetData(message.data(), message_string);
    if (message_string.empty()) {
        return;
    }

    // auto routing = wrouter::GetRoutingTable(base::ServiceType(message.src_service_type()));
    auto routing = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!routing) {
        TOP_WARN("no routing table:%d", message.src_service_type());
        return;
    }
    assert(routing);
    transport::protobuf::RoutingMessage pbft_message;
    routing->SetFreqMessage(pbft_message);
    pbft_message.set_type(kGossipBlockSyncResponse);
    pbft_message.set_id(message.id());
    pbft_message.set_des_node_id(message.src_node_id());
    transport::protobuf::GossipSyncBlockData gossip_data;
    gossip_data.set_header_hash(message.data());
    gossip_data.set_block(message_string);  // get the whole message stored
    pbft_message.set_data(gossip_data.SerializeAsString());
    pbft_message.set_src_service_type(message.src_service_type());

    routing->SendData(pbft_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
    TOP_DEBUG("[gossip_sync]handled request[%s].", HexEncode(message.data()).c_str());
}

void BlockSyncManager::HandleSyncResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!message.has_data()) {
        return;
    }

    if (message.type() != kGossipBlockSyncResponse) {
        return;
    }

    transport::protobuf::GossipSyncBlockData gossip_data;
    if (!gossip_data.ParseFromString(message.data())) {
        return;
    }
    std::string header_hash = gossip_data.header_hash();

    if (!HeaderHashExists(header_hash)) {
        return;
    }
    if (header_block_data_->HasData(header_hash)) {
        return;
    }

    transport::protobuf::RoutingMessage sync_message;
    if (!sync_message.ParseFromString(gossip_data.block())) {
        TOP_WARN("SyncMessae ParseFromString failed");
        return;
    }
    header_block_data_->AddData(header_hash, gossip_data.block());

    // call callback
    if (sync_message.type() == kElectVhostRumorP2PMessage || sync_message.type() == kElectVhostRumorGossipMessage) {
        std::string vhost_data = sync_message.gossip().block();
        uint32_t vhash = base::xhash32_t::digest(vhost_data);
        if (header_hash != std::to_string(vhash)) {
            TOP_WARN("[gossip_sync] header hash(%s) not equal", HexEncode(header_hash).c_str());
            return;
        }
        // RRS_pull_flag. only for local metrics statistics
        sync_message.set_ack_id(181819);
        base::xpacket_t packet;
        wrouter::Wrouter::Instance()->HandleOwnSyncPacket(sync_message, packet);
        TOP_DEBUG("blockmessage callback hash:%u,header_hash:%s,type:%d", vhash, HexEncode(header_hash).c_str(), sync_message.type());
    } else if (sync_message.type() == kTestMessageType) {
        std::string vhost_data = sync_message.gossip().block();
        uint32_t vhash = base::xhash32_t::digest(vhost_data);
        if (header_hash != std::to_string(vhash)) {
            TOP_WARN("[gossip_sync] header hash(%s) not equal", HexEncode(header_hash).c_str());
            return;
        }
        // just for test, mark this message as sync-message
        sync_message.set_ack_id(99);
        base::xpacket_t packet;
        wrouter::Wrouter::Instance()->HandleOwnSyncPacket(sync_message, packet);
    }
    RemoveHeaderBlock(header_hash);
    xinfo("blockmessage callback msg_hash:%u,header_hash:%s,type:%d", sync_message.msg_hash(), header_hash.c_str(), sync_message.type());
}

void BlockSyncManager::RemoveHeaderBlock(const std::string & header_hash) {
    {
        std::unique_lock<std::mutex> lock(requested_headers_mutex_);
        auto iter = requested_headers_.find(header_hash);
        if (iter != requested_headers_.end()) {
            requested_headers_.erase(iter);
        }
    }

    {
        std::unique_lock<std::mutex> lock(block_map_mutex_);
        auto iter = block_map_.find(header_hash);
        if (iter != block_map_.end()) {
            block_map_.erase(iter);
        }
    }
    // (delete block from db)
    header_block_data_->RemoveData(header_hash);
    // delete sync filter map
    RemoveSyncFilterMap(header_hash);
}

bool BlockSyncManager::HeaderRequested(const std::string & header_hash) {
    auto tp_now = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lock(requested_headers_mutex_);
    auto iter = requested_headers_.find(header_hash);
    if (iter != requested_headers_.end()) {
        if (iter->second <= tp_now) {
            requested_headers_.erase(iter);
            return false;
        }
        // in kHeaderRequestPeriod seconds, have requested
        return true;
    }

    requested_headers_.insert(std::make_pair(header_hash, std::chrono::steady_clock::now() + std::chrono::milliseconds(kHeaderRequestedPeriod)));
    return false;
}

}  // namespace gossip

}  // namespace top
