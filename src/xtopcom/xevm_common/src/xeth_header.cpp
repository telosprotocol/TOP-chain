// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xcrosschain/xeth_header.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xcontext.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xhex.h"
#include "xcommon/rlp.h"
#include "xevm_common/xerror/xerror.h"
#include "xutility/xhash.h"

#include <cinttypes>
#include <cstring>

NS_BEG2(top, evm_common)

bool xeth_header_t::operator==(xeth_header_t const & rhs) const {
    return (this->parent_hash == rhs.parent_hash) && (this->uncle_hash == rhs.uncle_hash) && (this->miner == rhs.miner) && (this->state_root == rhs.state_root) &&
           (this->transactions_root == rhs.transactions_root) && (this->receipts_root == rhs.receipts_root) && (this->bloom == rhs.bloom) && (this->difficulty == rhs.difficulty) &&
           (this->number == rhs.number) && (this->gas_limit == rhs.gas_limit) && (this->gas_used == rhs.gas_used) && (this->time == rhs.time) && (this->extra == rhs.extra) &&
           (this->mix_digest == rhs.mix_digest) && (this->nonce == rhs.nonce) && (this->base_fee_per_gas == rhs.base_fee_per_gas) &&
           (this->withdrawals_root == rhs.withdrawals_root);
}

xh256_t xeth_header_t::calc_hash(bool const partial) const {
    auto const value = encode_rlp(partial);
    auto const hash_value = utl::xkeccak256_t::digest(value.data(), value.size());
    return xh256_t{hash_value.data(), xh256_t::ConstructFromPointer};
}

void xeth_header_t::calc_hash(xh256_t & out, bool const partial) const {
    auto const value = encode_rlp(partial);
    auto const hash_value = utl::xkeccak256_t::digest(value.data(), value.size());
    assert(static_cast<size_t>(hash_value.size()) == out.size());
    std::memcpy(out.data(), hash_value.data(), std::min(out.size(), static_cast<size_t>(hash_value.size())));
}

