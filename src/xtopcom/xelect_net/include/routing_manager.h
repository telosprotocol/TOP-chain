//
//  ec_manager.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <memory>

#include "xpbase/base/top_config.h"
#include "xelect_net/include/routing_manager_base.h"
#include "xelect_net/include/elect_routing.h"

namespace top {

namespace kadmlia {
class RoutingTable;
class Transport;
}

namespace elect {
    
class RoutingManager : public RoutingManagerBase {
public:
    RoutingManager();
    explicit RoutingManager(uint64_t service_type);
    ~RoutingManager();
    /**
     * @brief init RoutingManager
     * 
     * @param kad_key kademlia id of routing table to be created
     * @param transport transport is instance of socket
     * @param config config struct
     * @return int 
     */
    virtual int Init(
            base::KadmliaKeyPtr kad_key,
            std::shared_ptr<transport::Transport> transport,
            const top::base::Config& config);

private:
    DISALLOW_COPY_AND_ASSIGN(RoutingManager);
};

}  // namespace elect

}  // namespace top

