#include "xevm_common/xcrosschain/xvalidators_snapshot.h"

#include "xbasic/xhex.h"
#include "xcrypto/xckey.h"
#include "xevm_common/rlp.h"
#include "xutility/xhash.h"

NS_BEG2(top, evm_common)

constexpr uint64_t extraVanity = 32;
constexpr uint64_t extraSeal = 64 + 1;
constexpr uint64_t epoch = 200;
constexpr uint64_t addressLength = 20;

static uint256_t seal_hash(const xeth_header_t & header) {
    bytes out;
    {
        auto tmp = RLP::encode(header.parent_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.uncle_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.miner.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.state_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.tx_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.receipt_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.bloom.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(header.difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(header.number));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.gas_limit);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.gas_used);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        xbytes_t extra{header.extra.begin(), header.extra.begin() + (header.extra.size() - extraSeal)};
        auto tmp = RLP::encode(extra);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.mix_digest.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.nonce.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    auto value = RLP::encodeList(out);
    return utl::xkeccak256_t::digest(value.data(), value.size());
}

static uint256_t seal_hash(const xeth_header_t & header, const bigint chainid) {
    bytes out;
    {
        auto tmp = RLP::encode(static_cast<u256>(chainid));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.parent_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.uncle_hash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.miner.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.state_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.tx_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.receipt_merkleroot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.bloom.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(header.difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(static_cast<u256>(header.number));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.gas_limit);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.gas_used);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        xbytes_t extra{header.extra.begin(), header.extra.begin() + (header.extra.size() - extraSeal)};
        auto tmp = RLP::encode(extra);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.mix_digest.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    {
        auto tmp = RLP::encode(header.nonce.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    auto value = RLP::encodeList(out);
    return utl::xkeccak256_t::digest(value.data(), value.size());
}

static xbytes_t ecrecover(const xeth_header_t & header) {
    if (header.extra.size() < extraSeal) {
        xwarn("[ecrecover] header.extra size %zu < extraSeal %lu", header.extra.size(), extraSeal);
        return {};
    }

    uint8_t sig_array[65] = {0};
    sig_array[0] = header.extra.back();
    std::memcpy(sig_array + 1, header.extra.data() + header.extra.size() - extraSeal, 64);
    utl::xecdsasig_t sig(sig_array);

    uint8_t pubkey[65] = {0};
    auto hash = seal_hash(header);
    if (false == utl::xsecp256k1_t::get_publickey_from_signature_directly(sig, hash, pubkey)) {
        xwarn("[ecrecover] get_publickey_from_signature_directly failed, extra: %s, seal_hash: %s", to_hex(header.extra).c_str(), to_hex(to_bytes(hash)).c_str());
        return {};
    }
    auto digest = to_bytes(utl::xkeccak256_t::digest(&pubkey[1], sizeof(pubkey) - 1));
    return {digest.begin() + 12, digest.end()};
}

static xbytes_t ecrecover(const xeth_header_t & header, const bigint chainid) {
    if (header.extra.size() < extraSeal) {
        xwarn("[ecrecover] header.extra size %zu < extraSeal %lu", header.extra.size(), extraSeal);
        return {};
    }

    uint8_t sig_array[65] = {0};
    sig_array[0] = header.extra.back();
    std::memcpy(sig_array + 1, header.extra.data() + header.extra.size() - extraSeal, 64);
    utl::xecdsasig_t sig(sig_array);

    uint8_t pubkey[65] = {0};
    auto hash = seal_hash(header, chainid);
    if (false == utl::xsecp256k1_t::get_publickey_from_signature_directly(sig, hash, pubkey)) {
        xwarn("[ecrecover] get_publickey_from_signature_directly failed, extra: %s, seal_hash: %s", to_hex(header.extra).c_str(), to_hex(to_bytes(hash)).c_str());
        return {};
    }
    auto digest = to_bytes(utl::xkeccak256_t::digest(&pubkey[1], sizeof(pubkey) - 1));
    return {digest.begin() + 12, digest.end()};
}

bool xvalidators_snapshot_t::init_with_epoch(const xeth_header_t & header) {
    if (header.number % epoch != 0) {
        xwarn("[xvalidators_snapshot_t::init_with_epoch] not epoch header");
        return false;
    }
    number = static_cast<uint64_t>(header.number);
    hash = header.hash();
    xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
    if (new_validators_bytes.size() % addressLength != 0) {
        xwarn("[xvalidators_snapshot_t::init_with_epoch] new_validators_bytes size error: %zu", new_validators_bytes.size());
        return false;
    }
    auto new_validators_num = new_validators_bytes.size() / addressLength;
    for (uint32_t i = 0; i < new_validators_num; ++i) {
        auto b = xbytes_t{new_validators_bytes.begin() + i * addressLength, new_validators_bytes.begin() + (i + 1) * addressLength};
        validators.insert(b);
    }

    return true;
}

bool xvalidators_snapshot_t::apply(const xeth_header_t & header, bool check_inturn) {
    auto height = header.number;
    if (height != number + 1) {
        xwarn("[xvalidators_snapshot_t::apply] number mismatch %s, %lu", height.str().c_str(), number + 1);
        return false;
    }
    auto limit = validators.size() / 2 + 1;
    if (height >= limit) {
        recents.erase(static_cast<uint64_t>(height - limit));
    }
    auto validator = ecrecover(header);
    xinfo("[xvalidators_snapshot_t::apply] number: %s, validator: %s", height.str().c_str(), to_hex(validator).c_str());

    if (!validators.count(validator)) {
        xwarn("[xvalidators_snapshot_t::apply] validator %s not in validators", to_hex(validator).c_str());
        return false;
    }
    if (static_cast<h160>(validator) != header.miner) {
        xwarn("[xvalidators_snapshot_t::apply] validator %s is not miner %s", to_hex(validator).c_str(), header.miner.hex().c_str());
        return false;
    }
    for (auto r : recents) {
        if (r.second == validator) {
            xwarn("[xvalidators_snapshot_t::apply] validator %s is in recent", to_hex(validator).c_str());
            return false;
        }
    }
    recents[static_cast<uint64_t>(height)] = validator;
    if (height > 0 && height % epoch == 0) {
        xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
        if (new_validators_bytes.size() % addressLength != 0) {
            xwarn("[xvalidators_snapshot_t::apply] new_validators_bytes size error: %zu", new_validators_bytes.size());
            return false;
        }
        auto new_validators_num = new_validators_bytes.size() / addressLength;
        std::set<xbytes_t> new_validators;
        for (uint32_t i = 0; i < new_validators_num; ++i) {
            new_validators.insert(xbytes_t{new_validators_bytes.begin() + i * addressLength, new_validators_bytes.begin() + (i + 1) * addressLength});
        }
        auto limit = new_validators.size() / 2 + 1;
        for (auto i = 0; i < int(validators.size() / 2 - new_validators.size() / 2); i++) {
            recents.erase(static_cast<uint64_t>(height - limit - i));
        }
        validators = new_validators;
    }
    number += 1;

    const bigint diffInTurn = 2;
    const bigint diffNoTurn = 1;
    if (check_inturn) {
        auto turn = inturn(number, validator);
        if (turn && header.difficulty != diffInTurn) {
            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
            return false;
        }
        if (!turn && header.difficulty != diffNoTurn) {
            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
            return false;
        }
    }

    hash = header.hash();
    return true;
}

bool xvalidators_snapshot_t::apply_with_chainid(const xeth_header_t & header, const bigint chainid, bool check_inturn) {
    auto height = header.number;
    if (height != number + 1) {
        xwarn("[xvalidators_snapshot_t::apply_with_chainid] number mismatch %s, %lu", height.str().c_str(), number + 1);
        return false;
    }
    auto limit = validators.size() / 2 + 1;
    if (height >= limit) {
        recents.erase(static_cast<uint64_t>(height - limit));
    }
    auto validator = ecrecover(header, chainid);
    xinfo("[xvalidators_snapshot_t::apply_with_chainid] number: %s, validator: %s", height.str().c_str(), to_hex(validator).c_str());

    if (!validators.count(validator)) {
        xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s not in validators", to_hex(validator).c_str());
        return false;
    }
    if (static_cast<h160>(validator) != header.miner) {
        xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s is not miner %s", to_hex(validator).c_str(), header.miner.hex().c_str());
        return false;
    }
    for (auto r : recents) {
        if (r.second == validator) {
            xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s is in recent", to_hex(validator).c_str());
            return false;
        }
    }
    recents[static_cast<uint64_t>(height)] = validator;
    if (height > 0 && height % epoch == 0) {
        xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
        if (new_validators_bytes.size() % addressLength != 0) {
            xwarn("[xvalidators_snapshot_t::apply_with_chainid] new_validators_bytes size error: %zu", new_validators_bytes.size());
            return false;
        }
        auto new_validators_num = new_validators_bytes.size() / addressLength;
        std::set<xbytes_t> new_validators;
        for (uint32_t i = 0; i < new_validators_num; ++i) {
            new_validators.insert(xbytes_t{new_validators_bytes.begin() + i * addressLength, new_validators_bytes.begin() + (i + 1) * addressLength});
        }
        auto limit = new_validators.size() / 2 + 1;
        for (auto i = 0; i < int(validators.size() / 2 - new_validators.size() / 2); i++) {
            recents.erase(static_cast<uint64_t>(height - limit - i));
        }
        validators = new_validators;
    }
    number += 1;

    const bigint diffInTurn = 2;
    const bigint diffNoTurn = 1;
    if (check_inturn) {
        auto turn = inturn(number, validator);
        if (turn && header.difficulty != diffInTurn) {
            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
            return false;
        }
        if (!turn && header.difficulty != diffNoTurn) {
            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
            return false;
        }
    }

    hash = header.hash();
    return true;
}

bool xvalidators_snapshot_t::inturn(uint64_t number, xbytes_t validator) {
    std::vector<h160> addrs;
    for (auto bytes : validators) {
        addrs.emplace_back(static_cast<h160>(bytes));
    }
    std::sort(addrs.begin(), addrs.end());
    uint32_t index{0};
    for (; index < addrs.size(); ++index) {
        if (addrs[index] == static_cast<h160>(validator)) {
            break;
        }
    }
    assert(index < addrs.size());
    return (number % addrs.size()) == index;
}

h256 xvalidators_snapshot_t::digest() const {
    RLPStream stream;
    stream << number << hash << validators;
    for (auto p : recents) {
        stream << p.first;
        stream << p.second;
    }
    auto v = stream.out();
    auto hash_value = utl::xkeccak256_t::digest(v.data(), v.size());
    return h256(hash_value.data(), h256::ConstructFromPointer);
}

void xvalidators_snapshot_t::print() const {
    printf("number: %lu\n", number);
    printf("hash: %s\n", hash.hex().c_str());
    printf("validators:\n");
    for (auto address : validators) {
        printf("%s\n", to_hex(address).c_str());
    }
    printf("recents:\n");
    for (auto recent : recents) {
        printf("%lu:%s\n", recent.first, to_hex(recent.second).c_str());
    }
    printf("digest: %s\n", digest().hex().c_str());
}

xvalidators_snap_info_t::xvalidators_snap_info_t(h256 snap_hash_, h256 parent_hash_, bigint number_) : snap_hash(snap_hash_), parent_hash(parent_hash_), number(number_) {
}

bytes xvalidators_snap_info_t::encode_rlp() const {
    bytes out;
    {
        auto tmp = RLP::encode(snap_hash.asBytes());
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

bool xvalidators_snap_info_t::decode_rlp(const bytes & input) {
    auto l = RLP::decodeList(input);
    if (l.decoded.size() != 3) {
        return false;
    }
    if (!l.remainder.empty()) {
        return false;
    }
    snap_hash = static_cast<h256>(l.decoded[0]);
    parent_hash = static_cast<h256>(l.decoded[1]);
    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[2]));
    return true;
}

NS_END2
