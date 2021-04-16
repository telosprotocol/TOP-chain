// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xtop_action.h"

#include <memory>
#include <vector>

NS_BEG2(top, contract_runtime)

class xtop_contract_action_generator {
public:
    static data::xtop_action_t generate(xobject_ptr_t<data::xcons_transaction_t> const & tx);
    static std::vector<data::xtop_action_t> generate(std::vector<xobject_ptr_t<data::xcons_transaction_t>> const & txs);
};
using xcontract_action_generator_t = xtop_contract_action_generator;

NS_END2
