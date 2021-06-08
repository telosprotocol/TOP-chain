// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <mutex>
#include <map>

#include "xpbase/base/top_config.h"
#include "xpbase/base/top_timer.h"
#include "xkad/routing_table/routing_utils.h"
// #include "xkad/routing_table/routing_table_base.h"
#include "xkad/routing_table/elect_routing_table.h"
#include "xkad/routing_table/root_routing_table.h"
#include "xwrouter/root/root_routing.h"
#include "xkad/routing_table/elect_routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xkad/proto/kadmlia.pb.h"

namespace top {

namespace wrouter {

class RootRoutingManager;

class MultiRouting {
public:
    static MultiRouting* Instance();
    // kadmlia::RoutingTablePtr GetRoutingTable(base::ServiceType const &service_type,bool is_root);
    kadmlia::ElectRoutingTablePtr GetElectRoutingTable(base::ServiceType const & service_type);
    kadmlia::RootRoutingTablePtr GetRootRoutingTable();
    void AddElectRoutingTable(base::ServiceType service_type, kadmlia::ElectRoutingTablePtr routing_table);
    void RemoveElectRoutingTable(base::ServiceType service_type);

    // kadmlia::RoutingTablePtr GetRoutingTable(const base::ServiceType& type, bool root = false);
    // kadmlia::RoutingTablePtr GetRoutingTable(const std::string& routing_id, bool root = false);

    // void SetRootRoutingManager(std::shared_ptr<RootRoutingManager> root_manager_ptr);

    // friend std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const uint64_t& type, bool root);
    // friend std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const std::string& routing_id, bool root);

    // friend void GetAllRegisterType(std::vector<uint64_t>& vec_type);
    // friend void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>>& vec_rt);


    // friend bool GetServiceBootstrapRootNetwork(
    //     uint64_t service_type,
    //     std::set<std::pair<std::string, uint16_t>>& boot_endpoints);
    // friend bool SetCacheServiceType(uint64_t service_type);
    
    void GetAllRegisterType(std::vector<base::ServiceType>& vec_type);
    void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::ElectRoutingTable>>& vec_rt);

    void CheckElectRoutingTable(base::ServiceType service_type);

public:
    int CreateRootRouting(std::shared_ptr<transport::Transport> transport, const base::Config & config, base::KadmliaKeyPtr kad_key_ptr);

public:
    void HandleRootMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

    void HandleGetElectNodesRequest(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleGetElectNodesResponse(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);


private:
    std::shared_ptr<wrouter::RootRouting> root_routing_table_;
    std::mutex root_routing_table_mutex_;


    // bool GetServiceBootstrapRootNetwork(
    //     uint64_t service_type,
    //     std::set<std::pair<std::string, uint16_t>>& boot_endpoints);
    // bool SetCacheServiceType(uint64_t service_type);
private:
    // kadmlia::RoutingTablePtr GetServiceRoutingTable(const base::ServiceType& type);
    // kadmlia::RoutingTablePtr GetServiceRoutingTable(const std::string& routing_id);

    // void CheckSingleNodeNetwork();
    // void WaitCheckSignal();
    // void NotifyCheckSignal();

    MultiRouting();
    ~MultiRouting();

    std::map<base::ServiceType, kadmlia::ElectRoutingTablePtr> elect_routing_table_map_;
    std::mutex elect_routing_table_map_mutex_;
    // std::shared_ptr<RootRoutingManager> root_manager_ptr_;

    // // CheckSingleNodeNetwork is a block task, so use thread instead of timer
    // std::mutex check_single_network_mutex_;
    // std::condition_variable check_single_network_condv_;
    // base::TimerRepeated timer_{base::TimerManager::Instance(), "MultiRouting"};
    // std::shared_ptr<std::thread> check_single_network_thread_;

    
    base::TimerManager * timer_manager_{base::TimerManager::Instance()};
    std::shared_ptr<base::TimerRepeated> check_elect_routing_;
    void CheckElectRoutingTableTimer();

    using OnCompleteElectRoutingTableCallback = std::function<void(base::ServiceType const, std::string const, kadmlia::NodeInfoPtr const &)>;
    void CompleteElectRoutingTable();
    void OnCompleteElectRoutingTable(base::ServiceType const service_type, std::string const election_xip2, kadmlia::NodeInfoPtr const & node_info);

    DISALLOW_COPY_AND_ASSIGN(MultiRouting);
};

}  // namespace wrouter

}  // namespace top
