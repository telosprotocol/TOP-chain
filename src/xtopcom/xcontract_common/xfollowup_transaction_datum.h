// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, contract_common)

enum class xenum_followup_transaction_schedule_type {
    invalid,
    immediately,
    normal
};
using xfollowup_transaction_schedule_type_t = xenum_followup_transaction_schedule_type;

struct xtop_followup_transaction_datum {
    xtop_followup_transaction_datum() = default;
    xtop_followup_transaction_datum(xtop_followup_transaction_datum const &) = default;
    xtop_followup_transaction_datum & operator=(xtop_followup_transaction_datum const &) = default;
    xtop_followup_transaction_datum(xtop_followup_transaction_datum &&) = default;
    xtop_followup_transaction_datum & operator=(xtop_followup_transaction_datum &&) = default;
    ~xtop_followup_transaction_datum() = default;

    xtop_followup_transaction_datum(data::xtransaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type);

    data::xtransaction_ptr_t followed_transaction{nullptr};
    xfollowup_transaction_schedule_type_t schedule_type{xfollowup_transaction_schedule_type_t::invalid};
};
using xfollowup_transaction_datum_t = xtop_followup_transaction_datum;

NS_END2
