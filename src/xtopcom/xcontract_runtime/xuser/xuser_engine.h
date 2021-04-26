// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_execution_context.h"

NS_BEG3(top, contract_runtime, user)

class xtop_user_engine {
public:
    xtop_user_engine() = default;
    xtop_user_engine(xtop_user_engine const&) = delete;
    xtop_user_engine& operator=(xtop_user_engine const&) = delete;
    xtop_user_engine(xtop_user_engine&&) = default;
    xtop_user_engine& operator=(xtop_user_engine&&) = default;
    ~xtop_user_engine() =  default;

    virtual void deploy_contract(xbyte_buffer_t const& code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) = 0;
    virtual void call_contract(std::string const& func_name, std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) = 0;
};
using xengine_t = xtop_user_engine;

NS_END3
