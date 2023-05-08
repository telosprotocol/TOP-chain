// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/root_routing_table.h"

#include "xbase/xpacket.h"
#include "xbase/xutl.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <bitset>
#include <chrono>
#include <fstream>
#include <future>
#include <limits>
#include <map>
#include <sstream>
#include <unordered_map>
//#include "xkad/top_main/top_commands.h"
// #include "xpbase/base/top_log_name.h"
#include "xtransport/udp_transport/transport_util.h"

namespace top {

namespace kadmlia {

// for now do nothing about hearbeat in kad-routing-table(do hearbeat in transport),
// so just change kHeartbeatPeriod more bigger
static const int32_t kHeartbeatPeriod = 30 * 1000 * 1000;             // 2s
static const int32_t kHeartbeatCheckProcPeriod = 1 * 1000 * 1000;     // 2s
static const int32_t kRejoinPeriod = 10 * 1000 * 1000;                // 10s
static const int32_t kFindNeighboursPeriod = 3 * 1000 * 1000;         // 3s
static const int32_t kDumpRoutingTablePeriod = 1 * 60 * 1000 * 1000;  // 5min

RootRoutingTable::RootRoutingTable(std::shared_ptr<transport::Transport> transport_ptr, std::shared_ptr<LocalNodeInfo> local_node_ptr)
  : transport_ptr_{transport_ptr}
  , local_node_ptr_{local_node_ptr}
  , RoutingMaxNodesSize_(kRoutingMaxNodesSize)
  , nodes_()
  , nodes_mutex_()
  , node_id_map_()
  , node_id_map_mutex_()
  , node_hash_map_(std::make_shared<std::map<uint64_t, NodeInfoPtr>>())
  , node_hash_map_mutex_()
  , bootstrap_mutex_()
  , bootstrap_cond_()
  , joined_(false)
  , bootstrap_id_()
  , bootstrap_ip_()
  , bootstrap_port_(0)
  , bootstrap_nodes_mutex_()
  , node_detection_ptr_(nullptr)
  , destroy_(false) {
}

// RootRoutingTable::~RootRoutingTable() {
// }

bool RootRoutingTable::Init() {
    {
        std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
        NodeInfoPtr node_ptr;
        node_ptr.reset(new NodeInfo(get_local_node_info()->kad_key()));
        node_ptr->public_ip = get_local_node_info()->public_ip();
        node_ptr->public_port = get_local_node_info()->public_port();
        node_ptr->xid = get_local_node_info()->kad_key();
        node_ptr->hash64 = get_local_node_info()->hash64();
        TOP_DEBUG(
            "add node_hash_map node_id:%s hash64:%llu public_endpoint:%s:%u", (node_ptr->node_id).c_str(), node_ptr->hash64, node_ptr->public_ip.c_str(), node_ptr->public_port);
        node_hash_map_->insert(std::make_pair(node_ptr->hash64, node_ptr));
    }
    {
        std::unique_lock<std::mutex> lock(use_nodes_mutex_);
        no_lock_for_use_nodes_.reset();
        no_lock_for_use_nodes_ = std::make_shared<std::vector<NodeInfoPtr>>(nodes());
    }

    node_detection_ptr_.reset(new NodeDetectionManager(timer_manager_, *this));

    // attention: hearbeat timer does not do hearbeating really(using xudp do)
    timer_heartbeat_ = std::make_shared<base::TimerRepeated>(timer_manager_, "RootRoutingTable::HeartbeatProc");
    timer_heartbeat_->Start(kHeartbeatPeriod, kHeartbeatPeriod, std::bind(&RootRoutingTable::HeartbeatProc, shared_from_this()));

    using namespace std::placeholders;
    HeartbeatManagerIntf::Instance()->Register(std::to_string((long)this), std::bind(&RootRoutingTable::OnHeartbeatFailed, shared_from_this(), _1, _2));

    SetFindNeighbourIntervalMin();
    timer_find_neighbours_ = std::make_shared<base::TimerRepeated>(timer_manager_, "RootRoutingTable::FindNeighbours");
    timer_find_neighbours_->Start(kFindNeighboursPeriod, kFindNeighboursPeriod, std::bind(&RootRoutingTable::FindNeighbours, shared_from_this()));

    return true;
}

void RootRoutingTable::PrintRoutingTable() {
#if 0
    if (destroy_) {
        return;
    }
    static std::atomic<uint32_t> index(0);

    uint32_t size = 0;
    {
        NodesLock lock(nodes_mutex_);
        uint32_t tmp_index = ++index;
        std::string file_name = "/tmp/all_node_id_" + get_local_node_info()->kad_key() + "_" + std::to_string(tmp_index);
        FILE * fd = fopen(file_name.c_str(), "w");
        if (fd == NULL) {
            return;
        }

        SortNodesByTargetXid(get_local_node_info()->kad_key(), nodes_.size());
        size = nodes_.size();
        fprintf(fd, "size: %d\n", size);
        fprintf(fd, "local: %s\t%s:%d\n", get_local_node_info()->kad_key().c_str(), get_local_node_info()->public_ip().c_str(), get_local_node_info()->public_port());
        std::map<uint32_t, unsigned int> bucket_rank_map;
        for (auto & node_ptr : nodes_) {
            bucket_rank_map[node_ptr->bucket_index] += 1;
            fprintf(fd, "node: %s\t%s:%d\n", node_ptr->node_id.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
        }

        fprintf(fd, "\n");
        for (auto & item : bucket_rank_map) {
            fprintf(fd, "bucket: %d:%d\n", item.first, item.second);
        }
        fclose(fd);
    }

    xdbg("RootRoutingTable dump for xnetwork_id(%d) service_type(%s), dump_size(%d)",
         get_local_node_info()->kadmlia_key()->xnetwork_id(),
         get_local_node_info()->kadmlia_key()->GetServiceType().info().c_str(),
         size);
#endif
}

bool RootRoutingTable::UnInit() {
    destroy_ = true;

    if (node_detection_ptr_) {
        node_detection_ptr_->Join();
    }

    timer_rejoin_ = nullptr;
    timer_find_neighbours_ = nullptr;
    timer_heartbeat_ = nullptr;
    timer_heartbeat_check_ = nullptr;
    timer_prt_ = nullptr;

    return true;
}

int RootRoutingTable::SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string data;
    if (!message.SerializeToString(&data)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    transport::UdpPropertyPtr udp_property;
    return transport_ptr_->SendDataWithProp(data, peer_ip, peer_port, udp_property, message.priority());
}

int RootRoutingTable::SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    return get_transport()->SendPing(data, peer_ip, peer_port);
}

int RootRoutingTable::SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node) {
    std::string data;
    if (!message.SerializeToString(&data)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    return transport_ptr_->SendDataWithProp(data, node->public_ip, node->public_port, node->udp_property, message.priority());
}

int RootRoutingTable::MultiJoin(const std::set<std::pair<std::string, uint16_t>> & boot_endpoints) {
    std::cout << "join p2p network..." << std::endl;
    xinfo("MultiJoin(%d) ...", (int)boot_endpoints.size());  // NOLINT
    // std::string boot_nodes;
    // for (const auto& kv : boot_endpoints) boot_nodes += kv.first + ":" + std::to_string(kv.second) + ", ";
    // xinfo("MultiJoin(%s) ...", boot_nodes.c_str());  // NOLINT

    if (joined_) {
        return kKadFailed;
    }

    for (auto & kv : boot_endpoints) {
        std::string endpoint_ip_port = kv.first + ":" + std::to_string(kv.second);
        m_endpoint_ip_port.insert(endpoint_ip_port);
    }

    int retried_times = 0;
    uint32_t wait_time = 4;
    // while (retried_times < kJoinRetryTimes) {
    while (1) {
        for (auto & kv : boot_endpoints) {
            const auto peer_ip = kv.first;
            const auto peer_port = kv.second;
            Bootstrap(peer_ip, peer_port, get_local_node_info()->service_type());
            xinfo("  -> Bootstrap(%s:%d) ...", peer_ip.c_str(), peer_port);
        }

        std::unique_lock<std::mutex> lock(bootstrap_mutex_);
        if (bootstrap_cond_.wait_for(lock, std::chrono::seconds(wait_time), [this]() -> bool { return this->joined_; })) {
            if (joined_) {
                xinfo("  node join(%s:%d) success", bootstrap_ip_.c_str(), (int)bootstrap_port_);  // NOLINT
                std::cout << "join p2p network ok" << std::endl;

                return kKadSuccess;
            }
        }

        retried_times += 1;
        if (retried_times > kJoinRetryTimes) {
            wait_time *= 2;
            if (wait_time > 128) {
                wait_time = 128;
            }
        }

        xkinfo("%s [%llu] has nodes_ size: %d, set_size: %d, ip: %s, port: %d",
               get_local_node_info()->kad_key().c_str(),
               get_local_node_info()->service_type().value(),
               node_id_map_.size(),
               nodes_size(),
               get_local_node_info()->public_ip().c_str(),
               get_local_node_info()->public_port());
    }

    xwarn("node join failed after retried: %d times!", retried_times);
    return kKadFailed;
}

bool RootRoutingTable::IsJoined() {
    return joined_;
}

bool RootRoutingTable::SetJoin(const std::string & boot_id, const std::string & boot_ip, int boot_port) {
    std::unique_lock<std::mutex> lock(joined_mutex_);
    if (joined_) {
        xinfo("SetJoin(%s:%d-%s) ignore", boot_ip.c_str(), boot_port, boot_id.c_str());
        return false;
    }

    joined_ = true;
    set_bootstrap_id(boot_id);
    set_bootstrap_ip(boot_ip);
    set_bootstrap_port(boot_port);    
    xinfo("SetJoin(%s:%d-%s) success", boot_ip.c_str(), boot_port, boot_id.c_str());
    return true;
}

void RootRoutingTable::WakeBootstrap() {
    std::lock_guard<std::mutex> lock(bootstrap_mutex_);
    bootstrap_cond_.notify_all();
}

void RootRoutingTable::SendToClosestNode(transport::protobuf::RoutingMessage & message, bool add_hop) {
    if (message.des_node_id() == get_local_node_info()->kad_key()) {
        return;
    }

    if (add_hop) {
        transport::protobuf::HopInfo * hop_info = message.add_hop_nodes();
        hop_info->set_node_id(get_local_node_info()->kad_key());
    }
    RecursiveSend(message, 0);
}

void RootRoutingTable::SendToClosestNode(transport::protobuf::RoutingMessage & message) {
    SendToClosestNode(message, true);
}

void RootRoutingTable::SendToNClosestNode(transport::protobuf::RoutingMessage & message, int n) {
    if (message.des_node_id() == get_local_node_info()->kad_key()) {
        return;
    }

    if (!joined_) {
        return;
    }

    {
        std::vector<NodeInfoPtr> ready_nodes;
        std::vector<NodeInfoPtr> next_nodes_vec = GetClosestNodes(message.des_node_id(), RoutingMaxNodesSize_);
        for (auto & nptr : next_nodes_vec) {
            if (nptr->node_id == get_local_node_info()->kad_key()) {
                continue;
            }
            if (nptr->public_ip == get_local_node_info()->public_ip() && nptr->public_port == get_local_node_info()->public_port()) {
                continue;
            }
            ready_nodes.push_back(nptr);
        }

        if (ready_nodes.empty()) {
            xwarn("SendToNClosestNode get empty nodes, send failed");
            return;
        }

        int n2 = 0;
        for (size_t i = 0; i < ready_nodes.size() && n2 < n; ++i) {
            NodeInfoPtr node = ready_nodes[i];
            if (!node) {
                continue;
            }
            SendData(message, node);
            n2 += 1;
        }
    }
}

void RootRoutingTable::RecursiveSend(transport::protobuf::RoutingMessage & message, int retry_times) {
    std::set<std::string> exclude;
    for (int i = 0; i < message.hop_nodes_size(); ++i) {
        auto iter = exclude.find(message.hop_nodes(i).node_id());
        if (iter != exclude.end()) {
            return;
        }
        exclude.insert(message.hop_nodes(i).node_id());
    }

    std::vector<NodeInfoPtr> ready_nodes;
    std::vector<NodeInfoPtr> next_nodes_vec = GetClosestNodes(message.des_node_id(), RoutingMaxNodesSize_);
    for (auto & nptr : next_nodes_vec) {
        if (nptr->node_id == get_local_node_info()->kad_key()) {
            continue;
        }
        if (nptr->public_ip == get_local_node_info()->public_ip() && nptr->public_port == get_local_node_info()->public_port()) {
            continue;
        }
        auto iter = exclude.find(nptr->node_id);
        if (iter != exclude.end()) {
            continue;
        }
        ready_nodes.push_back(nptr);
    }
    if (ready_nodes.empty()) {
        xwarn("%s SendToClosestNode get empty nodes, send failed", transport::FormatMsgid(message).c_str());
        return;
    }

    NodeInfoPtr node = ready_nodes[0];
    if (!node) {
        return;
    }
    SendData(message, node);
}

void RootRoutingTable::HeartbeatProc() {
    if (destroy_) {
        return;
    }
    XMETRICS_PACKET_INFO("p2p_kad_info",
                         "local_nodeid",
                         get_local_node_info()->kad_key(),
                         "service_type",
                         get_local_node_info()->kadmlia_key()->GetServiceType().info().c_str(),
                         "neighbours",
                         nodes_size(),
                         "public_ip",
                         get_local_node_info()->public_ip(),
                         "public_port",
                         get_local_node_info()->public_port());
}

uint32_t RootRoutingTable::GetFindNodesMaxSize() {
    return RoutingMaxNodesSize_;
}

void RootRoutingTable::SetFindNeighbourIntervalIncr() {
    if (find_neighbour_interval_threshold_ < 16) {
        find_neighbour_interval_threshold_ *= 2;
        xdbg("incr find interval to %d", find_neighbour_interval_threshold_);
    }
}

void RootRoutingTable::SetFindNeighbourIntervalDecr() {
    if (find_neighbour_interval_threshold_ > 2) {
        find_neighbour_interval_threshold_ = 2;
    } else {  // == 1,2
        find_neighbour_interval_threshold_ = 1;
    }

    xdbg("decr find interval to %d", find_neighbour_interval_threshold_);
}

void RootRoutingTable::SetFindNeighbourIntervalMin() {
    find_neighbour_interval_threshold_ = 1;
    xdbg("min find interval to %d", find_neighbour_interval_threshold_);
}

void RootRoutingTable::SetFindNeighbourIntervalMax() {
    find_neighbour_interval_threshold_ = 16;
    xdbg("max find interval to %d", find_neighbour_interval_threshold_);
}

void RootRoutingTable::SetFindNeighbourIntervalKeepOrMid() {
    if (find_neighbour_interval_threshold_ > 4) {
        find_neighbour_interval_threshold_ = 4;
        xdbg("mid find interval to %d", find_neighbour_interval_threshold_);
    } else {
        xdbg("keep find interval to %d", find_neighbour_interval_threshold_);
    }
}

void RootRoutingTable::FindNeighbours() {
    if (destroy_ || !joined_) {
        return;
    }

    // the first start-time  make sure go-to this if after calling Join
    if (get_local_node_info() && get_transport()) {
        std::vector<NodeInfoPtr> tmp_vec;
        {
            NodesLock lock(nodes_mutex_);
            int sort_num = SortNodesByTargetXid(get_local_node_info()->kad_key(), RoutingMaxNodesSize_);
            for (int i = 0; i < sort_num; ++i) {
                tmp_vec.push_back(nodes_[i]);
            }
        }

        find_neighbour_interval_ += 1;
        xdbg(
            "FindNeighbours alive for self_service_type(%llu), now size(%d), find_neighbour_interval(%u),"
            "find_neighbour_interval_threshold(%u)",
            get_local_node_info()->kadmlia_key()->GetServiceType().value(),
            nodes_size(),
            find_neighbour_interval_,
            find_neighbour_interval_threshold_);
        if (nodes_size() == 0 && find_neighbour_interval_threshold_ > 1) {
            find_neighbour_interval_ = 0;
            nodes_delta_ = 0;
            SetFindNeighbourIntervalMin();                     // set find_neighbour_interval_threshold_ = 1 as default
            FindClosestNodes(GetFindNodesMaxSize(), tmp_vec);  // do find
            TOP_WARN("FindNeighbours abnormal case is happening, now size(%u), using default find_method", nodes_size());
            return;
        }

        if (find_neighbour_interval_ >= find_neighbour_interval_threshold_) {
            find_neighbour_interval_ = 0;

            const int nodes_delta = nodes_delta_.load();
            if (nodes_delta == 0) {  // 0
                SetFindNeighbourIntervalIncr();
            } else if (nodes_delta == 1) {
                SetFindNeighbourIntervalKeepOrMid();
            } else {  // > 1
                SetFindNeighbourIntervalDecr();
            }
            nodes_delta_ = 0;

            FindClosestNodes(GetFindNodesMaxSize(), tmp_vec);  // do find
        }
    }
}

int RootRoutingTable::AddNode(NodeInfoPtr node) {
    if (get_transport()->CheckRatelimitMap(node->public_ip + ":" + std::to_string(node->public_port)) != 0) {
        TOP_WARN("socket ratelimit, disable addnode");
        return kKadFailed;
    }
    xdbg("node_id(%s), pub(%s:%d)", HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), node->public_port);

    if (node->node_id == get_local_node_info()->kad_key()) {
        xdbg("kHandshake: get_local_node_info()->kad_key()[%s][%s][%s][%d][%s]",
             (node->node_id).c_str(),
             (get_local_node_info()->kad_key()).c_str(),
             node->public_ip.c_str(),
             node->public_port,
             (global_xid->Get()).c_str());
        return kKadFailed;
    }

    if (!ValidNode(node)) {
        xwarn("node invalid.");
        return kKadFailed;
    }

    if (HasNode(node)) {
        xdbg("kHandshake: HasNode: %s", node->node_id.c_str());
        return kKadNodeHasAdded;
    }

    if (SetNodeBucket(node) != kKadSuccess) {
        xwarn("set node bucket index failed![%s]", node->node_id.c_str());
        return kKadFailed;
    }

    {
        NodesLock lock(nodes_mutex_);
        SortNodesByTargetXid(get_local_node_info()->kad_key(), nodes_.size());
        if (!NewNodeReplaceOldNode(node, true)) {
            xinfo("newnodereplaceoldnode failed. node_id:%s, node_bucket:%d, local:%s", (node->node_id).c_str(), node->bucket_index, (get_local_node_info()->kad_key()).c_str());
            return kKadFailed;
        }

        xdbg("newnodereplaceoldnode success. node_id:%s, node_bucket:%d, local:%s", node->node_id.c_str(), node->bucket_index, (get_local_node_info()->kad_key()).c_str());

        // if (HasNode(node)) {
        //     xdbg("kHandshake: HasNode");
        //     return kKadNodeHasAdded;
        // }
        XMETRICS_PACKET_INFO(
            "p2p_kad_addnode", "local_nodeid", get_local_node_info()->kad_key(), "nodeid", (node->node_id), "public_ip", node->public_ip, "public_port", node->public_port);
        nodes_.push_back(node);
        nodes_delta_ += 1;
    }

    {
        std::unique_lock<std::mutex> lock(node_id_map_mutex_);
        node_id_map_.insert(std::make_pair(node->node_id, node));
    }

    {
        std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
        TOP_DEBUG("add node_hash_map node_id:%s hash64:%llu public_endpoint:%s:%u", node->node_id.c_str(), node->hash64, node->public_ip.c_str(), node->public_port);
        node_hash_map_->insert(std::make_pair(node->hash64, node));
    }

    {
        std::unique_lock<std::mutex> lock(use_nodes_mutex_);
        no_lock_for_use_nodes_.reset();
        no_lock_for_use_nodes_ = std::make_shared<std::vector<NodeInfoPtr>>(nodes());
    }

    return kKadSuccess;
}

void RootRoutingTable::DumpNodes() {
    // dump all nodes
    {
        std::string fmt("all nodes:\n");
        xdbg("%s", fmt.c_str());
        fmt = base::StringUtil::str_fmt(
            "self]: %s, dis(0), pub(%s:%d)\n", (get_local_node_info()->kad_key()).c_str(), get_local_node_info()->public_ip().c_str(), (int)get_local_node_info()->public_port());
        xdbg("%s", fmt.c_str());
        for (int i = 0; i < (int)nodes_.size(); ++i) {
            // fmt += base::StringUtil::str_fmt("%d: count(%d)\n", kv.first, kv.second);
            fmt = base::StringUtil::str_fmt("%4d]: %s, dis(%d), pub(%s:%d)\n",
                                            (int)i,
                                            HexSubstr(nodes_[i]->node_id).c_str(),
                                            nodes_[i]->bucket_index,
                                            nodes_[i]->public_ip.c_str(),
                                            (int)nodes_[i]->public_port);
            xdbg("%s", fmt.c_str());
        }
    }
}

bool RootRoutingTable::CanAddNode(NodeInfoPtr node) {
    xdbg("node_id(%s), pub(%s:%d)", HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), node->public_port);

