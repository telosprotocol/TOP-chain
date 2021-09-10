// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xtop_action_generator.h"

#include "xbasic/xmemory.hpp"
#include "xdata/xconsensus_action.h"
#include "xcommon/xaddress.h"

#include <cassert>

NS_BEG2(top, contract_runtime)

std::unique_ptr<data::xbasic_top_action_t const> xtop_action_generator::generate(xobject_ptr_t<data::xcons_transaction_t> const & tx) {
    common::xaccount_address_t const target_address{ tx->get_transaction()->get_target_addr() };
    switch (target_address.type()) {
    case base::enum_vaccount_addr_type_native_contract:
        return top::make_unique<data::xsystem_consensus_action_t>(tx);

    case base::enum_vaccount_addr_type_custom_contract:
        return top::make_unique<data::xuser_consensus_action_t>(tx);

    default:
        assert(false);
        return nullptr;
    }
}

std::vector<std::unique_ptr<data::xbasic_top_action_t const>> xtop_action_generator::generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs) {
    std::vector<std::unique_ptr<data::xbasic_top_action_t const>> r;
    r.reserve(txs.size());

    for (auto const & tx : txs) {
        r.push_back(generate(tx));
    }
    return r;
}

NS_END2
