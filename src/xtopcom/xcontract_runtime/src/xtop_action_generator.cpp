// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xtop_action_generator.h"

#include "xdata/xconsensus_action.h"
#include "xcommon/xaddress.h"

#include <cassert>

NS_BEG2(top, contract_runtime)

data::xbasic_top_action_t xtop_action_generator::generate(xobject_ptr_t<data::xcons_transaction_t> const & tx) {
    common::xaccount_address_t const target_address{ tx->get_transaction()->get_target_addr() };
    switch (target_address.type()) {
    case base::enum_vaccount_addr_type_native_contract:
        return static_cast<data::xbasic_top_action_t>(data::xconsensus_action_t<data::xtop_action_type_t::system>{tx});

    case base::enum_vaccount_addr_type_custom_contract:
        return static_cast<data::xbasic_top_action_t>(data::xconsensus_action_t<data::xtop_action_type_t::user>{tx});

    default:
        assert(false);
        return data::xbasic_top_action_t{};
    }
}

std::vector<data::xbasic_top_action_t> xtop_action_generator::generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs) {
    std::vector<data::xbasic_top_action_t> r;
    r.reserve(txs.size());

    for (auto const & tx : txs) {
        r.push_back(generate(tx));
    }
    return r;
}

NS_END2
