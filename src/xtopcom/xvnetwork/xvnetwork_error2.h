// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include <system_error>

NS_BEG2(top, vnetwork)

enum class xtop_vnetwork_error {
    success = 0,
    vhost_not_run,
    cluster_address_not_match,
    invalid_src_address,
    invalid_dst_address,
    version_mismatch,
    epoch_mismatch = version_mismatch,
    invalid_epoch,
    empty_message,
    future_message,
    expired_message,
    invalid_account_address,
    slot_id_mismatch,
    not_supported,
    vnetwork_driver_not_run,
};
using xvnetwork_errc2_t = xtop_vnetwork_error;

std::error_code
make_error_code(xvnetwork_errc2_t const errc) noexcept;

std::error_condition
make_error_condition(xvnetwork_errc2_t const errc) noexcept;

std::error_category const &
vnetwork_category2();

NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::vnetwork::xvnetwork_errc2_t> : std::true_type {};

NS_END1
