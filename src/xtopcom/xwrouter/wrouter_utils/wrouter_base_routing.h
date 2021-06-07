// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#pragma once


#pragma once

#include <vector>
#include <string>
#include <memory>
#include <thread>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"
#include "xkad/routing_table/root_routing_table.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

namespace top {

namespace wrouter {

class  WrouterBaseRouting : public kadmlia::RootRoutingTable {
public:
   WrouterBaseRouting(
           std::shared_ptr<transport::Transport> transport,
           kadmlia::LocalNodeInfoPtr local_node_ptr)
           : kadmlia::RootRoutingTable(transport, local_node_ptr) {}
    virtual ~WrouterBaseRouting() {}

protected:
    DISALLOW_COPY_AND_ASSIGN(WrouterBaseRouting);
};

}  // namespace wrouter

}  // namespace top
#endif