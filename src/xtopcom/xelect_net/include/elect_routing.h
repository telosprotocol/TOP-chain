//
//  ec_routing.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <vector>

#include "xpbase/base/top_config.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/top_timer.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/node_info.h"
#include "xwrouter/root/root_message_handler.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"
#include "xwrouter/wrouter_utils/wrouter_base_routing.h"
#include "xelect_net/include/elect_uitils.h"

namespace top {

namespace kadmlia {
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
};

namespace elect {

class EcVnetworkDriver;

class ElectRouting : public wrouter::WrouterBaseRouting {
public:
    ElectRouting(
            std::shared_ptr<transport::Transport> transport,
            kadmlia::LocalNodeInfoPtr local_node_ptr,
            const uint32_t RoutingMaxNodesSize);
    virtual ~ElectRouting() override;
    /**
     * @brief init kademlia routing table
     * 
     * @return true 
     * @return false 
     */
    virtual bool Init() override;
    /**
     * @brief Uninit kademlia routing table
     * 
     * @return true 
     * @return false 
     */
    virtual bool UnInit() override;
    /**
     * @brief set bootstrap nodes of p2p network
     * 
     * @param boot_id bootstrap node kademlia id
     * @param boot_ip bootstrap node ip
     * @param boot_port bootstrap node port
     * @return true 
     * @return false 
     */
    virtual bool SetJoin(const std::string& boot_id, const std::string& boot_ip, int boot_port);
    /**
     * @brief add node to routing table
     * 
     * @param node node is a struct contains some info of a node, such as id, ip , port...
     * @return int 
     */
    virtual int AddNode(kadmlia::NodeInfoPtr node) override;
    /**
     * @brief drop node from routing table
     * 
     * @param node node to drop
     * @return int 
     */
    virtual int DropNode(kadmlia::NodeInfoPtr node) override;
    /**
     * @brief check elect data contains this node id
     * 
     * @param node_id kademlia node id
     * @return int 
     */
    int CheckElectHasNode(const std::string& node_id);

protected:
    virtual bool NewNodeReplaceOldNode(kadmlia::NodeInfoPtr node, bool remove) override;
    virtual uint32_t GetFindNodesMaxSize() override;

private:

    uint32_t RoutingMaxNodesSize_;

    DISALLOW_COPY_AND_ASSIGN(ElectRouting);
};

}  // namespace elect

}  // namespace top
