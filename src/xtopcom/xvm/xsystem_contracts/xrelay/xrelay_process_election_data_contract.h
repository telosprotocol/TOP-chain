// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, relay)

class xtop_relay_process_election_data_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_relay_process_election_data_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_relay_process_election_data_contract);

    explicit xtop_relay_process_election_data_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override {
        return new xtop_relay_process_election_data_contract(network_id());
    }

    void setup();

    BEGIN_CONTRACT_WITH_PARAM(xtop_relay_process_election_data_contract)
    CONTRACT_FUNCTION_PARAM(xtop_relay_process_election_data_contract, on_recv_election_data);
    END_CONTRACT_WITH_PARAM

private:
    void on_recv_election_data(xbytes_t const & election_data);
};
using xrelay_process_election_data_contract_t = xtop_relay_process_election_data_contract;

NS_END4