    if (node->node_id == get_local_node_info()->kad_key()) {
        xdbg("local node");
        return false;
    }

    if (!ValidNode(node)) {
        xwarn("node invalid.");
        return false;
    }

    if (HasNode(node)) {
        xdbg("has node");
        return false;
    }

    if (SetNodeBucket(node) != kKadSuccess) {
        xwarn("set node bucket index failed![%s]", HexSubstr(node->node_id).c_str());
        return false;
    }

    NodesLock lock(nodes_mutex_);
    SortNodesByTargetXid(get_local_node_info()->kad_key(), nodes_.size());
    NodeInfoPtr remove_node;
    if (!NewNodeReplaceOldNode(node, false)) {
        return false;
    }

    xdbg("node_id(%s) replace success, dis(%d)", HexSubstr(node->node_id).c_str(), node->bucket_index);
    return true;
}

int RootRoutingTable::DropNode(NodeInfoPtr node) {
    {
        NodesLock vec_lock(nodes_mutex_);
        for (auto iter = nodes_.begin(); iter != nodes_.end(); ++iter) {
            if ((*iter)->node_id == node->node_id) {
                EraseNode(iter);
                break;
            }
        }
    }

    {
        std::unique_lock<std::mutex> set_lock(node_id_map_mutex_);
        auto iter = node_id_map_.find(node->node_id);
        if (iter != node_id_map_.end()) {
            node_id_map_.erase(iter);
        }
    }

    {
        std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
        auto iter = node_hash_map_->find(node->hash64);
        if (iter != node_hash_map_->end()) {
            node_hash_map_->erase(iter);
        }
    }

    {
        std::unique_lock<std::mutex> lock(use_nodes_mutex_);
        no_lock_for_use_nodes_.reset();
        no_lock_for_use_nodes_ = std::make_shared<std::vector<NodeInfoPtr>>(nodes());
    }

    return kKadSuccess;
}

