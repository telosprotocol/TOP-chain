//
//  routing_manager_base.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <memory>

#include "xpbase/base/top_config.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/chain_kadmlia_key.h"
#include "xtransport/transport.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/multi_routing/multi_routing.h"

namespace top {

namespace elect {

static const int kElectRoutingMaxNodesSize = 256;

class RoutingManagerBase {
public:
    virtual int Init(
            base::KadmliaKeyPtr kad_key,
            std::shared_ptr<transport::Transport> transport,
            const top::base::Config& config) = 0;
    std::shared_ptr<top::kadmlia::RoutingTable> routing_table() {
        return routing_ptr_;
    }

protected:
    template<typename RoutingType>
    int TemplateInit(
            base::KadmliaKeyPtr kad_key,
            std::shared_ptr<transport::Transport> transport,
            const top::base::Config& config);

    RoutingManagerBase() : routing_ptr_(nullptr), service_type_(0) {
    }
    explicit RoutingManagerBase(uint64_t service_type) : routing_ptr_(nullptr), service_type_(service_type) {
    }
    virtual ~RoutingManagerBase() {
        TOP_WARN("unregister routing table: %llu", service_type_);
        wrouter::MultiRouting::Instance()->RemoveRoutingTable(service_type_);
    }

    template<typename RoutingType>
    std::shared_ptr<top::kadmlia::RoutingTable> CreateRoutingTable(
            base::KadmliaKeyPtr kad_key,
            std::shared_ptr<transport::Transport> transport,
            const top::base::Config& config);

    std::shared_ptr<top::kadmlia::RoutingTable> routing_ptr_;
    uint64_t service_type_;

    DISALLOW_COPY_AND_ASSIGN(RoutingManagerBase);
};

template<typename RoutingType>
int RoutingManagerBase::TemplateInit(
        base::KadmliaKeyPtr kad_key,
        std::shared_ptr<transport::Transport> transport,
        const top::base::Config& config) {
    routing_ptr_ = CreateRoutingTable<RoutingType>(
            kad_key,
            transport,
            config);
    if (!routing_ptr_) {
        TOP_KINFO("unregister routing table %llu", kad_key->GetServiceType());
        wrouter::MultiRouting::Instance()->RemoveRoutingTable(kad_key->GetServiceType());
        return top::kadmlia::kKadFailed;
    }

    return top::kadmlia::kKadSuccess;
}

template<typename RoutingType>
std::shared_ptr<top::kadmlia::RoutingTable> RoutingManagerBase::CreateRoutingTable(
        base::KadmliaKeyPtr kad_key,
        std::shared_ptr<transport::Transport> transport,
        const top::base::Config& config) {
    std::shared_ptr<top::kadmlia::RoutingTable> routing_table_ptr;
    kadmlia::LocalNodeInfoPtr local_node_ptr = kadmlia::CreateLocalInfoFromConfig(
            config,
            kad_key);
    std::cout << "ec create routing table: ";
    PrintXip(kad_key->Xip());
    if (!local_node_ptr) {
        TOP_WARN("local_node_ptr invalid");
        return nullptr;
    }
    routing_table_ptr = std::make_shared<RoutingType>(transport, local_node_ptr, kElectRoutingMaxNodesSize);
    if (!routing_table_ptr->Init()) {
        TOP_ERROR("init edge bitvpn routing table failed!");
        return nullptr;
    }
    uint64_t service_type =  kad_key->GetServiceType();
    routing_table_ptr->get_local_node_info()->set_service_type(service_type);
    service_type_ = service_type;
    // wrouter::RegisterRoutingTable(service_type, routing_table_ptr);
    TOP_KINFO("register routing table %llu", service_type);
    wrouter::MultiRouting::Instance()->AddRoutingTable(service_type, routing_table_ptr);
    bool first_node = false;
    std::set<std::pair<std::string, uint16_t>> join_endpoints;
    auto ret = wrouter::NetworkExists(kad_key, join_endpoints);
    TOP_INFO("check routing exists:[%llu], ret: %d, endpoints_size: %d", service_type, ret, join_endpoints.size());
    if (ret != kadmlia::kKadSuccess || join_endpoints.empty()) {
        first_node = true;
    }

    if (first_node) {
        auto root_routing = wrouter::GetRoutingTable(kRoot, true);
        local_node_ptr->set_public_ip(root_routing->get_local_node_info()->public_ip());
        local_node_ptr->set_public_port(root_routing->get_local_node_info()->public_port());
        local_node_ptr->set_first_node(true);
        if (root_routing->get_local_node_info()->public_ip().empty()) {
            TOP_ERROR("RoutingManagerBase local node public ip is empty.");
            assert(false);
        }
        return routing_table_ptr;
    }

    routing_table_ptr->MultiJoinAsync(join_endpoints);
    TOP_INFO("multijoin of service_type: %llu ...", service_type_);
    return routing_table_ptr;
}

}  // namespase elect

}  // namespace top
