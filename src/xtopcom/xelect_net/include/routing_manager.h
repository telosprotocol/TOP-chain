// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xkad/routing_table/routing_table.h"
#include "xpbase/base/top_config.h"

#include <memory>

namespace top {

namespace kadmlia {
class RoutingTable;
class Transport;
}  // namespace kadmlia

namespace elect {

class RoutingManager {
public:
    RoutingManager();
    explicit RoutingManager(base::ServiceType service_type);
    ~RoutingManager();
    /**
     * @brief init RoutingManager
     *
     * @param kad_key kademlia id of routing table to be created
     * @param transport transport is instance of socket
     * @param config config struct
     * @return int
     */
    int Init(base::KadmliaKeyPtr kad_key, std::shared_ptr<transport::Transport> transport, const top::base::Config & config);

private:
    std::shared_ptr<top::kadmlia::RoutingTable> CreateRoutingTable(base::KadmliaKeyPtr kad_key, std::shared_ptr<transport::Transport> transport, const top::base::Config & config);

    base::ServiceType service_type_;
    std::shared_ptr<top::kadmlia::RoutingTable> routing_ptr_;

    DISALLOW_COPY_AND_ASSIGN(RoutingManager);
};

}  // namespace elect

}  // namespace top
