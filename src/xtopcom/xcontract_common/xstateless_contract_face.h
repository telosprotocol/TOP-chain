// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xstateless_contract_execution_context.h"

#include <system_error>

NS_BEG2(top, contract_common)

class xtop_stateless_contract_face {
public:
    xtop_stateless_contract_face(xtop_stateless_contract_face const &) = delete;
    xtop_stateless_contract_face & operator=(xtop_stateless_contract_face const &) = delete;
    xtop_stateless_contract_face(xtop_stateless_contract_face &&) = default;
    xtop_stateless_contract_face & operator=(xtop_stateless_contract_face &&) = default;
    virtual ~xtop_stateless_contract_face() = default;

    virtual xcontract_execution_result_t execute(observer_ptr<xstateless_contract_execution_context_t> exe_ctx) = 0;
    virtual void reset_execution_context(observer_ptr<xstateless_contract_execution_context_t> exe_ctx) = 0;

protected:
    xtop_stateless_contract_face() = default;
};

NS_END2
