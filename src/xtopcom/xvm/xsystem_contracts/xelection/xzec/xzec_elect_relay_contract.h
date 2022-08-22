// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

NS_BEG4(top, xvm, system_contracts, zec)

class xtop_zec_elect_relay_contract final : public xelect_consensus_group_contract_t {
    using xbase_t = xelect_consensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_zec_elect_relay_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_zec_elect_relay_contract);

    explicit xtop_zec_elect_relay_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override { return new xtop_zec_elect_relay_contract(this->m_network_id); }

    void setup() override;

    BEGIN_CONTRACT_WITH_PARAM(xtop_zec_elect_relay_contract)
    CONTRACT_FUNCTION_PARAM(xtop_zec_elect_relay_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
    void on_timer(common::xlogic_time_t const current_time);
};
using xzec_elect_relay_contract_t = xtop_zec_elect_relay_contract;

NS_END4
