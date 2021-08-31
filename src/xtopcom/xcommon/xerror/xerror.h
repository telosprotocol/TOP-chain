// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

NS_BEG3(top, common, error)

enum class xenum_errc {
    ok,
    invalid_rotation_status,
};
using xerrc = xenum_errc;

std::error_code make_error_code(xerrc const errc) noexcept;
std::error_condition make_error_condition(xerrc const errc) noexcept;

std::error_category const & common_category() noexcept;

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::common::error::xerrc> final {
    size_t operator()(top::common::error::xerrc const errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::common::error::xerrc> : true_type {};

template <>
struct is_error_condition_enum<top::common::error::xerrc> : true_type {};

NS_END1