int RootRoutingTable::BulkDropNode(const std::vector<std::string> & drop_nodes) {
    std::string tmp_id;
    for (auto & node_id : drop_nodes) {
        {
            NodesLock vec_lock(nodes_mutex_);
            for (auto iter = nodes_.begin(); iter != nodes_.end(); ++iter) {
                if ((*iter)->node_id == node_id) {
                    tmp_id = (*iter)->node_id;
                    EraseNode(iter);
                    break;
                }
            }
        }

        {
            std::unique_lock<std::mutex> set_lock(node_id_map_mutex_);
            auto iter = node_id_map_.find(node_id);
            if (iter != node_id_map_.end()) {
                node_id_map_.erase(iter);
            }
        }

        {
            std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
            auto hash64 = base::xhash64_t::digest(tmp_id);
            auto iter = node_hash_map_->find(hash64);
            if (iter != node_hash_map_->end()) {
                node_hash_map_->erase(iter);
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(use_nodes_mutex_);
        no_lock_for_use_nodes_.reset();
        no_lock_for_use_nodes_ = std::make_shared<std::vector<NodeInfoPtr>>(nodes());
    }

    return kKadSuccess;
}

NodeInfoPtr RootRoutingTable::GetRandomNode() {
    NodesLock lock(nodes_mutex_);
    if (nodes_.empty()) {
        return nullptr;
    }
    return nodes_[RandomUint32() % nodes_.size()];
}

bool RootRoutingTable::GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) {
    NodesLock lock(nodes_mutex_);
    if (nodes_.empty()) {
        return false;
    }

    if (nodes_.size() <= size) {
        vec = nodes_;
        return true;
    }

    auto nsize = nodes_.size();
    std::set<uint32_t> index_set;
    for (size_t i = 0; i < 2 * size; ++i) {
        if (index_set.size() >= size) {
            break;
        }
        auto index = RandomUint32() % nsize;
        index_set.insert(index);
    }
    for (auto & item : index_set) {
        vec.push_back(nodes_[item]);
    }
    return true;
}

std::vector<NodeInfoPtr> RootRoutingTable::nodes() {
    NodesLock lock(nodes_mutex_);
    return nodes_;
}

void RootRoutingTable::GetRangeNodes(const uint64_t & min, const uint64_t & max, std::vector<NodeInfoPtr> & vec) {
#ifndef NDEBUG
    {
        // for test
        std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
        std::string hash_list;
        for (auto & item : *node_hash_map_) {
            hash_list += std::to_string(item.first);
            hash_list += ",";
        }
        TOP_DEBUG("node_hash_map_list:%s", hash_list.c_str());
    }
#endif

    if (min == 0 && max == std::numeric_limits<uint64_t>::max()) {
        vec = nodes();
        return;
    }

    std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
    auto minit = node_hash_map_->lower_bound(min);  // the first item not less than
    auto maxit = node_hash_map_->upper_bound(max);  // the first item greater than
    for (auto it = minit; it != maxit && it != node_hash_map_->end(); ++it) {
        vec.push_back(it->second);
    }
    return;
}

// include min_index, include max_index. [,]
void RootRoutingTable::GetRangeNodes(uint32_t min_index, uint32_t max_index, std::vector<NodeInfoPtr> & vec) {
    if (min_index > max_index) {
        return;
    }
    if (min_index >= node_hash_map_->size() || max_index < 0) {
        return;
    }
    if (max_index >= node_hash_map_->size()) {
        max_index = node_hash_map_->size() - 1;
    }
    if (min_index == 0 && max_index == node_hash_map_->size() - 1) {
        vec = nodes();
        return;
    }

    std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
    auto ibegin = node_hash_map_->begin();
    auto nxit_min = std::next(ibegin, min_index);
    auto nxit_max = std::next(ibegin, max_index + 1);

    for (; nxit_min != nxit_max; ++nxit_min) {
        vec.push_back(nxit_min->second);
    }
    return;
}

int32_t RootRoutingTable::GetSelfIndex() {
    std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
    auto ifind = node_hash_map_->find(get_local_node_info()->hash64());
    if (ifind == node_hash_map_->end()) {
        // std::cout << "not found" << std::endl;
        return -1;
    }
    auto index = std::distance(node_hash_map_->begin(), ifind);
    // std::cout << "index:" << index << std::endl;
    return index;
}

std::size_t RootRoutingTable::nodes_size() {
    NodesLock lock(nodes_mutex_);
    return nodes_.size();
}

NodeInfoPtr RootRoutingTable::GetNode(const std::string & id) {
    std::unique_lock<std::mutex> set_lock(node_id_map_mutex_);
    auto iter = node_id_map_.find(id);
    if (iter != node_id_map_.end()) {
        return iter->second;
    }

    return nullptr;
}

int RootRoutingTable::ClosestToTarget(const std::string & target, bool & closest) {
    if (target == get_local_node_info()->kad_key()) {
        xwarn("target equal local nodeid, CloserToTarget goes wrong");
        return kKadFailed;
    }

    if (nodes_size() == 0) {
        closest = true;
        return kKadSuccess;
    }

    if (target.size() != kNodeIdSize) {
        xinfo("Invalid target_id passed. node id size[%d]", target.size());
        return kKadFailed;
    }

    std::set<std::string> exclude;
    NodeInfoPtr closest_node(GetClosestNode(target, true, exclude));
    if (!closest_node) {
        closest = true;
        return kKadSuccess;
    }
    closest = (closest_node->bucket_index == kSelfBucketIndex) || CloserToTarget(get_local_node_info()->kad_key(), closest_node->node_id, target);
    return kKadSuccess;
}

NodeInfoPtr RootRoutingTable::GetClosestNode(const std::string & target_id, bool not_self, const std::set<std::string> & exclude, bool base_xip) {
    const uint32_t number_to_get = exclude.size() + 1;
    auto closest_nodes(GetClosestNodes(target_id, number_to_get));
    for (const auto & node_info : closest_nodes) {
        if (not_self) {
            if (node_info->node_id == get_local_node_info()->kad_key()) {
                assert(0);
            }
        }

        auto iter = exclude.find(node_info->node_id);
        if (iter != exclude.end()) {
            continue;
        }

        return node_info;
    }
    return nullptr;
}

std::vector<NodeInfoPtr> RootRoutingTable::GetClosestNodes(const std::string & target_id, uint32_t number_to_get) {
    NodesLock lock(nodes_mutex_);
    if (number_to_get == 0) {
        return std::vector<NodeInfoPtr>();
    }

    int sorted_count = 0;
    // if (base_xip) {
    //     sorted_count = SortNodesByTargetXip(target_id, number_to_get);
    // } else {
    sorted_count = SortNodesByTargetXid(target_id, number_to_get);
    // }

    if (sorted_count == 0) {
        return std::vector<NodeInfoPtr>();
    }

    return std::vector<NodeInfoPtr>(std::begin(nodes_), std::begin(nodes_) + static_cast<size_t>(sorted_count));
}

bool RootRoutingTable::HasNode(NodeInfoPtr node) {
    std::unique_lock<std::mutex> lock(node_id_map_mutex_);
    auto iter = node_id_map_.find(node->node_id);
    return iter != node_id_map_.end();
}

NodeInfoPtr RootRoutingTable::FindLocalNode(const std::string node_id) {
    std::unique_lock<std::mutex> lock(node_id_map_mutex_);
    auto iter = node_id_map_.find(node_id);
    if (iter != node_id_map_.end()) {
        return iter->second;
    }
    return nullptr;
}

bool RootRoutingTable::ValidNode(NodeInfoPtr node) {
    if (node->node_id.size() != kNodeIdSize) {
        xwarn("node id size is invalid![%d] should[%d]", node->node_id.size(), kNodeIdSize);
        return false;
    }

#if !defined(XBUILD_CI) && !defined(XBUILD_DEV)
    if(node->public_ip == "0.0.0.0"){
        xwarn("node[%s] public ip 0.0.0.0!", node->node_id.c_str());
        return false;
    }
#endif

    if (node->public_ip.empty() || node->public_port <= 0) {
        xwarn("node[%s] public ip or public port invalid!", node->node_id.c_str());
        return false;
    }
    return true;
}

int RootRoutingTable::SetNodeBucket(NodeInfoPtr node) {
    int id_bit_index(0);
    while (id_bit_index != kNodeIdSize) {
        if (get_local_node_info()->kad_key()[id_bit_index] != node->node_id[id_bit_index]) {
            std::bitset<8> holder_byte(static_cast<int>(get_local_node_info()->kad_key()[id_bit_index]));
            std::bitset<8> node_byte(static_cast<int>(node->node_id[id_bit_index]));
            int bit_index(0);
            while (bit_index != 8U) {
                if (holder_byte[7U - bit_index] != node_byte[7U - bit_index]) {
                    break;
                }
                ++bit_index;
            }

            node->bucket_index = (8 * (kNodeIdSize - id_bit_index)) - bit_index;
            return kKadSuccess;
        }
        ++id_bit_index;
    }
    node->bucket_index = kSelfBucketIndex;
    return kKadFailed;
}

void RootRoutingTable::SortNodesByTargetXid(const std::string & target_xid, std::vector<NodeInfoPtr> & nodes) {
    std::sort(nodes.begin(), nodes.end(), [target_xid, this](const NodeInfoPtr & lhs, const NodeInfoPtr & rhs) { return CloserToTarget(lhs->node_id, rhs->node_id, target_xid); });
}

int RootRoutingTable::SortNodesByTargetXid(const std::string & target_xid, int number) {
    int count = std::min(number, static_cast<int>(nodes_.size()));
    if (count <= 0) {
        return 0;
    }

    std::partial_sort(nodes_.begin(), nodes_.begin() + count, nodes_.end(), [target_xid, this](const NodeInfoPtr & lhs, const NodeInfoPtr & rhs) {
        return CloserToTarget(lhs->node_id, rhs->node_id, target_xid);
    });
    return count;
}

bool RootRoutingTable::CloserToTarget(const std::string & id1, const std::string & id2, const std::string & target_id) {
    for (int i = 0; i < kNodeIdSize; ++i) {
        unsigned char result1 = id1[i] ^ target_id[i];
        unsigned char result2 = id2[i] ^ target_id[i];
        if (result1 != result2) {
            return result1 < result2;
        }
    }
    return false;
}

bool RootRoutingTable::NewNodeReplaceOldNode(NodeInfoPtr node, bool remove) {
    int sum = 0;
    for (auto & n : nodes_) {
        if (n->bucket_index == node->bucket_index) {
            sum += 1;
        }
    }

    // the k-bucket is full
    if (sum >= kKadParamK * 4) {
        xdbg("k-bucket(%d) is full", node->bucket_index);
        return false;
    }

    return true;
}

void RootRoutingTable::GetRandomAlphaNodes(std::map<std::string, NodeInfoPtr> & query_nodes) {
    query_nodes.clear();
    {
        NodesLock lock(nodes_mutex_);
        if (nodes_.size() == 0) {
            return;
        }
        const auto count = std::min((int)nodes_.size(), kKadParamAlphaRandom);
        while ((int)query_nodes.size() < count) {
            uint32_t rand_index = RandomUint32() % nodes_.size();
            auto node = nodes_[rand_index];
            if (query_nodes.find(node->node_id) != query_nodes.end()) {
                continue;  // random again
            }
            query_nodes[node->node_id] = node;
        }
    }
}

void RootRoutingTable::GetClosestAlphaNodes(std::map<std::string, NodeInfoPtr> & query_nodes) {
    query_nodes.clear();
    {
        NodesLock lock(nodes_mutex_);
        // when nodes_.size is not enough
        if (nodes_.size() <= kKadParamAlpha + kKadParamAlphaRandom) {
            for (auto & node : nodes_) {
                query_nodes[node->node_id] = node;
            }
            return;
        }

        // add alpha closest nodes
        const auto count = SortNodesByTargetXid(get_local_node_info()->kad_key(), kKadParamAlpha);
        for (int i = 0; i < count; ++i) {
            query_nodes[nodes_[i]->node_id] = nodes_[i];
        }

        // add alpha random nodes
        while ((int)query_nodes.size() < kKadParamAlpha + kKadParamAlphaRandom) {
            uint32_t rand_index = RandomUint32() % (nodes_.size() - kKadParamAlpha);
            auto node = nodes_[rand_index + kKadParamAlpha];  // without first alpha closest nodes
            if (query_nodes.find(node->node_id) != query_nodes.end()) {
                continue;  // random again
            }
            query_nodes[node->node_id] = node;
        }
    }
}

void RootRoutingTable::GetQueryNodesFromKBucket(std::map<std::string, NodeInfoPtr> & query_nodes) {
    // GetRandomAlphaNodes(query_nodes);
    GetClosestAlphaNodes(query_nodes);
}

std::vector<NodeInfoPtr> RootRoutingTable::GetRandomLocalNodes(const std::vector<NodeInfoPtr> & nodes, size_t n) {
    std::vector<NodeInfoPtr> ret;
    if (nodes.empty()) {
        TOP_WARN("give vector of nodes empty, get nodes failed");
        return ret;
    }
    if (nodes.size() <= n) {
        return nodes;
    }

    std::set<size_t> index_set;
    while (ret.size() < n) {
        uint32_t rand_index = RandomUint32() % nodes.size();
        if (index_set.find(rand_index) != index_set.end()) {
            continue;  // random again
        }

        index_set.insert(rand_index);
        ret.push_back(nodes[rand_index]);
    }
    return ret;
}

void RootRoutingTable::FindElectionNodesInfo(std::map<std::string, top::base::KadmliaKeyPtr> const & kad_keys, std::map<std::string, kadmlia::NodeInfoPtr> & nodes) {
    for (auto & _p : kad_keys) {
        std::string root_kad_key = _p.second->Get();
        if (node_id_map_.find(root_kad_key) != node_id_map_.end()) {
            nodes.insert(std::make_pair(_p.first, node_id_map_[root_kad_key]));
        }
    }
}

void RootRoutingTable::FindClosestNodes(int count, const std::vector<NodeInfoPtr> & nodes) {
    xdbg("<bluefind> FindClosestNodes(count=%d, nodes.size=%zu)", count, nodes.size());

    std::map<std::string, NodeInfoPtr> query_nodes;
    const size_t need_nodes_size = kKadParamAlpha + kKadParamAlphaRandom;
    GetQueryNodesFromKBucket(query_nodes);
    if (query_nodes.size() < need_nodes_size) {
        std::unique_lock<std::mutex> lock(bootstrap_nodes_mutex_);
        auto boot_nodes = GetRandomLocalNodes(bootstrap_nodes_, need_nodes_size - query_nodes.size());
        for (auto & node : boot_nodes) {
            query_nodes[node->node_id] = node;
        }
    }

    xdbg("findnodes count: %zu", query_nodes.size());
    for (auto & kv : query_nodes) {
        SendFindClosestNodes(kv.second, count, nodes, get_local_node_info()->service_type());
    }
}

int RootRoutingTable::Bootstrap(const std::string & peer_ip, uint16_t peer_port, base::ServiceType des_service_type) {
    xdbg("Bootstrap to (%s:%d_%ld)", peer_ip.c_str(), (int)peer_port, (long)des_service_type.value());
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_des_service_type(des_service_type.value());
    message.set_priority(enum_xpacket_priority_type_flash);
    xinfo("join with service type[%llu] ,src[%llu], des[%llu]",
          get_local_node_info()->service_type().value(),
          get_local_node_info()->service_type().value(),
          des_service_type.value());
    message.set_des_node_id("");
    message.set_type(kKadBootstrapJoinRequest);

    protobuf::BootstrapJoinRequest join_req;
    join_req.set_xid(global_xid->Get());
    std::string data;
    if (!join_req.SerializeToString(&data)) {
        xinfo("ConnectReq SerializeToString failed!");
        return kKadFailed;
    }

    message.set_data(data);
    return SendData(message, peer_ip, peer_port);
}

void RootRoutingTable::FindCloseNodesWithEndpoint(const std::string & des_node_id, const std::pair<std::string, uint16_t> & boot_endpoints) {
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_des_service_type(get_local_node_info()->service_type().value());
    message.set_des_node_id(des_node_id);
    message.set_type(kKadFindNodesRequest);
    message.set_priority(enum_xpacket_priority_type_flash);

    protobuf::FindClosestNodesRequest find_nodes_req;
    find_nodes_req.set_count(GetFindNodesMaxSize());
    find_nodes_req.set_target_id(get_local_node_info()->kad_key());

    auto src_nodeinfo_ptr = find_nodes_req.mutable_src_nodeinfo();
    src_nodeinfo_ptr->set_id(get_local_node_info()->kad_key());
    src_nodeinfo_ptr->set_public_ip(get_local_node_info()->public_ip());
    src_nodeinfo_ptr->set_public_port(get_local_node_info()->public_port());
    src_nodeinfo_ptr->set_xid(global_xid->Get());

    std::string data;
    if (!find_nodes_req.SerializeToString(&data)) {
        xinfo("ConnectReq SerializeToString failed!");
        return;
    }

    message.set_data(data);
    SendData(message, boot_endpoints.first, boot_endpoints.second);
}

int RootRoutingTable::SendFindClosestNodes(const NodeInfoPtr & node_ptr, int count, const std::vector<NodeInfoPtr> & nodes, base::ServiceType des_service_type) {
    const auto node_id = node_ptr->node_id;
    xdbg("bluefindnodes to %s %s:%d", HexSubstr(node_id).c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_des_service_type(des_service_type.value());
    message.set_des_node_id(node_id);
    message.set_type(kKadFindNodesRequest);
    message.set_priority(enum_xpacket_priority_type_flash);

    protobuf::FindClosestNodesRequest find_nodes_req;
    find_nodes_req.set_count(count);
    find_nodes_req.set_target_id(get_local_node_info()->kad_key());
    std::vector<uint64_t> bloomfilter_vec;
    GetExistsNodesBloomfilter(nodes, bloomfilter_vec);
    for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
        find_nodes_req.add_bloomfilter(bloomfilter_vec[i]);
    }

    auto src_nodeinfo_ptr = find_nodes_req.mutable_src_nodeinfo();
    src_nodeinfo_ptr->set_id(get_local_node_info()->kad_key());
    src_nodeinfo_ptr->set_public_ip(get_local_node_info()->public_ip());
    src_nodeinfo_ptr->set_public_port(get_local_node_info()->public_port());
    src_nodeinfo_ptr->set_xid(global_xid->Get());

    std::string data;
    if (!find_nodes_req.SerializeToString(&data)) {
        xinfo("ConnectReq SerializeToString failed!");
        return kKadFailed;
    }

    message.set_data(data);
    xdbg("sendfindclosestnodes: message.is_root(%d), message.des_service_type:(%llu), local_service_type:(%llu)",
         message.is_root(),
         message.des_service_type(),
         get_local_node_info()->kadmlia_key()->GetServiceType().value());
    xdbg("bluefind send_find to node: %s", HexSubstr(node_ptr->node_id).c_str());
    SendData(message, node_ptr);
    return kKadSuccess;
}

void RootRoutingTable::GetExistsNodesBloomfilter(const std::vector<NodeInfoPtr> & nodes, std::vector<uint64_t> & bloomfilter_vec) {
    base::Uint64BloomFilter bloomfilter{kFindNodesBloomfilterBitSize, kFindNodesBloomfilterHashNum};
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        bloomfilter.Add(nodes[i]->node_id);
    }
    bloomfilter_vec = bloomfilter.Uint64Vector();
}

