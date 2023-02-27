// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

namespace top {
namespace gasfee {
namespace error {

enum class xenum_errc {
    ok,

    account_withdraw_failed,
    account_deposit_failed,
    account_property_set_failed,
    account_balance_not_enough,
    tx_deposit_not_enough,
    tx_out_of_gas,
    tx_priority_fee_error,
    invalid_param,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & gasfee_category();

}
}
}

namespace std {

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::gasfee::error::xerrc_t> final {
    size_t operator()(top::gasfee::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::gasfee::error::xerrc_t> : std::true_type {
};

template <>
struct is_error_condition_enum<top::gasfee::error::xerrc_t> : std::true_type {
};

}
