// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, zec)

class xtop_zec_standby_pool_contract final : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_zec_standby_pool_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_zec_standby_pool_contract);

    explicit
    xtop_zec_standby_pool_contract(common::xnetwork_id_t const & network_id);

    xcontract_base *
    clone() override;

    void
    setup() override;

    BEGIN_CONTRACT_WITH_PARAM(xtop_zec_standby_pool_contract)
    CONTRACT_FUNCTION_PARAM(xtop_zec_standby_pool_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
    void on_timer(common::xlogic_time_t const current_time);
};
using xzec_standby_pool_contract_t = xtop_zec_standby_pool_contract;

NS_END4