bool RootRoutingTable::IsDestination(const std::string & des_node_id, bool check_closest) {
    if (des_node_id == get_local_node_info()->kad_key()) {
        return true;
    }

    if (!check_closest) {
        return false;
    }

    bool closest = false;
    if (ClosestToTarget(des_node_id, closest) != kKadSuccess) {
        xwarn(
            "this message must drop! this node is not des "
            "but nearest node is this node![%s] to [%s]",
            (get_local_node_info()->kad_key()).c_str(),
            (des_node_id).c_str());
        return false;
    }

    return closest;
}

void RootRoutingTable::HandleFindNodesRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    xdbg("bluefindnodes from %s %s:%d", HexSubstr(message.src_node_id()).c_str(), packet.get_from_ip_addr().c_str(), packet.get_from_ip_port());
    
    if (!joined_) {
        // forbidden handle bootstrap request when offline or self not join network yet
        TOP_WARN("forbidden HandleBootstrapJoinRequest when offline or self not join network yet");
        return;
    }
    
    if (message.des_node_id() != get_local_node_info()->kad_key()) {
        xwarn("find nodes must direct![des: %s] [local: %s] [msg.src: %s] [msg.is_root: %d]",
              (message.des_node_id()).c_str(),
              (get_local_node_info()->kad_key()).c_str(),
              (message.src_node_id()).c_str(),
              message.is_root());
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xinfo("HandleFindNodesRequest request in data is empty.");
        return;
    }

    protobuf::FindClosestNodesRequest find_nodes_req;
    if (!find_nodes_req.ParseFromString(message.data())) {
        xinfo("FindClosestNodesRequest ParseFromString from string failed!");
        return;
    }

    // asker node canadd to local routingtable?
    auto src_nodeinfo = find_nodes_req.src_nodeinfo();
    NodeInfoPtr req_src_node_ptr;
    req_src_node_ptr.reset(new NodeInfo(src_nodeinfo.id()));
    req_src_node_ptr->public_ip = src_nodeinfo.public_ip();
    req_src_node_ptr->public_port = src_nodeinfo.public_port();
    req_src_node_ptr->xid = src_nodeinfo.xid();
    req_src_node_ptr->hash64 = base::xhash64_t::digest(req_src_node_ptr->node_id);

    bool ask_public_endpoint = false;
    if (src_nodeinfo.public_ip().empty() || src_nodeinfo.public_port() <= 0) {
        ask_public_endpoint = true;
    }

    if (CanAddNode(req_src_node_ptr)) {
        AddNode(req_src_node_ptr);
    }

    std::vector<uint64_t> bloomfilter_vec;
    for (auto i = 0; i < find_nodes_req.bloomfilter_size(); ++i) {
        bloomfilter_vec.push_back(find_nodes_req.bloomfilter(i));
    }

    std::shared_ptr<base::Uint64BloomFilter> new_bloomfilter;
    if (bloomfilter_vec.empty()) {
        new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(kadmlia::kFindNodesBloomfilterBitSize, kadmlia::kFindNodesBloomfilterHashNum);
    } else {
        new_bloomfilter = std::make_shared<base::Uint64BloomFilter>(bloomfilter_vec, kadmlia::kFindNodesBloomfilterHashNum);
    }

    std::vector<NodeInfoPtr> closest_nodes = GetClosestNodes(find_nodes_req.target_id(), find_nodes_req.count() + 1);
    xdbg("bluefind closest_nodes.size=%d", (int)closest_nodes.size());
    std::string find_nodes;
    protobuf::FindClosestNodesResponse find_nodes_res;

    for (uint32_t i = 0; i < closest_nodes.size(); ++i) {
        if (closest_nodes[i]->node_id == find_nodes_req.target_id()) {
            continue;
        }

        if (new_bloomfilter->Contain(closest_nodes[i]->node_id)) {
            continue;
        }

        protobuf::NodeInfo * tmp_node = find_nodes_res.add_nodes();
        tmp_node->set_id(closest_nodes[i]->node_id);
        tmp_node->set_public_ip(closest_nodes[i]->public_ip);
        tmp_node->set_public_port(closest_nodes[i]->public_port);
        tmp_node->set_xid(closest_nodes[i]->xid);
        find_nodes += closest_nodes[i]->public_ip + ", ";
    }

    if (find_nodes_res.nodes_size() <= 0) {
        return;
    }
    xdbg("HandleFindNodesRequest: get %d nodes", find_nodes_res.nodes_size());
    xdbg("<bluefind> recv_find: %d nodes from node %s", find_nodes_res.nodes_size(), HexSubstr(message.src_node_id()).c_str());

    // separate nodes into multiple udp packets
    std::vector<std::string> datas;
    const int N_NODES_ONCE = 9;  // TODO(blueshi): depends on NodeInfo ByteSize(126B),1334(MAX)
    for (int n_nodes_sent = 0; n_nodes_sent < find_nodes_res.nodes_size(); n_nodes_sent += N_NODES_ONCE) {
        protobuf::FindClosestNodesResponse res;
        int n_nodes_once_current = std::min(N_NODES_ONCE, find_nodes_res.nodes_size() - n_nodes_sent);
        for (int k = 0; k < n_nodes_once_current; ++k) {
            res.add_nodes()->CopyFrom(find_nodes_res.nodes(n_nodes_sent + k));  // TODO(blueshi): opt copy!
            if (ask_public_endpoint) {
                res.set_public_ip(packet.get_from_ip_addr());
                res.set_public_port(packet.get_from_ip_port());
                ask_public_endpoint = false;
                TOP_DEBUG("findnodes ask public endpoints, return  %s:%u", packet.get_from_ip_addr().c_str(), packet.get_from_ip_port());
            }
            datas.push_back(res.SerializeAsString());
        }
    }

    for (const auto & data : datas) {
        transport::protobuf::RoutingMessage res_message;
        SetFreqMessage(res_message);  // for RootRouting, this virtual func will set is_root true
        res_message.set_des_node_id(message.src_node_id());
        res_message.set_type(kKadFindNodesResponse);
        res_message.set_id(message.id());
        res_message.set_data(data);
        res_message.set_priority(enum_xpacket_priority_type_flash);

        SendData(res_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
    }
}

