// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/routing_utils.h"

#ifdef _MSC_VER
#    define _WINSOCKAPI_
#    include <windows.h>
#endif

#include "xbase/xhash.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xkad/routing_table/local_node_info.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"

#include <algorithm>
#include <limits>
#include <locale>

namespace top {

namespace kadmlia {

void GetPublicEndpointsConfig(const top::base::Config &, std::set<std::pair<std::string, uint16_t>> & boot_endpoints) {
    std::string p2p_endpoints = XGET_CONFIG(p2p_endpoints);
    top::base::ParseEndpoints(p2p_endpoints, boot_endpoints);
}

bool CreateGlobalXid(const base::Config & config) try {
    assert(!global_node_id.empty());
    global_xid = base::GetRootKadmliaKey(global_node_id);
    return true;
} catch (...) {
    TOP_FATAL("catch ...");
    return false;
}

LocalNodeInfoPtr CreateLocalInfoFromConfig(const base::Config & config, base::KadmliaKeyPtr kad_key) try {
    std::string local_ip = XGET_CONFIG(ip);
    uint16_t local_port = XGET_CONFIG(node_p2p_port);

    kadmlia::LocalNodeInfoPtr local_node_ptr = nullptr;
    local_node_ptr.reset(new top::kadmlia::LocalNodeInfo());
    if (!local_node_ptr->Init(local_ip, local_port, kad_key)) {
        TOP_ERROR("init local node info failed!");
        return nullptr;
    }

    uint16_t http_port = static_cast<uint16_t>(RandomUint32());
    config.Get("node", "http_port", http_port);
    uint16_t ws_port = static_cast<uint16_t>(RandomUint32());
    config.Get("node", "ws_port", ws_port);
    local_node_ptr->set_rpc_http_port(http_port);
    local_node_ptr->set_rpc_ws_port(ws_port);
    return local_node_ptr;
} catch (std::exception & e) {
    TOP_ERROR("catched error[%s]", e.what());
    return nullptr;
}

}  // namespace kadmlia

}  // namespace top
