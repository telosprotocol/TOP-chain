// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcommon.h"
#include "xevm_common/xcrosschain/xbsc/xvote.h"

#include <cstdint>

NS_BEG4(top, evm, crosschain, bsc)

struct xtop_validator_info {
    int32_t index;
    common::xbls_publick_key_t vote_address;
};
using xvalidator_info_t = xtop_validator_info;

class xtop_snapshot {
private:
    uint64_t number_{0};
    xh256_t hash_{};
    std::map<top::common::xeth_address_t, xvalidator_info_t> validators_{};
    std::set<top::common::xeth_address_t> last_validators_{};
    std::map<uint64_t, top::common::xeth_address_t> recents_{};
    std::map<uint64_t, std::string> recent_fork_hashes_{};
    xvote_data_t attestation_{};

public:
    static auto new_snapshot(uint64_t number,
                             xh256_t const & hash,
                             std::vector<top::common::xeth_address_t> const & validators,
                             std::vector<common::xbls_publick_key_t> const & vote_addresses,
                             std::error_code & ec) -> xtop_snapshot;

    auto number() const noexcept -> uint64_t;
    auto hash() const noexcept -> xh256_t const &;
    auto validators() const noexcept -> std::map<top::common::xeth_address_t, xvalidator_info_t> const &;
    auto validators() noexcept -> std::map<top::common::xeth_address_t, xvalidator_info_t> &;
    // auto last_validators() const -> std::set<top::common::xeth_address_t> const &;
    // auto last_validators() -> std::set<top::common::xeth_address_t> &;
    auto recents() const noexcept -> std::map<uint64_t, top::common::xeth_address_t> const &;
    auto recents() noexcept -> std::map<uint64_t, top::common::xeth_address_t> &;
    auto recent_fork_hashes() const noexcept -> std::map<uint64_t, std::string> const &;
    auto recent_fork_hashes() noexcept -> std::map<uint64_t, std::string> &;
    auto attestation() const noexcept -> xvote_data_t const &;

    bool init_with_epoch(evm_common::xeth_header_t const & header);
    bool init_with_double_epoch(evm_common::xeth_header_t const & header1, evm_common::xeth_header_t const & header2);
    xh256_t digest() const;
    bool apply(evm_common::xeth_header_t const & header, bool check);
    bool apply_with_chainid(evm_common::xeth_header_t const & header, evm_common::bigint const & chainid, bool check_inturn);
    bool inturn(uint64_t num, top::common::xeth_address_t const & validator, bool use_old) const;
    void print() const;

    
};
using xsnapshot_t = xtop_snapshot;

NS_END4
