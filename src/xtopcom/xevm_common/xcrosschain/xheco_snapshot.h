#pragma once

#include "xcommon/xeth_address.h"
#include "xcommon/xeth_address_fwd.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/rlp.h"
#include "xutility/xhash.h"
#include "xevm_common/xeth/xeth_header.h"

#include "xbasic/xhex.h"
#include "xcrypto/xckey.h"

NS_BEG3(top, evm_common, heco)

constexpr uint64_t extraVanity = 32;
constexpr uint64_t extraSeal = 64 + 1;
constexpr uint64_t epoch = 200;
constexpr uint64_t addressLength = 20;

uint256_t seal_hash(const eth::xeth_header_t & header) {
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

xbytes_t ecrecover(const eth::xeth_header_t & header) {
    if (header.extra.size() < extraSeal) {
        return {};
    }
    xbytes_t signature{header.extra.begin() + header.extra.size() - extraSeal, header.extra.end()};
    
    uint8_t sig_array[65] = {0};
    std::memcpy(sig_array, signature.data(), 65);

    for (auto i = 0; i < 65; i++) {
        printf("%d ", sig_array[i]);
    }
    utl::xecdsasig_t sig((uint8_t *)sig_array);
    uint8_t szOutput[65] = {0};
    auto h = seal_hash(header);
    auto h_bytes = to_bytes(h);
    printf("%s\n", to_hex(h_bytes).c_str());
    top::utl::xkeyaddress_t pubkey1("");  // just init xsecp256k1_t
    if (false == utl::xsecp256k1_t::get_publickey_from_signature(sig, h, szOutput)) {
        printf("error\n");
        return {};
    }
    for (auto i = 0; i < 65; i++) {
        printf("%d ", szOutput[i]);
    }
    auto digest = to_bytes(utl::xkeccak256_t::digest(&szOutput[1], sizeof(szOutput) - 1));
    return {digest.begin() + 12, digest.end()};
}

class snapshot {
public:
    uint64_t number{0};
    h256 hash;
    std::set<xbytes_t> validators;
    std::map<uint64_t, xbytes_t> recents;

    h256 digest() const {
        RLPStream stream;
        stream << number << hash << validators;
        for (auto p : recents) {
            stream << p.first;
            stream << p.second;
        }
        auto v = stream.out();
        printf("%s\n", to_hex(v).c_str());
        auto hashValue = utl::xkeccak256_t::digest(v.data(), v.size());
        return h256(hashValue.data(), h256::ConstructFromPointer);
    }

    bool apply(const eth::xeth_header_t & header) {
        auto height = header.number;
        if (height != number + 1) {
            return false;
        }
        auto limit = validators.size() / 2 + 1;
        if (height >= limit) {
            recents.erase(static_cast<uint64_t>(height - limit));
        }
        auto validator = ecrecover(header);
        if (!validators.count(validator)) {
            return false;
        }
        if (static_cast<h160>(validator) != header.miner) {
            return false;
        }
        for (auto r : recents) {
            if (r.second == validator) {
                return false;
            }
        }
        recents[static_cast<uint64_t>(height)] = validator;
        if (height > 0 && height % epoch == 0) {
            xbytes_t new_validators_bytes{header.extra.begin() + extraVanity + extraSeal, header.extra.end()};
            if (new_validators_bytes.size() % addressLength != 0) {
                return false;
            }
            auto new_validators_num = new_validators_bytes.size() / addressLength;
            std::set<xbytes_t> new_validators;
            for (uint32_t i = 0; i < new_validators_num; ++i) {
                new_validators.insert(xbytes_t{new_validators_bytes.begin() + i * addressLength, new_validators_bytes.begin() + (i + 1) * addressLength - 1});
            }
            auto limit = new_validators.size() / 2 + 1;
            for (auto i = 0; i < int(validators.size() / 2 - new_validators.size() / 2); i++) {
                recents.erase(static_cast<uint64_t>(height - limit - i));
            }
            if (!new_validators.count(validator)) {
                return false;
            }
            validators = new_validators;
        }
        const bigint diffInTurn = 2;
        const bigint diffNoTurn = 1;
        auto turn = inturn(number, validator);
        if (turn && header.difficulty != diffInTurn) {
            return false;
        }
        if (!turn && header.difficulty != diffNoTurn) {
            return false;
        }

        number += 1;
        hash = header.hash();

        return true;
    }

    bool inturn(uint64_t number, xbytes_t validator) {
        std::vector<xbytes_t> addrs{validators.begin(), validators.end()};
        std::sort(addrs.begin(), addrs.end());
        uint32_t index{0};
        for (;index < addrs.size(); ++index) {
            if (addrs[index] == validator) {
                break;
            }
        }
        assert(index < addrs.size());
        return (number % addrs.size()) == index;
    }
};

struct xheco_snap_info_t {
    xheco_snap_info_t() = default;
    xheco_snap_info_t(h256 snap_hash_, h256 parent_hash_, bigint number_) : snap_hash(snap_hash_), parent_hash(parent_hash_), number(number_) {}

    bytes encode_rlp() const {
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
    bool decode_rlp(const bytes & input) {
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

    h256 snap_hash;
    h256 parent_hash;
    bigint number;
};

NS_END3
