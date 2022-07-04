// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>

NS_BEG3(top, xunit_service, error)

enum class xenum_errc {
    ok,
    unknown_error,

    vnode_already_exist,
    vnode_not_exist,
    serialization_error,
    packer_cert_block_invalid,
    packer_view_behind,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const errc) noexcept;
std::error_condition make_error_condition(xerrc_t const errc) noexcept;

std::error_category const & unit_service_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::xunit_service::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::xunit_service::error::xerrc_t> : std::true_type {};

NS_END1