xbytes_t xeth_header_t::encode_rlp(bool const partial) const {
    xbytes_t out;
    {
        auto tmp = RLP::encode(parent_hash.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(uncle_hash.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(miner.to_bytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(state_root.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(transactions_root.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(receipts_root.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(bloom.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(difficulty);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(number);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(gas_limit);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(gas_used);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(extra);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    if (!partial) {
        {
            auto tmp = RLP::encode(mix_digest.asArray());
            out.insert(out.end(), tmp.begin(), tmp.end());
        }
        {
            auto tmp = RLP::encode(nonce);
            out.insert(out.end(), tmp.begin(), tmp.end());
        }
    }

    if (base_fee_per_gas.has_value()) {
        auto tmp = RLP::encode(static_cast<u256>(base_fee_per_gas.value()));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    if (withdrawals_root.has_value()) {
        auto tmp = RLP::encode(withdrawals_root.value().asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    return RLP::encodeList(out);
}

void xeth_header_t::decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
    assert(!ec);

    auto const & l = RLP::decode_list(bytes, ec);
    if (ec) {
        xwarn("xeth_header_t::decode_rlp failed, rlp list decode failed: %s", ec.message().c_str());
        return;
    }

    if (l.decoded.size() < 15) {
        xwarn("xeth_header_t::decode_rlp failed, rlp list size not match");
        ec = error::xerrc_t::rlp_list_size_not_match;
        return;
    }

    if (l.decoded[0].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp parent hash invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    parent_hash = xh256_t{xspan_t<xbyte_t const>{l.decoded[0]}};

    if (l.decoded[1].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp uncle hash invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    uncle_hash = xh256_t{xspan_t<xbyte_t const>{l.decoded[1]}};

    if (l.decoded[2].size() != xh160_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp miner invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    miner = common::xeth_address_t::build_from(l.decoded[2]);

    if (l.decoded[3].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp state root invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    state_root = xh256_t{xspan_t<xbyte_t const>{l.decoded[3]}};

    if (l.decoded[4].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp transactions root invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    transactions_root = xh256_t{xspan_t<xbyte_t const>{l.decoded[4]}};

    if (l.decoded[5].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp receipts root invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    receipts_root = xh256_t{xspan_t<xbyte_t const>{l.decoded[5]}};

    if (l.decoded[6].size() != xh2048_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp bloom invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    bloom = LogBloom{xspan_t<xbyte_t const>{l.decoded[6]}};

    difficulty = evm_common::fromBigEndian<u256>(l.decoded[7]);
    number = evm_common::fromBigEndian<uint64_t>(l.decoded[8]);
    gas_limit = evm_common::fromBigEndian<u256>(l.decoded[9]);
    gas_used = evm_common::fromBigEndian<u256>(l.decoded[10]);
    time = evm_common::fromBigEndian<uint64_t>(l.decoded[11]);
    extra = l.decoded[12];

    if (l.decoded[13].size() != xh256_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp mix digest invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    mix_digest = xh256_t{xspan_t<xbyte_t const>{l.decoded[13]}};

    if (l.decoded[14].size() != xh64_t::size()) {
        xwarn("xeth_header_t::decode_rlp failed, rlp nonce invalid");
        ec = error::xerrc_t::rlp_bytes_invalid;
        return;
    }
    nonce = xh64_t{xspan_t<xbyte_t const>{l.decoded[14]}};

    if (l.decoded.size() >= 16) {
        base_fee_per_gas = evm_common::fromBigEndian<uint64_t>(l.decoded[15]);
    }

    if (l.decoded.size() >= 17) {
        if (l.decoded[16].size() != xh256_t::size()) {
            xwarn("xeth_header_t::decode_rlp failed, rlp withdrawals root invalid");
            ec = error::xerrc_t::rlp_bytes_invalid;
            return;
        }
        withdrawals_root = xh256_t{xspan_t<xbyte_t const>{l.decoded[16]}};
    }

    {
        xbytes_t rlp_bytes = bytes;
        auto const & encoded_bytes = encode_rlp();
        if (encoded_bytes.size() != rlp_bytes.size()) {
            rlp_bytes = RLP::encodeList(rlp_bytes);
        }
        calc_hash(hash);
        auto const & bytes_hash = utl::xkeccak256_t::digest(rlp_bytes.data(), rlp_bytes.size());
        assert(hash.size() == static_cast<size_t>(bytes_hash.size()));
        if (std::memcmp(hash.data(), bytes_hash.data(), hash.size()) != 0) {
            xwarn("xeth_header_t::decode_rlp failed, rlp hash invalid");
            hash.clear();
            ec = error::xerrc_t::rlp_bytes_invalid;
            assert(false);
            return;
        }

        calc_hash(partial_hash, true);
    }
}

void xeth_header_t::decode_rlp(xbytes_t const & bytes) {
    std::error_code ec;
    decode_rlp(bytes, ec);
    top::error::throw_error(ec);
}

std::string xeth_header_t::dump() const {
    char local_param_buf[256] = {0};
    xprintf(local_param_buf, sizeof(local_param_buf), "height: %" PRIu64 ", hash: %s, parent_hash: %s", number, calc_hash().hex().c_str(), parent_hash.hex().c_str());
    return std::string{local_param_buf};
}

void xeth_header_t::print() const {
    printf("parent_hash: %s\n", parent_hash.hex().c_str());
    printf("uncle_hash: %s\n", uncle_hash.hex().c_str());
    printf("miner: %s\n", miner.to_hex_string().c_str());
    printf("state_merkleroot: %s\n", state_root.hex().c_str());
    printf("tx_merkleroot: %s\n", transactions_root.hex().c_str());
    printf("receipt_merkleroot: %s\n", receipts_root.hex().c_str());
    printf("bloom: %s\n", bloom.hex().c_str());
    printf("difficulty: %s\n", difficulty.str().c_str());
    printf("number: %" PRIu64 "\n", number);
    printf("gas_limit: %s\n", gas_limit.str().c_str());
    printf("gas_used: %s\n", gas_used.str().c_str());
    printf("time: %lu\n", time);
    printf("extra: %s\n", top::to_hex(extra).c_str());
    printf("mix_digest: %s\n", mix_digest.hex().c_str());
    printf("nonce: %s\n", nonce.hex().c_str());
    printf("hash: %s\n", calc_hash().hex().c_str());
    if (base_fee_per_gas.has_value()) {
        printf("base_fee: %" PRIu64 "\n", base_fee_per_gas.value());
    }
    if (withdrawals_root.has_value()) {
        printf("withdrawals_root: %s\n", withdrawals_root.value().hex().c_str());
    }
}

xeth_header_info_t::xeth_header_info_t(bigint difficult_sum, xh256_t parent_hash, bigint number)
  : difficult_sum{std::move(difficult_sum)}, parent_hash{parent_hash}, number{std::move(number)} {
}

xbytes_t xeth_header_info_t::encode_rlp() const {
    xbytes_t out;
    {
        auto tmp = RLP::encode(static_cast<u256>(difficult_sum));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(parent_hash.asArray());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(number));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    return RLP::encodeList(out);
}

bool xeth_header_info_t::decode_rlp(xbytes_t const & input) {
    auto l = RLP::decodeList(input);
    if (l.decoded.size() != 3) {
        return false;
    }
    if (!l.remainder.empty()) {
        return false;
    }
    difficult_sum = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[0]));
    parent_hash = xh256_t{xspan_t<xbyte_t const>{l.decoded[1]}};
    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[2]));
    return true;
}

NS_END2
