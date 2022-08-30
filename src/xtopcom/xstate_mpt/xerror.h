// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

namespace top {
namespace state_mpt {
namespace error {

enum class xenum_errc {
    ok,

    mpt_get_error,
    mpt_set_error,
    mpt_delete_error,
    mpt_not_found,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & state_mpt_category();

}  // namespace error
}  // namespace state_mpt
}  // namespace top

namespace std {

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::state_mpt::error::xerrc_t> final {
    size_t operator()(top::state_mpt::error::xerrc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::state_mpt::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::state_mpt::error::xerrc_t> : std::true_type {};

}  // namespace std