void RootRoutingTable::HandleFindNodesResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    xdbg("bluefindnodes from %s %s:%d", HexSubstr(message.src_node_id()).c_str(), packet.get_from_ip_addr().c_str(), packet.get_from_ip_port());
    if (message.des_node_id() != get_local_node_info()->kad_key()) {
        xwarn("find nodes response error. must direct receive!");
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xinfo("HandleFindNodesResponse data is empty.");
        return;
    }

    protobuf::FindClosestNodesResponse find_nodes_res;
    if (!find_nodes_res.ParseFromString(message.data())) {
        xinfo("FindClosestNodesResponse ParseFromString from string failed!");
        return;
    }

    if (find_nodes_res.has_public_port() && find_nodes_res.public_port() > 0) {
        get_local_node_info()->set_public_ip(find_nodes_res.public_ip());
        get_local_node_info()->set_public_port(find_nodes_res.public_port());
        xinfo("set public_endpoints %s:%u", find_nodes_res.public_ip().c_str(), find_nodes_res.public_port());
    }

    xdbg("HandleFindNodesResponse get %d nodes", find_nodes_res.nodes_size());
    for (int i = 0; i < find_nodes_res.nodes_size(); ++i) {
        NodeInfoPtr node_ptr;
        node_ptr.reset(new NodeInfo(find_nodes_res.nodes(i).id()));
        node_ptr->public_ip = find_nodes_res.nodes(i).public_ip();
        node_ptr->public_port = find_nodes_res.nodes(i).public_port();
        node_ptr->service_type = base::ServiceType::build_from(message.src_service_type());  // for RootRouting, is always kRoot
        node_ptr->xid = find_nodes_res.nodes(i).xid();
        node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
        if (CanAddNode(node_ptr)) {
            if (get_local_node_info()->public_ip() == node_ptr->public_ip && node_ptr->public_port == get_local_node_info()->public_port()) {
                if (node_ptr->node_id != get_local_node_info()->kad_key()) {
                    xdbg("bluenat[%d] get node(%s:%d-%d)",
                         get_local_node_info()->service_type().value(),
                         node_ptr->public_ip.c_str(),
                         node_ptr->public_port,
                         node_ptr->service_type.value());
                    node_ptr->xid = global_xid->Get();
                    node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
                    if (AddNode(node_ptr) == kKadSuccess) {
                        xdbg("update add_node(%s) from find node response(%s, %s:%d)",
                             HexSubstr(node_ptr->node_id).c_str(),
                             HexSubstr(message.src_node_id()).c_str(),
                             packet.get_from_ip_addr().c_str(),
                             packet.get_from_ip_port());
                    }
                }
                continue;
            }

            xdbg("find node: %s:%d ", node_ptr->public_ip.c_str(), node_ptr->public_port);
            node_detection_ptr_->AddDetectionNode(node_ptr);
        }
    }
}

