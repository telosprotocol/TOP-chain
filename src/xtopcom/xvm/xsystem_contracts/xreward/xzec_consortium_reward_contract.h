// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtableblock.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"

NS_BEG2(top, xstake)

using namespace xvm;
using namespace xvm::xcontract;


struct xreward_cons_property_param_t {
    std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> map_node_reward_detail;
    data::system_contract::xaccumulated_reward_record accumulated_reward_record;
    std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> map_nodes;
};

class xzec_consortium_reward_contract : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_consortium_reward_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_consortium_reward_contract);

    explicit
    xzec_consortium_reward_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*  clone() override {return new xzec_consortium_reward_contract(network_id());}



    BEGIN_CONTRACT_WITH_PARAM(xzec_consortium_reward_contract)
        CONTRACT_FUNCTION_PARAM(xzec_consortium_reward_contract, on_timer);
        CONTRACT_FUNCTION_PARAM(xzec_consortium_reward_contract, calculate_reward);
    END_CONTRACT_WITH_PARAM

private:
   
};

NS_END2
