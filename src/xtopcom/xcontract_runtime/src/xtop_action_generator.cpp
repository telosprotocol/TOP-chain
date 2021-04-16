// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xtop_action_generator.h"

#include "xdata/xtransaction_action.h"

#include <cassert>

NS_BEG2(top, contract_runtime)

data::xtop_action_t xtop_contract_action_generator::generate(xobject_ptr_t<data::xcons_transaction_t> const & tx) {
    return data::xtransaction_action_t{ tx };
}

std::vector<data::xtop_action_t> xtop_contract_action_generator::generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs) {
    std::vector<data::xtop_action_t> r;
    r.reserve(txs.size());

    for (auto const & tx : txs) {
        r.push_back(generate(tx));
    }
    return r;
}

NS_END2