void RootRoutingTable::OnHeartbeatFailed(const std::string & ip, uint16_t port) {
    std::vector<NodeInfoPtr> failed_nodes;
    {
        NodesLock lock(nodes_mutex_);
        for (auto it = nodes_.begin(); it != nodes_.end();) {
            NodeInfoPtr node = *it;
            if (!node) {
                TOP_WARN("node empty, invalid");
                it = EraseNode(it);
                continue;
            }
            if (node->public_ip == ip && node->public_port == port) {
                failed_nodes.push_back(node);
            }
            ++it;
        }
    }

    for (auto & node : failed_nodes) {
        DropNode(node);
        xwarn("[%ld] node heartbeat error .ID:[%s],IP:[%s],Port[%d] to ID:[%s],IP[%s],Port[%d] drop it.",
              (long)this,
              // node->heartbeat_count,
              (get_local_node_info()->kad_key()).c_str(),
              get_local_node_info()->local_ip().c_str(),
              get_local_node_info()->local_port(),
              (node->node_id).c_str(),
              node->local_ip.c_str(),
              node->local_port);
    }
}

void RootRoutingTable::HandleHandshake(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!IsDestination(message.des_node_id(), false)) {
        xwarn("handshake message destination id error![%s][%s][%s]", message.src_node_id().c_str(), message.des_node_id().c_str(), get_local_node_info()->kad_key().c_str());
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xwarn("connect request in data is empty.");
        return;
    }

    protobuf::Handshake handshake;
    if (!handshake.ParseFromString(message.data())) {
        xwarn("ConnectRequest ParseFromString from string failed!");
        return;
    }

    NodeInfoPtr node_ptr;
    node_ptr.reset(new NodeInfo(message.src_node_id()));
    std::string pub_ip = handshake.public_ip();
    uint16_t pub_port = handshake.public_port();
    if (pub_ip.empty()) {
        pub_ip = packet.get_from_ip_addr();
        pub_port = packet.get_from_ip_port();
    }
    assert(!pub_ip.empty());
    node_ptr->public_ip = pub_ip;
    node_ptr->public_port = pub_port;
    node_ptr->service_type = base::ServiceType::build_from(message.src_service_type());
    node_ptr->xid = handshake.xid();
    node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
    if (handshake.type() == kHandshakeResponse) {
        node_detection_ptr_->RemoveDetection(node_ptr->public_ip, node_ptr->public_port);

        if (AddNode(node_ptr) == kKadSuccess) {
            xdbg("update add_node(%s) from handshake(%s, %s:%d)",
                 (node_ptr->node_id).c_str(),
                 (message.src_node_id()).c_str(),
                 packet.get_from_ip_addr().c_str(),
                 packet.get_from_ip_port());
        }

        // if (!joined_) {
        //     if (!SetJoin(message.src_node_id(), handshake.public_ip(), handshake.public_port())) {
        //         xinfo("ignore BootstrapJoinResponse because this node already joined");
        //     }
        // }
        return;
    }

    if (!joined_) {
        TOP_WARN("forbidden kHandshakeRequest when offline or self not join network yet");
        return;
    }

    // kHandshakeRequest
    if (CanAddNode(node_ptr)) {
        AddNode(node_ptr);
    }
    handshake.set_type(kHandshakeResponse);
    handshake.set_public_ip(get_local_node_info()->public_ip());
    handshake.set_public_port(get_local_node_info()->public_port());
    handshake.set_xid(global_xid->Get());
    std::string data;
    if (!handshake.SerializeToString(&data)) {
        xwarn("ConnectResponse SerializeToString failed!");
        return;
    }

    transport::protobuf::RoutingMessage res_message;
    SetFreqMessage(res_message);
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(message.src_service_type());
    res_message.set_src_node_id(message.des_node_id());
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kKadHandshake);
    res_message.set_id(message.id());
    message.set_priority(enum_xpacket_priority_type_flash);

    res_message.set_data(data);
    SendPing(res_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
}

