// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xcrosschain/xbsc/xsnapshot.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG4(top, evm, crosschain, bsc)

auto xtop_snapshot::new_snapshot(uint64_t const number,
                                 xh256_t const & hash,
                                 std::vector<top::common::xeth_address_t> const & validators,
                                 std::vector<common::xbls_publick_key_t> const & vote_addresses,
                                 std::error_code & ec) -> xtop_snapshot {
    assert(!ec);

    xtop_snapshot snapshot;
    if (vote_addresses.size() != validators.size()) {
        ec = top::evm_common::error::xerrc_t::invalid_bsc_epoch_data;
        return snapshot;
    }

    
    snapshot.number_ = number;
    snapshot.hash_ = hash;

    size_t i = 0;
    for (; i < validators.size(); ++i) {
        auto const & v = validators[i];

        snapshot.validators_[v].index = 0;
        snapshot.validators_[v].vote_address = vote_addresses[i];
    }

    i = 0;
    for (auto & v : snapshot.validators()) {
        v.second.index = static_cast<int32_t>(++i);
    }

    return snapshot;
}

auto xtop_snapshot::number() const noexcept-> uint64_t {
    return number_;
}

auto xtop_snapshot::hash() const noexcept -> xh256_t const& {
    return hash_;
}

auto xtop_snapshot::validators() const noexcept -> std::map<top::common::xeth_address_t, xvalidator_info_t> const & {
    return validators_;
}

auto xtop_snapshot::validators() noexcept -> std::map<top::common::xeth_address_t, xvalidator_info_t> & {
    return validators_;
}

auto xtop_snapshot::recents() const noexcept -> std::map<uint64_t, top::common::xeth_address_t> const & {
    return recents_;
}

auto xtop_snapshot::recents() noexcept -> std::map<uint64_t, top::common::xeth_address_t> & {
    return recents_;
}

auto xtop_snapshot::recent_fork_hashes() const noexcept -> std::map<uint64_t, std::string> const & {
    return recent_fork_hashes_;
}

auto xtop_snapshot::recent_fork_hashes() noexcept -> std::map<uint64_t, std::string> & {
    return recent_fork_hashes_;
}

auto xtop_snapshot::attestation() const noexcept -> xvote_data_t const & {
    return attestation_;
}

NS_END4

