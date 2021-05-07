// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xbyte_buffer.h"
#include "xcontract_common/xcontract_state.h"

#include <string>

using top::observer_ptr;
using top::contract_common::xcontract_state_t;
struct erc20_params {
    observer_ptr<xcontract_state_t> contract_state;
    std::string code;
    std::string symbol;
    std::string total_supply;

    std::vector<top::xbyte_buffer_t> const call_param;

    erc20_params(observer_ptr<xcontract_state_t> contract_state, std::string const& code, std::string const& symbol, std::string const& total_supply):
                contract_state(contract_state), code(code), symbol(symbol), total_supply(total_supply){
    }
    erc20_params(observer_ptr<xcontract_state_t> contract_state, std::vector<top::xbyte_buffer_t> const param): contract_state(contract_state), call_param(param){}

    erc20_params(observer_ptr<xcontract_state_t> _contract_state, std::string _code) : contract_state{_contract_state}, code{_code} {
    }
};