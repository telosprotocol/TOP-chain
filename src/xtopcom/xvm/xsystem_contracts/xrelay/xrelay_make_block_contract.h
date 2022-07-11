// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, relay)

class xtop_relay_make_block_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_relay_make_block_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_relay_make_block_contract);

    explicit xtop_relay_make_block_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override {
        return new xtop_relay_make_block_contract(network_id());
    }

    void setup();

    BEGIN_CONTRACT_WITH_PARAM(xtop_relay_make_block_contract)
    CONTRACT_FUNCTION_PARAM(xtop_relay_make_block_contract, on_receive_cross_txs);
    CONTRACT_FUNCTION_PARAM(xtop_relay_make_block_contract, on_make_block);
    END_CONTRACT_WITH_PARAM

private:
    void on_receive_cross_txs(std::string const & cross_txs_data);
    void on_make_block(std::string const & make_block_info);
};
using xrelay_make_block_contract_t = xtop_relay_make_block_contract;

NS_END4
