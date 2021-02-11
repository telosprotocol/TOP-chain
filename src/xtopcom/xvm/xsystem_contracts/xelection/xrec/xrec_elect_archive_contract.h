// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xblock.h"
#include "xdata/xdata_common.h"
#include "xvm/xcontract/xcontract_register.h"
#include "xvm/xsystem_contracts/xelection/xelect_nonconsensus_group_contract.h"

NS_BEG4(top, xvm, system_contracts, rec)

class xtop_rec_elect_archive_contract final : public xvm::system_contracts::xelect_nonconsensus_group_contract_t {
    using xbase_t = xvm::system_contracts::xelect_nonconsensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_rec_elect_archive_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_rec_elect_archive_contract);

    explicit xtop_rec_elect_archive_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override { return new xtop_rec_elect_archive_contract(network_id()); }

    void setup() override;

    void on_timer(const uint64_t current_time);
#ifdef STATIC_CONSENSUS
    void elect_config_nodes(common::xlogic_time_t const current_time);
#endif
    BEGIN_CONTRACT_WITH_PARAM(xtop_rec_elect_archive_contract)
    CONTRACT_FUNCTION_PARAM(xtop_rec_elect_archive_contract, on_timer);
    END_CONTRACT_WITH_PARAM
};
using xrec_elect_archive_contract_t = xtop_rec_elect_archive_contract;

NS_END4
