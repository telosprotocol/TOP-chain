#pragma once
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state.h"

#include <string>

using top::observer_ptr;
using top::contract_common::xcontract_state_t;
struct erc20_params {
    observer_ptr<xcontract_state_t> contract_state;
    std::string code;

    std::string account_from;
    std::string account_to;
    int value;
    erc20_params(std::string _f, std::string _t, int _v) : account_from{_f}, account_to{_t}, value{_v} {
    }
    erc20_params(observer_ptr<xcontract_state_t> _contract_state, std::string _code) : contract_state{_contract_state}, code{_code} {
    }
};