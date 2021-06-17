// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace top {
namespace state_accessor {
namespace error {

enum class xenum_errc : uint16_t {
    ok,
    invalid_state_backend,

    token_insufficient,

    invalid_property_type,
    property_access_denied,
    load_property_failed,
    property_not_exist,
    property_key_not_exist,
    property_key_out_of_range,
    property_already_exist,
    property_value_out_of_range,
    property_name_out_of_range,
    create_property_failed,
    update_property_failed,
    property_not_changed,

    property_id_conversion_invalid,

    // capability errors
    load_account_state_failed,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & state_category();

}
}
}

namespace std {

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::state_accessor::error::xerrc_t> final {
    size_t operator()(top::state_accessor::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::state_accessor::error::xerrc_t> : std::true_type {
};

template <>
struct is_error_condition_enum<top::state_accessor::error::xerrc_t> : std::true_type {
};

}
