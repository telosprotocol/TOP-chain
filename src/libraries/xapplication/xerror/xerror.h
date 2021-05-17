// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <system_error>
#include <iosfwd>
#include <type_traits>

#include "xbase/xns_macro.h"

namespace top {
namespace application {
namespace error {

enum class xenum_errc {
    successful,
    load_election_data_missing_block,
    load_election_data_block_type_mismatch,
    load_election_data_property_empty,
    load_election_data_missing_property,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & application_category();

}
}
}

namespace std {

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::application::error::xerrc_t> final {
    size_t operator()(top::application::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::application::error::xerrc_t> : std::true_type {
};

template <>
struct is_error_condition_enum<top::application::error::xerrc_t> : std::true_type {
};

}
