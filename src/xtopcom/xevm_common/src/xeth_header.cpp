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
#include "xdepends/include/json/reader.h"
#include "xevm_common/rlp.h"
#include "xutility/xhash.h"

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

Hash xeth_header_t::hash() const {
    auto value = encode_rlp();
    auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size());
    return FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
}

Hash xeth_header_t::hash_without_seal() const {
    auto value = encode_rlp_withoutseal();
    auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size());
    return FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
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
        auto tmp = RLP::encode(miner.asBytes());
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
        auto tmp = RLP::encode(miner.asBytes());
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

bool xeth_header_t::decode_rlp(const xbytes_t & bytes) {
    auto l = RLP::decodeList(bytes);
    if (l.decoded.size() < 15) {
        return false;
    }
    parent_hash = static_cast<Hash>(l.decoded[0]);
    uncle_hash = static_cast<Hash>(l.decoded[1]);
    miner = static_cast<Address>(l.decoded[2]);
    state_merkleroot = static_cast<Hash>(l.decoded[3]);
    tx_merkleroot = static_cast<Hash>(l.decoded[4]);
    receipt_merkleroot = static_cast<Hash>(l.decoded[5]);
    bloom = static_cast<LogBloom>(l.decoded[6]);
    difficulty = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[7]));
    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[8]));
    gas_limit = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[9]));
    gas_used = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[10]));
    time = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[11]));
    extra = l.decoded[12];
    mix_digest = static_cast<Hash>(l.decoded[13]);
    nonce = static_cast<BlockNonce>(l.decoded[14]);
    if (l.decoded.size() >= 16) {
        base_fee = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[15]));
    }
    return true;
}

std::string xeth_header_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf, sizeof(local_param_buf), "height: %s, hash: %s, parent_hash: %s", number.str().c_str(), hash().hex().c_str(), parent_hash.hex().c_str());
    return std::string(local_param_buf);
}

void xeth_header_t::print() const {
    printf("parent_hash: %s\n", parent_hash.hex().c_str());
    printf("uncle_hash: %s\n", uncle_hash.hex().c_str());
    printf("miner: %s\n", miner.hex().c_str());
    printf("state_merkleroot: %s\n", state_merkleroot.hex().c_str());
    printf("tx_merkleroot: %s\n", tx_merkleroot.hex().c_str());
    printf("receipt_merkleroot: %s\n", receipt_merkleroot.hex().c_str());
    printf("bloom: %s\n", bloom.hex().c_str());
    printf("difficulty: %s\n", difficulty.str().c_str());
    printf("number: %s\n", number.str().c_str());
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

xeth_header_info_t::xeth_header_info_t(bigint difficult_sum_, Hash parent_hash_, bigint number_) : difficult_sum{difficult_sum_}, parent_hash{parent_hash_}, number{number_} {
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

bool xeth_header_info_t::decode_rlp(const xbytes_t & input) {
    auto l = RLP::decodeList(input);
    if (l.decoded.size() != 3) {
        return false;
    }
    if (!l.remainder.empty()) {
        return false;
    }
    difficult_sum = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[0]));
    parent_hash = static_cast<Hash>(l.decoded[1]);
    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[2]));
    return true;
}

NS_END2
