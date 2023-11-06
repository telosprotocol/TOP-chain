// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xcrosschain/xbsc/xvote.h"
#include "xcommon/rlp.h"
#include "xevm_common/xerror/xerror.h"
#include "xevm_common/xcommon.h"
#include "xutility/xhash.h"

NS_BEG4(top, evm, crosschain, bsc)
xtop_vote_data::xtop_vote_data(std::uint64_t source_number, xh256_t const & source_hash, std::uint64_t target_number, xh256_t const & target_hash)
    : source_number_{source_number}, source_hash_{source_hash}, target_number_{target_number}, target_hash_{target_hash} {
}

auto xtop_vote_data::source_number() const noexcept -> std::uint64_t {
    return source_number_;
}

auto xtop_vote_data::source_hash() const noexcept -> xh256_t const & {
    return source_hash_;
}

auto xtop_vote_data::target_number() const noexcept -> std::uint64_t {
    return target_number_;
}

auto xtop_vote_data::target_hash() const noexcept -> xh256_t const& {
    return target_hash_;
}

auto xtop_vote_data::encode_rlp() const -> xbytes_t {
    xbytes_t out;

    auto bytes = evm_common::RLP::encode(source_number());
    out.insert(out.end(), bytes.begin(), bytes.end());

    bytes = evm_common::RLP::encode(source_hash());
    out.insert(out.end(), bytes.begin(), bytes.end());

    bytes = evm_common::RLP::encode(target_number());
    out.insert(out.end(), bytes.begin(), bytes.end());

    bytes = evm_common::RLP::encode(target_hash());
    out.insert(out.end(), bytes.begin(), bytes.end());

    out = evm_common::RLP::encodeList(out);
    return out;
}

auto xtop_vote_data::hash() const -> xh256_t {
    xbytes_t const & value = encode_rlp();
    auto const hash_value = utl::xkeccak256_t::digest(value.data(), value.size());
    return xh256_t{hash_value.data(), xh256_t::ConstructFromPointer};
}

auto xtop_vote_attestation::decode_rlp(xbytes_t const & bytes, std::error_code & ec) -> void {
    assert(!ec);

    auto const & l = evm_common::RLP::decode_list(bytes, ec);
    if (ec) {
        xwarn("%s, decode_rlp failed, ec: {}", __func__, ec.message().c_str());
        return;
    }

    if (l.decoded.size() != 4) {
        ec = evm_common::error::xerrc_t::rlp_list_size_not_match;
        xwarn("%s, decode_rlp failed, expected 4 fields in xvote_attestation, got %zu", __func__, l.decoded.size());
        return;
    }

    vote_address_set = evm_common::fromBigEndian<uint64_t>(l.decoded[0]);

    if (l.decoded[1].size() != common::BLS_SIGNATURE_LEN) {
        ec = evm_common::error::xerrc_t::rlp_bytes_invalid;
        xwarn("%s, decode_rlp failed, invalid BLS signature length: %sz", __func__, l.decoded[1].size());
        return;
    }
}


NS_END4
