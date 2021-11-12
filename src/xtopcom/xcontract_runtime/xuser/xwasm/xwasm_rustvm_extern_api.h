// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xstate_accessor/xstate_accessor.h"

#include <string>

using top::observer_ptr;
using top::state_accessor::xstate_accessor_t;
struct erc20_params {
    observer_ptr<xstate_accessor_t> contract_state;
    std::string code;
    std::string symbol;
    std::string total_supply;

    std::vector<top::xbyte_buffer_t> const call_param;

    erc20_params(observer_ptr<xstate_accessor_t> contract_state, std::string const & code, std::string const & symbol, std::string const & total_supply)
      :
                contract_state(contract_state), code(code), symbol(symbol), total_supply(total_supply){
    }
    erc20_params(observer_ptr<xstate_accessor_t> contract_state, std::vector<top::xbyte_buffer_t> const param) : contract_state(contract_state), call_param(param) {
    }

    erc20_params(observer_ptr<xstate_accessor_t> _contract_state, std::string _code) : contract_state{_contract_state}, code{_code} {
    }
};

struct Erc20_Instance;
extern "C" Erc20_Instance * get_erc20_instance(uint8_t * s, uint32_t size);
extern "C" int32_t depoly_erc20(Erc20_Instance * ins_ptr, erc20_params * ptr);
extern "C" int32_t call_erc20(Erc20_Instance * ins_ptr, erc20_params * ptr);
extern "C" void set_gas_left(Erc20_Instance * inst_ptr, uint64_t gas_limit);
extern "C" uint64_t get_gas_left(Erc20_Instance * ins_ptr);
extern "C" void release_instance(Erc20_Instance * ins_ptr);

static top::xbyte_buffer_t erc20_code = {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60, 0x02, 0x7E, 0x7E, 0x01, 0x7F, 0x02, 0x1D, 0x02,
                                         0x03, 0x65, 0x6E, 0x76, 0x06, 0x63, 0x5F, 0x63, 0x61, 0x6C, 0x6C, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76, 0x08, 0x63, 0x5F,
                                         0x64, 0x65, 0x70, 0x6F, 0x6C, 0x79, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x00, 0x07,
                                         0x11, 0x02, 0x04, 0x63, 0x61, 0x6C, 0x6C, 0x00, 0x02, 0x06, 0x64, 0x65, 0x70, 0x6F, 0x6C, 0x79, 0x00, 0x03, 0x0A, 0x13,
                                         0x02, 0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B, 0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x01, 0x0B};