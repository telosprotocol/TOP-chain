// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

NS_BEG4(top, xvm, system_contracts, rec)

class xtop_rec_elect_zec_contract final : public xelect_consensus_group_contract_t {
    using xbase_t = xelect_consensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_rec_elect_zec_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_rec_elect_zec_contract);

    explicit xtop_rec_elect_zec_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override { return new xtop_rec_elect_zec_contract(network_id()); }

    void setup();

    void on_timer(common::xlogic_time_t const current_time);
#ifdef STATIC_CONSENSUS
    void elect_config_nodes(common::xlogic_time_t const current_time);
#endif
    BEGIN_CONTRACT_WITH_PARAM(xtop_rec_elect_zec_contract)
    CONTRACT_FUNCTION_PARAM(xtop_rec_elect_zec_contract, on_timer);
    END_CONTRACT_WITH_PARAM
};
using xrec_elect_zec_contract_t = xtop_rec_elect_zec_contract;

NS_END4
