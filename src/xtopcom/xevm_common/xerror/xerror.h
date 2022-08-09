// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>

NS_BEG3(top, evm_common, error)

enum class xenum_errc {
    ok,
    not_enough_data,
    abi_data_length_error,
    abi_decode_outofrange,
    abi_data_value_error,
    trie_db_missing_node_error,
    trie_db_not_provided,
    trie_db_not_found,
    rlp_canonint,
    rlp_canonsize,
    rlp_uint_overflow,
    rlp_oversized,
    rlp_value_too_large,
    rlp_expected_string,
    rlp_expected_list,
    trie_proof_missing,
    trie_node_unexpected,
    trie_sync_not_requested,
    trie_sync_already_processed,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const errc) noexcept;
std::error_condition make_error_condition(xerrc_t const errc) noexcept;

std::error_category const & evm_common_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::evm_common::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::evm_common::error::xerrc_t> : std::true_type {};

NS_END1
