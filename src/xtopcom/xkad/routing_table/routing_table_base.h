// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#if 0
#include "xbase/xbase.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport.h"

#include <memory>
#include <string>
#include <vector>

namespace top {

namespace kadmlia {

class NodeDetectionManager;
class LocalNodeInfo;

class RoutingTableBase {
public:
    RoutingTableBase() = delete;
    RoutingTableBase(std::shared_ptr<transport::Transport> transport_ptr, std::shared_ptr<LocalNodeInfo> local_node_ptr);
    RoutingTableBase(RoutingTableBase const &) = delete;
    RoutingTableBase & operator=(RoutingTableBase const &) = delete;
    RoutingTableBase(RoutingTableBase &&) = default;
    RoutingTableBase & operator=(RoutingTableBase &&) = default;
    // todo check here!
    virtual ~RoutingTableBase() {
    }

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
    virtual bool Init() = 0;
    virtual bool UnInit() = 0;
    virtual int AddNode(NodeInfoPtr node) = 0;
    virtual int DropNode(NodeInfoPtr node) = 0;

    virtual std::size_t nodes_size() = 0;
    virtual std::vector<NodeInfoPtr> GetClosestNodes(const std::string & target_id, uint32_t number_to_get) = 0;


public:
// todo charles
    // tmp will be deleted after new algorithm
    virtual void GetRangeNodes(uint32_t min_index, uint32_t max_index, std::vector<NodeInfoPtr> & vec);

public:
    virtual int SendData(const xbyte_buffer_t & data, const std::string & peer_ip, uint16_t peer_port, uint16_t priority = enum_xpacket_priority_type_priority);
    virtual int SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);
    virtual int SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node_ptr);
    virtual int SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port);

    virtual NodeInfoPtr GetRandomNode() = 0;
    virtual bool GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) = 0;

private:
    std::shared_ptr<transport::Transport> transport_ptr_;
    std::shared_ptr<LocalNodeInfo> local_node_ptr_;
};

typedef std::shared_ptr<RoutingTableBase> RoutingTablePtr;

}  // namespace kadmlia

}  // namespace top
#endif