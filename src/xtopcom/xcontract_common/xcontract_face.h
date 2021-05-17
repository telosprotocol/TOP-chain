// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xfollowup_transaction_datum.h"

#include <system_error>

NS_BEG2(top, contract_common)

class xtop_contract_face {
public:
    xtop_contract_face(xtop_contract_face const &) = delete;
    xtop_contract_face & operator=(xtop_contract_face const &) = delete;
    xtop_contract_face(xtop_contract_face &&) = default;
    xtop_contract_face & operator=(xtop_contract_face &&) = default;
    virtual ~xtop_contract_face() = default;

    virtual xcontract_execution_result_t execute(observer_ptr<xcontract_execution_context_t> exe_ctx) = 0;

    virtual common::xaccount_address_t sender() const = 0;
    virtual common::xaccount_address_t recver() const = 0;
    virtual common::xaccount_address_t address() const = 0;

protected:
    xtop_contract_face() = default;

    virtual void reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) = 0;
    virtual bool at_source_action_stage() const noexcept = 0;
    virtual bool at_target_action_stage() const noexcept = 0;
    virtual bool at_confirm_action_stage() const noexcept = 0;

    virtual xbyte_buffer_t const & receipt_data(std::string const & key, std::error_code & ec) const = 0;
    virtual void write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) = 0;
    virtual void call(common::xaccount_address_t const & target_addr,
                      std::string const & method_name,
                      std::string const & method_params,
                      xfollowup_transaction_schedule_type_t type) = 0;
};

NS_END2
