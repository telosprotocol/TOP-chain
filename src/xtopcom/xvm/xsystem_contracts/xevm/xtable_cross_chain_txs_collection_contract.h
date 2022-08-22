// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xblock_statistics_data.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtableblock.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"

NS_BEG3(top, xvm, system_contracts)

XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT = "@1";  // TODO(jimmy)
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME = "@2";  // TODO(jimmy)

class xtable_cross_chain_txs_collection_contract : public xcontract::xcontract_base
{
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtable_cross_chain_txs_collection_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtable_cross_chain_txs_collection_contract);

    explicit xtable_cross_chain_txs_collection_contract(common::xnetwork_id_t const &network_id);

    xcontract_base *clone() override { return new xtable_cross_chain_txs_collection_contract(network_id()); }

    /**
     * @brief setup the contract
     *
     */
    void setup();

    /**
     * @brief call zec reward contract to calculate reward
     *
     * @param timestamp the time to call
     */
    void on_timer(common::xlogic_time_t const timestamp);

    BEGIN_CONTRACT_WITH_PARAM(xtable_cross_chain_txs_collection_contract)
    CONTRACT_FUNCTION_PARAM(xtable_cross_chain_txs_collection_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:

};

using xtable_cross_chain_txs_collection_contract_t = xtable_cross_chain_txs_collection_contract;

NS_END3
