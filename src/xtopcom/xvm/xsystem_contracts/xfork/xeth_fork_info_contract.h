// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, fork)

class xtop_eth_fork_info_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_eth_fork_info_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_eth_fork_info_contract);

    explicit xtop_eth_fork_info_contract(common::xnetwork_id_t const & network_id);

    /**
     * @brief setup the contract
     *
     */
    void setup() override;

    xcontract::xcontract_base * clone() override {
        return new xtop_eth_fork_info_contract(this->m_network_id);
    }

    void on_timer(common::xlogic_time_t const trigger_time);

    BEGIN_CONTRACT_WITH_PARAM(xtop_eth_fork_info_contract)
    CONTRACT_FUNCTION_PARAM(xtop_eth_fork_info_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
};

using xeth_fork_info_contract_t = xtop_eth_fork_info_contract;

NS_END4