//
//  manager_base.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/xip_parser.h"
#include "xtransport/transport.h"

namespace top {

namespace kadmlia {
    class RoutingTable;
}

namespace elect {


class NodeManagerBase {
public:
    virtual int Init() {
        return 0;
    }

    /**
     * @brief join elect p2p network
     * 
     * @param xip xip is similar to network type
     * @return int 
     */
    virtual int Join(const base::XipParser& xip) = 0;
    /**
     * @brief quit elect p2p network
     * 
     * @param xip xip is similar to network type
     * @return int 
     */
    virtual int Quit(const base::XipParser& xip) = 0;
    /**
     * @brief drop nodes of network
     * 
     * @param xip xip is similar to network typ
     * @param drop_accounts list of accounts
     * @return int 
     */
    virtual int DropNodes(const base::XipParser& xip, const std::vector<std::string>& drop_accounts) = 0;

protected:
    NodeManagerBase(std::shared_ptr<transport::Transport>, const base::Config&);
    virtual ~NodeManagerBase();

    std::shared_ptr<transport::Transport> transport_{ nullptr };
    base::Config base_config_;

private:

    DISALLOW_COPY_AND_ASSIGN(NodeManagerBase);
};

}  // namespace elect

}  // namespace top
