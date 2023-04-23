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
#include "json/reader.h"
#include "xcommon/rlp.h"
#include "xutility/xhash.h"

#include <cinttypes>

NS_BEG2(top, evm_common)
bool xeth_header_t::operator==(xeth_header_t const & rhs) const {
    return (this->parent_hash == rhs.parent_hash)
    && (this->uncle_hash == rhs.uncle_hash)
    && (this->miner == rhs.miner)
    && (this->state_merkleroot == rhs.state_merkleroot)
    && (this->tx_merkleroot == rhs.tx_merkleroot)
    && (this->receipt_merkleroot == rhs.receipt_merkleroot)
    && (this->bloom == rhs.bloom)
    && (this->difficulty == rhs.difficulty)
    && (this->number == rhs.number)
    && (this->gas_limit == rhs.gas_limit)
    && (this->gas_used == rhs.gas_used)
    && (this->time == rhs.time)
    && (this->extra == rhs.extra)
    && (this->mix_digest == rhs.mix_digest)
    && (this->nonce == rhs.nonce)
    && (this->base_fee.value() == rhs.base_fee.value());
}

xh256_t xeth_header_t::hash() const {
    auto const value = encode_rlp();
    auto const hash_value = utl::xkeccak256_t::digest(value.data(), value.size());
    return xh256_t{hash_value.data(), xh256_t::ConstructFromPointer};
}

xh256_t xeth_header_t::hash_without_seal() const {
    auto const value = encode_rlp_withoutseal();
    auto const hash_value = utl::xkeccak256_t::digest(value.data(), value.size());
    return xh256_t{hash_value.data(), xh256_t::ConstructFromPointer};
}

xbytes_t xeth_header_t::encode_rlp_withoutseal() const {
    xbytes_t out;
    {
        auto tmp = RLP::encode(parent_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(uncle_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(miner.to_bytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(state_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(tx_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(receipt_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(bloom.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(number));
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
    if (base_fee.has_value()) {
        auto tmp = RLP::encode(static_cast<u256>(base_fee.value()));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    return RLP::encodeList(out);
}

xbytes_t xeth_header_t::encode_rlp() const {
    xbytes_t out;
    {
        auto tmp = RLP::encode(parent_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(uncle_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(miner.to_bytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(state_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(tx_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(receipt_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(bloom.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(number));
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
    {
        auto tmp = RLP::encode(mix_digest.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(nonce.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    if (base_fee.has_value()) {
        auto tmp = RLP::encode(static_cast<u256>(base_fee.value()));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    return RLP::encodeList(out);
}

bool xeth_header_t::decode_rlp(xbytes_t const & bytes) {
    auto l = RLP::decodeList(bytes);
    if (l.decoded.size() < 15) {
        return false;
    }

    if (l.decoded[0].size() != xh256_t::size()) {
        return false;
    }
    parent_hash = static_cast<xh256_t>(l.decoded[0]);

    if (l.decoded[1].size() != xh256_t::size()) {
        return false;
    }
    uncle_hash = static_cast<xh256_t>(l.decoded[1]);

    if (l.decoded[2].size() != xh160_t::size()) {
        return false;
    }
    miner = common::xeth_address_t::build_from(l.decoded[2]);

    if (l.decoded[3].size() != xh256_t::size()) {
        return false;
    }
    state_merkleroot = static_cast<xh256_t>(l.decoded[3]);

    if (l.decoded[4].size() != xh256_t::size()) {
        return false;
    }
    tx_merkleroot = static_cast<xh256_t>(l.decoded[4]);

    if (l.decoded[5].size() != xh256_t::size()) {
        return false;
    }
    receipt_merkleroot = static_cast<xh256_t>(l.decoded[5]);

    if (l.decoded[6].size() != xh2048_t::size()) {
        return false;
    }
    bloom = static_cast<LogBloom>(l.decoded[6]);

    difficulty = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[7]));
    number = evm_common::fromBigEndian<u256>(l.decoded[8]).convert_to<uint64_t>();
    gas_limit = evm_common::fromBigEndian<u64>(l.decoded[9]).convert_to<uint64_t>();
    gas_used = evm_common::fromBigEndian<u64>(l.decoded[10]).convert_to<uint64_t>();
    time = evm_common::fromBigEndian<u64>(l.decoded[11]).convert_to<uint64_t>();
    extra = l.decoded[12];

    if (l.decoded[13].size() != xh256_t::size()) {
        return false;
    }
    mix_digest = static_cast<xh256_t>(l.decoded[13]);

    nonce = static_cast<BlockNonce>(l.decoded[14]);
    if (l.decoded.size() >= 16) {
        base_fee = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[15]));
    }
    return true;
}

std::string xeth_header_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf, sizeof(local_param_buf), "height: %" PRIu64 ", hash: %s, parent_hash: %s", number, hash().hex().c_str(), parent_hash.hex().c_str());
    return std::string(local_param_buf);
}

void xeth_header_t::print() const {
    printf("parent_hash: %s\n", parent_hash.hex().c_str());
    printf("uncle_hash: %s\n", uncle_hash.hex().c_str());
    printf("miner: %s\n", miner.to_hex_string().c_str());
    printf("state_merkleroot: %s\n", state_merkleroot.hex().c_str());
    printf("tx_merkleroot: %s\n", tx_merkleroot.hex().c_str());
    printf("receipt_merkleroot: %s\n", receipt_merkleroot.hex().c_str());
    printf("bloom: %s\n", bloom.hex().c_str());
    printf("difficulty: %s\n", difficulty.str().c_str());
    printf("number: %" PRIu64 "\n", number);
    printf("gas_limit: %lu\n", gas_limit);
    printf("gas_used: %lu\n", gas_used);
    printf("time: %lu\n", time);
    printf("extra: %s\n", top::to_hex(extra).c_str());
    printf("mix_digest: %s\n", mix_digest.hex().c_str());
    printf("nonce: %s\n", nonce.hex().c_str());
    printf("hash: %s\n", hash().hex().c_str());
    if (base_fee.has_value()) {
        printf("base_fee: %s\n", base_fee.value().str().c_str());
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
        auto tmp = RLP::encode(parent_hash.asBytes());
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
    parent_hash = static_cast<xh256_t>(l.decoded[1]);
    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[2]));
    return true;
}

NS_END2