void RootRoutingTable::HandleBootstrapJoinRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    xdbg("HandleBootstrapJoinRequest from %s:%d", packet.get_from_ip_addr().c_str(), (int)packet.get_from_ip_port());
    // bool allow_add = true;
    if (!message.has_data() || message.data().empty()) {
        xinfo("HandleBootstrapJoinRequest request in data is empty.");
        return;
    }

    protobuf::BootstrapJoinRequest join_req;
    if (!join_req.ParseFromString(message.data())) {
        xinfo("BootstrapJoinRequest ParseFromString from string failed!");
        return;
    }

    if (!joined_ && m_endpoint_ip_port.find(packet.get_from_ip_addr() + ":" + std::to_string(packet.get_from_ip_port())) == m_endpoint_ip_port.end()) {
        // forbidden handle bootstrap request when offline or self not join network yet
        TOP_WARN("forbidden HandleBootstrapJoinRequest when offline or self not join network yet");
        return;
    }

    NodeInfoPtr node_ptr;
    node_ptr.reset(new NodeInfo(message.src_node_id()));
    node_ptr->public_ip = packet.get_from_ip_addr();
    node_ptr->public_port = packet.get_from_ip_port();
    node_ptr->service_type = base::ServiceType::build_from(message.src_service_type());
    node_ptr->xid = join_req.xid();
    node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
    SendBootstrapJoinResponse(message, packet);

    xdbg("[%llu] get node(%s:%d-%llu)", get_local_node_info()->service_type().value(), node_ptr->public_ip.c_str(), node_ptr->public_port, node_ptr->service_type.value());
    if (AddNode(node_ptr) == kKadSuccess) {
        xdbg("update add_node(%s) from bootstrap request(%s, %s:%d)",
             HexSubstr(node_ptr->node_id).c_str(),
             HexSubstr(message.src_node_id()).c_str(),
             packet.get_from_ip_addr().c_str(),
             packet.get_from_ip_port());
    }
}

