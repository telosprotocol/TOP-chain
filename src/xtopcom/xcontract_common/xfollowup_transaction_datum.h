#pragma once

#include "xbasic/xns_macro.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, contract_common)

enum class xenum_followup_transaction_schedule_type {
    invalid,
    immediately,
    normal
};
using xfollowup_transaction_schedule_type_t = xenum_followup_transaction_schedule_type;

struct xtop_followup_transaction_datum {
    xtop_followup_transaction_datum(data::xtransaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type) : followed_transaction(std::move(tx)), schedule_type(type) {
   
    }

    data::xtransaction_ptr_t followed_transaction{nullptr};
    xfollowup_transaction_schedule_type_t schedule_type{xfollowup_transaction_schedule_type_t::invalid};
};
using xfollowup_transaction_datum_t = xtop_followup_transaction_datum;

NS_END2
