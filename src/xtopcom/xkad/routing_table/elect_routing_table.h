// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "heartbeat_manager.h"
#include "xbase/xbase.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_table_base.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_timer.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport.h"

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace top {

namespace kadmlia {

class LocalNodeInfo;

class ElectRoutingTable
  : public RoutingTableBase
  , public std::enable_shared_from_this<ElectRoutingTable> {
public:
    ElectRoutingTable(std::shared_ptr<transport::Transport>, std::shared_ptr<LocalNodeInfo>);
    ElectRoutingTable(ElectRoutingTable const &) = delete;
    ElectRoutingTable & operator=(ElectRoutingTable const &) = delete;
    ElectRoutingTable(ElectRoutingTable &&) = default;
    ElectRoutingTable & operator=(ElectRoutingTable &&) = default;
    ~ElectRoutingTable() override = default;
    bool Init() override;
    bool UnInit() override;

    
    std::size_t nodes_size() override;
    std::vector<NodeInfoPtr> GetClosestNodes(const std::string & target_id, uint32_t number_to_get) override;

public:
    int AddNode(NodeInfoPtr node) override;
    int DropNode(NodeInfoPtr node) override;
    bool CanAddNode(NodeInfoPtr node);

    NodeInfoPtr GetRandomNode() override;
    bool GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) override;
    std::unordered_map<std::string, NodeInfoPtr> nodes();
    NodeInfoPtr GetNode(const std::string & id);

    bool HasNode(NodeInfoPtr node);
    bool CloserToTarget(const std::string & id1, const std::string & id2, const std::string & target_id);
    // NodeInfoPtr FindLocalNode(const std::string node_id);

public:
    // map<election_xip2_str,node_id_root_kad_key>
    void SetElectionNodesExpected(std::map<std::string, base::KadmliaKeyPtr> const & elect_root_kad_keys_map);
    void EraseElectionNodesExpected(std::vector<base::KadmliaKeyPtr> const & kad_keys);
    std::map<std::string, base::KadmliaKeyPtr> GetElectionNodesExpected();

    void HandleElectionNodesInfoFromRoot(std::map<std::string, kadmlia::NodeInfoPtr> const & nodes);

    // multi_rooting call this to insert node_info got from other nodes' root routing
    void OnFindNodesFromRootRouting(std::string const & election_xip2, kadmlia::NodeInfoPtr const & node_info);

private:
    void PrintRoutingTable();
    void OnHeartbeatFailed(const std::string & ip, uint16_t port);
    void SetFreqMessage(transport::protobuf::RoutingMessage & message);

private:
    base::TimerManager * timer_manager_{base::TimerManager::Instance()};

    std::shared_ptr<base::TimerRepeated> timer_heartbeat_;

    bool destroy_;

private:
    std::map<std::string, base::KadmliaKeyPtr> m_expected_kad_keys;  // map<election_xip2_str,node_id_root_kad_key>
    std::unordered_map<std::string, NodeInfoPtr> m_nodes;            // map<election_xip2_str,nodeinfoptr>
};

typedef std::shared_ptr<ElectRoutingTable> ElectRoutingTablePtr;

}  // namespace kadmlia

}  // namespace top
