// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xfollowup_transaction_datum.h"

namespace top {
namespace contract_common {

xtop_followup_transaction_datum::xtop_followup_transaction_datum(data::xcons_transaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type) : followed_transaction(std::move(tx)), schedule_type(type) {
}

}
}
