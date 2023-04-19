// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "heartbeat_manager.h"
#include "xbase/xbase.h"
#include "xbase/xcxx_config.h"
#if defined(XCXX20)
#include "xkad/proto/ubuntu/kadmlia.pb.h"
#else
#include "xkad/proto/centos/kadmlia.pb.h"
#endif
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_timer.h"
#if defined(XCXX20)
#include "xtransport/proto/ubuntu/transport.pb.h"
#else
#include "xtransport/proto/centos/transport.pb.h"
#endif
#include "xtransport/transport.h"

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace top {

namespace kadmlia {

class LocalNodeInfo;

class ElectRoutingTable : public std::enable_shared_from_this<ElectRoutingTable> {
public:
    ElectRoutingTable(std::shared_ptr<transport::Transport>, std::shared_ptr<LocalNodeInfo>);
    ElectRoutingTable(ElectRoutingTable const &) = delete;
    ElectRoutingTable & operator=(ElectRoutingTable const &) = delete;
    ElectRoutingTable(ElectRoutingTable &&) = default;
    ElectRoutingTable & operator=(ElectRoutingTable &&) = default;
    ~ElectRoutingTable();
    bool Init();
    bool UnInit();

public:
    base::ServiceType GetRoutingTableType() {
        return local_node_ptr_->service_type();
    }
    std::shared_ptr<transport::Transport> get_transport() {
        return transport_ptr_;
    }
    std::shared_ptr<LocalNodeInfo> get_local_node_info() {
        return local_node_ptr_;
    }

    int SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);
    int SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node_ptr);
    int SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);

    std::size_t nodes_size();

public:
    void GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size);
    NodeInfoPtr GetOneRandomNode();
    bool lack_last_round_nodes() const {
        return m_lack_last_round_nodes;
    }
    void set_lack_last_round_nodes(bool value) {
        m_lack_last_round_nodes = value;
    }
    std::unordered_map<std::string, NodeInfoPtr> nodes(bool cover_old_version = false);
    std::unordered_map<std::string, std::size_t> index_map(bool cover_old_version = false);
    std::vector<std::string> get_shuffled_xip2(bool cover_old_version = false);
    std::size_t get_self_index();

    NodeInfoPtr GetNode(const std::string & id);

    bool CloserToTarget(const std::string & id1, const std::string & id2, const std::string & target_id);

public:
    std::map<std::string, base::KadmliaKeyPtr> const & GetAllNodesRootKeyMap() const noexcept;

    // map<election_xip2_str,node_id_root_kad_key>
    void SetElectionNodesExpected(std::map<std::string, base::KadmliaKeyPtr> const & elect_root_kad_keys_map, std::map<std::string, NodeInfoPtr> const & last_round_nodes_map);
    void EraseElectionNodesExpected(std::vector<base::KadmliaKeyPtr> const & kad_keys);
    void DoEraseElectionNodesExpected(std::string const & node_id);
    std::map<std::string, base::KadmliaKeyPtr> GetElectionNodesExpected();

    void HandleElectionNodesInfoFromRoot(std::map<std::string, kadmlia::NodeInfoPtr> const & nodes);

    // multi_rooting call this to insert node_info got from other nodes' root routing
    void OnFindNodesFromRootRouting(std::string const & election_xip2, kadmlia::NodeInfoPtr const & node_info);

    void getLastRoundElectNodesInfo(std::vector<std::pair<std::string, NodeInfoPtr>> & nodes_info);
    // multi_routing call this to insert last round node_info got from other nodes.(Only when this node is new electIn)
    void OnAddLastRoundElectNodes(std::vector<std::pair<std::string, NodeInfoPtr>> const & nodes);

private:
    void PrintRoutingTable();
    void OnHeartbeatFailed(const std::string & ip, uint16_t port);
    void UpdateBroadcastNodeInfo();

private:
    std::shared_ptr<transport::Transport> transport_ptr_;
    std::shared_ptr<LocalNodeInfo> local_node_ptr_;

private:
    base::TimerManager * timer_manager_{base::TimerManager::Instance()};

    std::shared_ptr<base::TimerRepeated> timer_heartbeat_;

    bool destroy_;

private:
    std::size_t m_self_index;
    std::mutex m_nodes_mutex;
    std::unordered_map<std::string, NodeInfoPtr> m_nodes;            // map<election_xip2_str,nodeinfoptr>
    std::mutex m_expected_kad_keys_mutex;
    std::map<std::string, base::KadmliaKeyPtr> m_expected_kad_keys;  // map<election_xip2_str,node_id_root_kad_key>
    std::map<std::string, base::KadmliaKeyPtr> m_all_nodes_root_kay_keys;
    std::unordered_map<std::string, std::size_t> m_index_map;  // map<election_xip2_str,index>
    std::mutex m_xip2_for_shuffle_mutex;
    std::vector<std::string> m_xip2_for_shuffle;               // random shuffled everytime used.

    // for 2 round broadcast
    std::mutex m_broadcast_nodes_mutex;
    std::unordered_map<std::string, NodeInfoPtr> m_broadcast_nodes;
    std::unordered_map<std::string, std::size_t> m_broadcast_index_map;
    std::mutex m_broadcast_xip2_for_shuffle_mutex;
    std::vector<std::string> m_broadcast_xip2_for_shuffle;
    bool m_lack_last_round_nodes{false};
};

typedef std::shared_ptr<ElectRoutingTable> ElectRoutingTablePtr;

}  // namespace kadmlia

}  // namespace top
