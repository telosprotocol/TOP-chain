// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xcommon/xaccount_address.h"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, contract_common)

enum class xenum_followup_transaction_schedule_type {
    invalid,
    immediately,
    delay
};
using xfollowup_transaction_schedule_type_t = xenum_followup_transaction_schedule_type;

enum class xenum_followup_transaction_execute_type {
    unexecuted,
    success,
    fail
};
using xfollowup_transaction_execute_type_t = xenum_followup_transaction_execute_type;

enum class xenum_delay_followup_type {
    invalid,
    call,
    transfer,
};
using xdelay_followup_type_t = xenum_delay_followup_type;

struct xtop_followup_transaction_delay_param {
    xdelay_followup_type_t type;
    uint64_t time;
    common::xaccount_address_t target_address;
    std::string method_name;
    std::string method_params;

    xtop_followup_transaction_delay_param() = default;
    xtop_followup_transaction_delay_param(xdelay_followup_type_t type_,
                                          uint64_t time_,
                                          common::xaccount_address_t const & addr_,
                                          std::string const & method_name_,
                                          std::string const & method_params_)
      : type(type_), time(time_), target_address(addr_), method_name(method_name_), method_params(method_params_) {
    }
};
using xfollowup_transaction_delay_param_t = xtop_followup_transaction_delay_param;

struct xtop_followup_transaction_datum {
    xtop_followup_transaction_datum() = default;
    xtop_followup_transaction_datum(xtop_followup_transaction_datum const &) = default;
    xtop_followup_transaction_datum & operator=(xtop_followup_transaction_datum const &) = default;
    xtop_followup_transaction_datum(xtop_followup_transaction_datum &&) = default;
    xtop_followup_transaction_datum & operator=(xtop_followup_transaction_datum &&) = default;
    ~xtop_followup_transaction_datum() = default;

    xtop_followup_transaction_datum(data::xcons_transaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type);

    data::xcons_transaction_ptr_t followed_transaction{nullptr};
    xfollowup_transaction_schedule_type_t schedule_type{xfollowup_transaction_schedule_type_t::invalid};
    xfollowup_transaction_execute_type_t execute_type{xfollowup_transaction_execute_type_t::unexecuted};
};
using xfollowup_transaction_datum_t = xtop_followup_transaction_datum;

NS_END2
