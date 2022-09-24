// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

namespace top {
namespace statestore {
namespace error {

enum class xenum_errc {
    ok,

    statestore_extract_state_root_err,
    statestore_load_tableblock_err,
    statestore_load_tableblock_output_err,
    statestore_load_unitblock_err,
    statestore_load_tablestate_err,
    statestore_block_root_unmatch_mpt_root_err,
    statestore_tablestate_exec_fail,
    statestore_block_unmatch_prev_err,
    statestore_try_limit_arrive_err,
    statestore_cannot_execute_for_long_distance_err,
    statestore_db_write_err,
    statestore_db_read_abnormal_err,
    statestore_binlog_apply_err,
    statestore_block_invalid_err,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & statestore_category();

}  // namespace error
}  // namespace statestore
}  // namespace top

namespace std {

template <>
struct is_error_code_enum<top::statestore::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::statestore::error::xerrc_t> : std::true_type {};

}  // namespace std
