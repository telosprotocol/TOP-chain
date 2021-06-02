//
//  manager_base.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xelect_net/include/node_manager_base.h"

#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xtransport/transport.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing_manager.h"

namespace top {

using namespace kadmlia;

namespace elect {

NodeManagerBase::NodeManagerBase(
        std::shared_ptr<transport::Transport> transport,
        const base::Config& config)
        : transport_(transport), base_config_(config) {}

NodeManagerBase::~NodeManagerBase() {}

}  // namespace elect

}  // namespace top
