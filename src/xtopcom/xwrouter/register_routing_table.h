// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/routing_utils.h"
#include "xbase/xpacket.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xwrouter/root/root_routing.h"

namespace top {

namespace kadmlia {
class RoutingTable;
class LocalNodeInfo;
class CallbackManager;
struct NodeInfo;
    
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
}

namespace transport {
class UdpTransport;
}

namespace wrouter {

class RootRoutingManager;

std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const uint64_t& type, bool root = false);
std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const std::string& routing_id, bool root = false);

void GetAllRegisterType(std::vector<uint64_t>& vec_type);
void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>>& vec_rt);

bool SetCacheServiceType(uint64_t service_type);
bool GetServiceBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints);

int NetworkExists(
        base::KadmliaKeyPtr& kad_key_ptr,
        std::set<std::pair<std::string, uint16_t>>& endpoints);
// using elect data to search elect nodes from root network
int GetSameNetworkNodesV2(
        const std::string& des_kroot_id,
        uint64_t des_service_type,
        std::vector<kadmlia::NodeInfoPtr>& ret_nodes);
 

}  // namespace wrouter

}  // namespace top
