#pragma once
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state.h"

#include <string>

using top::observer_ptr;
using top::contract_common::xcontract_state_t;
struct erc20_params {
    observer_ptr<xcontract_state_t> contract_state;
    std::string code;
    std::string token_symbol;
    uint64_t total_supply;

    // depoly
    erc20_params(observer_ptr<xcontract_state_t> _contract_state, std::string const & _code, std::string const & _token_symbol, uint64_t _total_supply)
      : contract_state{_contract_state}, code{_code}, token_symbol{_token_symbol}, total_supply{_total_supply} {
    }
    // call
    // erc20_params()
};