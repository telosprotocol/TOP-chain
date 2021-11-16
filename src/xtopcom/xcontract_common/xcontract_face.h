// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcommon/xsymbol.h"
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

    virtual void register_property(properties::xbasic_property_t * property) = 0;

    virtual common::xnetwork_id_t network_id() const = 0;
    virtual uint64_t balance() const = 0;
    virtual state_accessor::xtoken_t withdraw(std::uint64_t amount) = 0;
    virtual void deposit(state_accessor::xtoken_t token) = 0;

    virtual void reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) = 0;

    virtual observer_ptr<xcontract_state_t> contract_state() const noexcept = 0;

protected:
    xtop_contract_face() = default;

    virtual xbyte_buffer_t receipt_data(std::string const & key) const = 0;
    // virtual state_accessor::xtoken_t retrive_received_asset(std::string const & asset_id) const = 0;
    virtual void write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) = 0;
    virtual void call(common::xaccount_address_t const & target_addr,
                      std::string const & method_name,
                      std::string const & method_params,
                      xfollowup_transaction_schedule_type_t type) = 0;
    virtual void call(common::xaccount_address_t const & target_addr,
                      std::string const & source_method_name,
                      std::string const & source_method_params,
                      std::string const & target_method_name,
                      std::string const & target_method_params,
                      xfollowup_transaction_schedule_type_t type) = 0;
};

NS_END2