void RootRoutingTable::SendBootstrapJoinResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    xdbg("SendBootstrapJoinResponse to (%s:%d)", packet.get_from_ip_addr().c_str(), (int)packet.get_from_ip_port());
    transport::protobuf::RoutingMessage res_message;
    // TODO(smaug) message.des_service_type maybe not equal the service_type of this routing table
    SetFreqMessage(res_message);
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(message.src_service_type());
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kKadBootstrapJoinResponse);
    res_message.set_id(message.id());
    message.set_priority(enum_xpacket_priority_type_flash);
    res_message.set_debug("join res");

    protobuf::BootstrapJoinResponse join_res;
    join_res.set_public_ip(packet.get_from_ip_addr());
    join_res.set_public_port(packet.get_from_ip_port());
    join_res.set_xid(global_xid->Get());

    if (join_res.public_ip().empty() || join_res.public_port() <= 0) {
        xwarn("join node [%s] get public ip or public port failed!", HexSubstr(message.src_node_id()).c_str());
        return;
    }

    join_res.set_bootstrap_id(get_local_node_info()->kad_key());
    std::string data;
    if (!join_res.SerializeToString(&data)) {
        xinfo("ConnectResponse SerializeToString failed!");
        return;
    }

    res_message.set_data(data);
    SendData(res_message, packet.get_from_ip_addr(), packet.get_from_ip_port());
}

void RootRoutingTable::HandleBootstrapJoinResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    xdbg("HandleBootstrapJoinResponse from %s:%d", packet.get_from_ip_addr().c_str(), (int)packet.get_from_ip_port());
    if (!IsDestination(message.des_node_id(), false)) {
        xwarn("HandleBootstrapJoinResponse message destination id error![%s][%s][%s]",
              (message.src_node_id()).c_str(),
              (message.des_node_id()).c_str(),
              (get_local_node_info()->kad_key()).c_str());
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xinfo("ConnectResponse data is empty.");
        return;
    }

    protobuf::BootstrapJoinResponse join_res;
    if (!join_res.ParseFromString(message.data())) {
        xinfo("ConnectResponse ParseFromString failed!");
        return;
    }

    NodeInfoPtr node_ptr;
    node_ptr.reset(new NodeInfo(message.src_node_id()));
    node_ptr->local_ip = packet.get_from_ip_addr();
    node_ptr->local_port = packet.get_from_ip_port();
    node_ptr->public_ip = packet.get_from_ip_addr();
    node_ptr->public_port = packet.get_from_ip_port();
    node_ptr->service_type = base::ServiceType::build_from(message.src_service_type());
    node_ptr->xid = join_res.xid();
    node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);

    {
        std::unique_lock<std::mutex> lock(joined_mutex_);
        get_local_node_info()->set_public_ip(join_res.public_ip());
        get_local_node_info()->set_public_port(join_res.public_port());
    }

    {
        std::unique_lock<std::mutex> bootstrap_lock(bootstrap_nodes_mutex_);
        bootstrap_nodes_.push_back(node_ptr);
        xinfo("add bootstrap node success. id:%s ip:%s port:%d", node_ptr->node_id.c_str(), (node_ptr->public_ip).c_str(), node_ptr->public_port);
    }
    if (!SetJoin(join_res.bootstrap_id(), packet.get_from_ip_addr(), packet.get_from_ip_port())) {
        xinfo("ignore BootstrapJoinResponse because this node already joined");
        return;
    }

    std::vector<NodeInfoPtr> boot_node;
    FindClosestNodes(GetFindNodesMaxSize(), boot_node);  // do find
    WakeBootstrap();
    return;
}

void RootRoutingTable::SetFreqMessage(transport::protobuf::RoutingMessage & message) {
    message.set_hop_num(0);
    message.set_src_service_type(get_local_node_info()->service_type().value());
    message.set_src_node_id(get_local_node_info()->kad_key());
    message.set_xid(global_xid->Get());
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_id(CallbackManager::MessageId());
    if (message.broadcast()) {
        auto gossip = message.mutable_gossip();
        gossip->set_neighber_count(4);
        gossip->set_stop_times(gossip::kGossipSendoutMaxTimes);
        gossip->set_gossip_type(gossip::kGossipBloomfilter);
        gossip->set_max_hop_num(10);
    }
}

std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> RootRoutingTable::GetUnLockNodes() {
    std::unique_lock<std::mutex> lock(use_nodes_mutex_);
    return no_lock_for_use_nodes_;
}

std::vector<NodeInfoPtr>::iterator RootRoutingTable::EraseNode(std::vector<NodeInfoPtr>::iterator it) {
    XMETRICS_PACKET_INFO("p2p_kad_dropnode",
                         "local_nodeid",
                         (get_local_node_info()->kad_key()),
                         "nodeid",
                         HexSubstr((*it)->node_id),
                         "public_ip",
                         (*it)->public_ip,
                         "public_port",
                         (*it)->public_port);

    auto it2 = nodes_.erase(it);
    nodes_delta_ += 1;
    return it2;
}

}  // namespace kadmlia

}  // namespace top
