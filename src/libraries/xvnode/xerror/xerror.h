// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

NS_BEG3(top, vnode, error)

enum class xenum_errc {
    ok,
    vnode_is_not_running,
    invalid_address,
};
using xerrc = xenum_errc;

std::error_code make_error_code(xerrc const errc) noexcept;
std::error_condition make_error_condition(xerrc const errc) noexcept;

std::error_category const & vnode_category() noexcept;

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::vnode::error::xerrc> final {
    size_t operator()(top::vnode::error::xerrc const errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::vnode::error::xerrc> : true_type {};

template <>
struct is_error_condition_enum<top::vnode::error::xerrc> : true_type {};

NS_END1
