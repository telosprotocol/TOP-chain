//
//  ec_manager.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xelect_net/include/routing_manager.h"

#include <set>

#include "xpbase/base/top_log.h"
#include "xpbase/base/endpoint_util.h"
#include "xkad/routing_table/nodeid_utils.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xelect_net/include/elect_routing.h"

namespace top {

namespace elect {

RoutingManager::RoutingManager() {}

RoutingManager::RoutingManager(uint64_t service_type) : RoutingManagerBase(service_type) {
}

RoutingManager::~RoutingManager() {}

int RoutingManager::Init(
        base::KadmliaKeyPtr kad_key,
        std::shared_ptr<transport::Transport> transport,
        const top::base::Config& config) {
    return RoutingManagerBase::TemplateInit<ElectRouting>(
            kad_key,
            transport,
            config);
}

}  // namespase elect

}  // namespace top
