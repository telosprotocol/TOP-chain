// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcxx_config.h"
#if defined(XCXX20)
#include "xkad/proto/ubuntu/kadmlia.pb.h"
#else
#include "xkad/proto/centos/kadmlia.pb.h"
#endif
#include "xkad/routing_table/callback_manager.h"
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
#include <unordered_set>
#include <vector>
#include "heartbeat_manager.h"
#include "xbase/xbase.h"
#include "xkad/routing_table/local_node_info.h"

namespace top {

namespace kadmlia {

class NodeDetectionManager;
class LocalNodeInfo;

class RootRoutingTable
  : public std::enable_shared_from_this<RootRoutingTable> {
public:
    RootRoutingTable(std::shared_ptr<transport::Transport> transport_ptr, std::shared_ptr<LocalNodeInfo> local_node_ptr);
    virtual ~RootRoutingTable()  = default;
    bool Init() ;
    bool UnInit() ;
    int AddNode(NodeInfoPtr node) ;
    int DropNode(NodeInfoPtr node) ;

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
public:
    int SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);
    int SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node_ptr);
    int SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);

    std::size_t nodes_size() ;    
    std::vector<NodeInfoPtr> GetClosestNodes(const std::string & target_id, uint32_t number_to_get) ;

    NodeInfoPtr GetRandomNode() ;
    bool GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) ;

     int BulkDropNode(const std::vector<std::string> & drop_nodes);

     bool IsDestination(const std::string & des_node_id, bool check_closest);
    void SetFreqMessage(transport::protobuf::RoutingMessage & message);

     void PrintRoutingTable();

    // message handler
    // virtual void HandleMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
     void HandleFindNodesRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
     void HandleFindNodesResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

     void HandleHandshake(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
     void SendBootstrapJoinResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
     void HandleBootstrapJoinRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
     void HandleBootstrapJoinResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

public:
    std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> GetUnLockNodes();

    // void HandleNodeQuit(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void SortNodesByTargetXid(const std::string & target_xid, std::vector<NodeInfoPtr> & nodes);

    bool CanAddNode(NodeInfoPtr node);
    int ClosestToTarget(const std::string & target, bool & closest);

    void FindClosestNodes(int count, const std::vector<NodeInfoPtr> & nodes);
    std::vector<NodeInfoPtr> nodes();
    void GetRangeNodes(const uint64_t & min, const uint64_t & max, std::vector<NodeInfoPtr> & vec);
    void GetRangeNodes(uint32_t min_index,
                       uint32_t max_index,  // [,]
                       std::vector<NodeInfoPtr> & vec);
    int32_t GetSelfIndex();
    int MultiJoin(const std::set<std::pair<std::string, uint16_t>> & boot_endpoints);
    // void MultiJoinAsync(const std::set<std::pair<std::string, uint16_t>> & boot_endpoints);
    bool IsJoined();
    // void SetUnJoin();
    void WakeBootstrap();
    void GetQueryNodesFromKBucket(std::map<std::string, NodeInfoPtr> & query_nodes);
    std::vector<NodeInfoPtr> GetRandomLocalNodes(const std::vector<NodeInfoPtr> & nodes, size_t n);
    NodeInfoPtr GetNode(const std::string & id);
    NodeInfoPtr GetClosestNode(const std::string & target_id, bool not_self, const std::set<std::string> & exclude, bool base_xip = false);
    void SendToClosestNode(transport::protobuf::RoutingMessage & message);
    void SendToClosestNode(transport::protobuf::RoutingMessage & message, bool add_hop);
    void SendToNClosestNode(transport::protobuf::RoutingMessage & message, int n);

    bool CloserToTarget(const std::string & id1, const std::string & id2, const std::string & target_id);
    bool HasNode(NodeInfoPtr node);
    NodeInfoPtr FindLocalNode(const std::string node_id);
    void FindCloseNodesWithEndpoint(const std::string & des_node_id, const std::pair<std::string, uint16_t> & boot_endpoints);

    std::string bootstrap_id() {
        return bootstrap_id_;
    }
    void set_bootstrap_id(const std::string & id) {
        bootstrap_id_ = id;
    }
    std::string bootstrap_ip() {
        return bootstrap_ip_;
    }
    void set_bootstrap_ip(const std::string & ip) {
        bootstrap_ip_ = ip;
    }
    uint16_t bootstrap_port() {
        return bootstrap_port_;
    }
    void set_bootstrap_port(uint16_t port) {
        bootstrap_port_ = port;
    }
    

private:
    std::shared_ptr<transport::Transport> transport_ptr_;
    std::shared_ptr<LocalNodeInfo> local_node_ptr_;

protected:
     int Bootstrap(const std::string & peer_ip, uint16_t peer_port, base::ServiceType des_service_type);
     int SendFindClosestNodes(const NodeInfoPtr & node_ptr, int count, const std::vector<NodeInfoPtr> & nodes, base::ServiceType des_service_type);

    bool SetJoin(const std::string & boot_id, const std::string & boot_ip, int boot_port);
    // -1: all bits equal(and return kKadFailed)
    // 0: all bits equal expect the last bit
    // 1: all bits equal expect the last second bit
    // 8*kNodeIdSize-1: the first bit is different already
    int SetNodeBucket(NodeInfoPtr node);
    bool ValidNode(NodeInfoPtr node);
    int SortNodesByTargetXid(const std::string & target_xid, int number);
    // int SortNodesByTargetXip(const std::string & target_xip, int number);
    // make sure nodes_ is sorted by kad algo
     bool NewNodeReplaceOldNode(NodeInfoPtr node, bool remove);
     uint32_t GetFindNodesMaxSize();
    void RecursiveSend(transport::protobuf::RoutingMessage & message, int retry_times);
    void HeartbeatProc();

    void FindNeighbours();
    void GetExistsNodesBloomfilter(const std::vector<NodeInfoPtr> & nodes, std::vector<uint64_t> & bloomfilter_vec);

    // void TellNeighborsDropAllNode();
    // void SendDropNodeRequest(const std::string & id);
    void OnHeartbeatFailed(const std::string & ip, uint16_t port);
    void GetRandomAlphaNodes(std::map<std::string, NodeInfoPtr> & query_nodes);
    void GetClosestAlphaNodes(std::map<std::string, NodeInfoPtr> & query_nodes);
    void DumpNodes();
    void SetFindNeighbourIntervalIncr();
    void SetFindNeighbourIntervalDecr();
    void SetFindNeighbourIntervalMin();
    void SetFindNeighbourIntervalMax();
    // if > mid: to mid, or keep the interval
    void SetFindNeighbourIntervalKeepOrMid();
    std::vector<NodeInfoPtr>::iterator EraseNode(std::vector<NodeInfoPtr>::iterator it);

    bool is_elect_routing_table_;

    uint32_t RoutingMaxNodesSize_;
    
    std::string name_;
    std::vector<NodeInfoPtr> nodes_;
    using NodesMutex = std::recursive_mutex;
    // using NodesMutex = std::mutex;
    using NodesLock = std::unique_lock<NodesMutex>;
    NodesMutex nodes_mutex_;
    std::map<std::string, NodeInfoPtr> node_id_map_;
    std::mutex node_id_map_mutex_;
    std::shared_ptr<std::map<uint64_t, NodeInfoPtr>> node_hash_map_;
    std::mutex node_hash_map_mutex_;
    std::mutex bootstrap_mutex_;
    std::condition_variable bootstrap_cond_;

    std::set<std::string> m_endpoint_ip_port;

    std::mutex joined_mutex_;
    std::atomic<bool> joined_;
    std::string bootstrap_id_;
    std::string bootstrap_ip_;
    uint16_t bootstrap_port_;
    // keep the first bootstrap id
    std::vector<NodeInfoPtr> bootstrap_nodes_;
    std::mutex bootstrap_nodes_mutex_;

    int find_neighbour_interval_threshold_{1};  // 3 * 1/2/4/8/16
    int find_neighbour_interval_{0};
    std::atomic<int> nodes_delta_{0};
    std::shared_ptr<NodeDetectionManager> node_detection_ptr_;
    base::TimerManager * timer_manager_{base::TimerManager::Instance()};
    std::shared_ptr<base::TimerRepeated> timer_rejoin_;
    std::shared_ptr<base::TimerRepeated> timer_find_neighbours_;
    std::shared_ptr<base::TimerRepeated> timer_heartbeat_;
    std::shared_ptr<base::TimerRepeated> timer_heartbeat_check_;
    std::shared_ptr<base::TimerRepeated> timer_prt_;
    bool destroy_;


    uint32_t kadmlia_key_len_;

public:
    void FindElectionNodesInfo(std::map<std::string, base::KadmliaKeyPtr> const & kad_keys, std::map<std::string, kadmlia::NodeInfoPtr> & nodes);

private:
    std::mutex use_nodes_mutex_;
    std::shared_ptr<std::vector<NodeInfoPtr>> no_lock_for_use_nodes_{nullptr};

    DISALLOW_COPY_AND_ASSIGN(RootRoutingTable);
};  // class RootRoutingTable

typedef std::shared_ptr<RootRoutingTable> RootRoutingTablePtr;

}  // namespace kadmlia

}  // namespace top
