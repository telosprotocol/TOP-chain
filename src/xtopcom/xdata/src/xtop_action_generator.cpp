// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtop_action_generator.h"

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xdata/xconsensus_action.h"
#include "xdata/xerror/xerror.h"

#include <cassert>

NS_BEG2(top, contract_runtime)

std::unique_ptr<data::xbasic_top_action_t const> xtop_action_generator::generate(xobject_ptr_t<data::xcons_transaction_t> const & tx) {
    std::error_code ec;
    auto r = generate(tx, ec);
    top::error::throw_error(ec);
    return r;
}

std::unique_ptr<data::xbasic_top_action_t const> xtop_action_generator::generate(xobject_ptr_t<data::xcons_transaction_t> const & tx, std::error_code & ec) {
    common::xaccount_address_t const target_address{ tx->get_transaction()->get_target_addr() };
    
    // eth deploy code
    if (target_address.empty()) {
        common::xaccount_address_t const source_address{ tx->get_transaction()->get_source_addr() };
        if (source_address.type() != base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
            ec = data::error::xenum_errc::action_address_type_error;
            assert(false);
            return nullptr;
        }
        return top::make_unique<data::xevm_consensus_action_t>(tx);
    } 

    switch (target_address.type()) {
    case base::enum_vaccount_addr_type_native_contract:
    // just for followup transfer tx
    case base::enum_vaccount_addr_type_secp256k1_user_account:
        return top::make_unique<data::xsystem_consensus_action_t>(tx);

    // T8 address use for tvm (solidity tx)
    case base::enum_vaccount_addr_type_secp256k1_eth_user_account:
    case base::enum_vaccount_addr_type_secp256k1_evm_user_account:
        return top::make_unique<data::xevm_consensus_action_t>(tx);

    default:
        ec = data::error::xenum_errc::action_address_type_error;
        assert(false);
        return nullptr;
    }  
}

std::vector<std::unique_ptr<data::xbasic_top_action_t const>> xtop_action_generator::generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs) {
    std::error_code ec;
    auto r = generate(txs, ec);
    top::error::throw_error(ec);
    return r;
}

std::vector<std::unique_ptr<data::xbasic_top_action_t const>> xtop_action_generator::generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs,
                                                                                              std::error_code & ec) {
    std::vector<std::unique_ptr<data::xbasic_top_action_t const>> r;
    r.reserve(txs.size());

    for (auto const & tx : txs) {
        auto action = generate(tx, ec);
        if (ec) {
            return r;
        }
        r.push_back(std::move(action));
    }
    return r;
}

NS_END2
