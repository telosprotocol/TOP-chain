//// Copyright (c) 2023-present Telos Foundation & contributors
//// Distributed under the MIT software license, see the accompanying
//// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
//#include "xevm_common/xcrosschain/xvalidators_snapshot.h"
//
//#include "xbasic/xhex.h"
//#include "xcrypto/xckey.h"
//#include "xcommon/rlp.h"
//#include "xutility/xhash.h"
//
//#include <cinttypes>
//
//NS_BEG2(top, evm_common)
//constexpr uint64_t extraVanity = 32;
//constexpr uint64_t extraSeal = 64 + 1;
//constexpr uint64_t epoch = 200;
//
//static uint256_t seal_hash(xeth_header_t const & header) {
//    xbytes_t out;
//    {
//        auto tmp = RLP::encode(header.parent_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.uncle_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.miner.to_bytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.state_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.transactions_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.receipts_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.bloom.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(static_cast<u256>(header.difficulty));
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.number);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.gas_limit);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.gas_used);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.time);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        xbytes_t extra{header.extra.begin(), std::next(header.extra.begin(), static_cast<intptr_t>(header.extra.size() - extraSeal))};
//        auto tmp = RLP::encode(extra);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.mix_digest.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.nonce);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    auto const value = RLP::encodeList(out);
//    return utl::xkeccak256_t::digest(value.data(), value.size());
//}
//
//static uint256_t seal_hash(xeth_header_t const & header, bigint const & chainid) {
//    xbytes_t out;
//    {
//        auto tmp = RLP::encode(static_cast<u256>(chainid));
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.parent_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.uncle_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.miner.to_bytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.state_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.transactions_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.receipts_root.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.bloom.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(static_cast<u256>(header.difficulty));
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.number);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.gas_limit);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.gas_used);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.time);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        xbytes_t extra{header.extra.begin(), header.extra.begin() + (header.extra.size() - extraSeal)};
//        auto tmp = RLP::encode(extra);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.mix_digest.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(header.nonce);
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    auto value = RLP::encodeList(out);
//    return utl::xkeccak256_t::digest(value.data(), value.size());
//}
//
//static common::xeth_address_t ecrecover(xeth_header_t const & header, std::error_code & ec) {
//    if (header.extra.size() < extraSeal) {
//        xwarn("[ecrecover] header.extra size %zu < extraSeal %lu", header.extra.size(), extraSeal);
//        return {};
//    }
//
//    uint8_t sig_array[65] = {0};
//    sig_array[0] = header.extra.back();
//    std::memcpy(sig_array + 1, header.extra.data() + header.extra.size() - extraSeal, 64);
//    utl::xecdsasig_t sig(sig_array);
//
//    uint8_t pubkey[65] = {0};
//    auto const hash = seal_hash(header);
//    if (false == utl::xsecp256k1_t::get_publickey_from_signature_directly(sig, hash, pubkey)) {
//        xwarn("[ecrecover] get_publickey_from_signature_directly failed, extra: %s, seal_hash: %s", to_hex(header.extra).c_str(), to_hex(to_bytes(hash)).c_str());
//        return {};
//    }
//    auto digest = to_bytes(utl::xkeccak256_t::digest(&pubkey[1], sizeof(pubkey) - 1));
//    return common::xeth_address_t::build_from(std::next(std::begin(digest), 12), std::end(digest), ec);
//}
//
//static common::xeth_address_t ecrecover(xeth_header_t const & header, bigint const & chainid, std::error_code & ec) {
//    if (header.extra.size() < extraSeal) {
//        xwarn("[ecrecover] header.extra size %zu < extraSeal %lu", header.extra.size(), extraSeal);
//        return {};
//    }
//
//    uint8_t sig_array[65] = {0};
//    sig_array[0] = header.extra.back();
//    std::memcpy(sig_array + 1, header.extra.data() + header.extra.size() - extraSeal, 64);
//    utl::xecdsasig_t sig(sig_array);
//
//    uint8_t pubkey[65] = {0};
//    auto const hash = seal_hash(header, chainid);
//    if (false == utl::xsecp256k1_t::get_publickey_from_signature_directly(sig, hash, pubkey)) {
//        xwarn("[ecrecover] get_publickey_from_signature_directly failed, extra: %s, seal_hash: %s", to_hex(header.extra).c_str(), to_hex(to_bytes(hash)).c_str());
//        return {};
//    }
//    auto digest = to_bytes(utl::xkeccak256_t::digest(&pubkey[1], sizeof(pubkey) - 1));
//    return common::xeth_address_t::build_from(std::next(std::begin(digest), 12), std::end(digest), ec);
//}
//
//bool xvalidators_snapshot_t::init_with_epoch(xeth_header_t const & header) {
//    if (header.number % epoch != 0) {
//        xwarn("[xvalidators_snapshot_t::init_with_epoch] not epoch header");
//        return false;
//    }
//    number = static_cast<uint64_t>(header.number);
//    hash = header.calc_hash();
//    xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
//    if (new_validators_bytes.size() % common::xeth_address_t::size() != 0) {
//        xwarn("[xvalidators_snapshot_t::init_with_epoch] new_validators_bytes size error: %zu", new_validators_bytes.size());
//        return false;
//    }
//    auto const new_validators_num = new_validators_bytes.size() / common::xeth_address_t::size();
//    for (uint32_t i = 0; i < new_validators_num; ++i) {
//        auto b = common::xeth_address_t::build_from(std::next(std::begin(new_validators_bytes), static_cast<intptr_t>(i * common::xeth_address_t::size())),
//                                                    std::next(std::begin(new_validators_bytes), static_cast<intptr_t>((i + 1) * common::xeth_address_t::size())));
//        validators.insert(b);
//    }
//
//    return true;
//}
//
//bool xvalidators_snapshot_t::init_with_double_epoch(xeth_header_t const & header1, xeth_header_t const & header2) {
//    if (header1.number % epoch != 0 || header2.number % epoch != 0) {
//        xwarn("[xvalidators_snapshot_t::init_with_epoch] not epoch header");
//        return false;
//    }
//    {
//        number = static_cast<uint64_t>(header1.number);
//        hash = header1.calc_hash();
//        xbytes_t new_validators_bytes{header1.extra.begin() + extraVanity, header1.extra.end() - extraSeal};
//        if (new_validators_bytes.size() % common::xeth_address_t::size() != 0) {
//            xwarn("[xvalidators_snapshot_t::init_with_epoch] new_validators_bytes size error: %zu", new_validators_bytes.size());
//            return false;
//        }
//        auto new_validators_num = new_validators_bytes.size() / common::xeth_address_t::size();
//        for (uint32_t i = 0; i < new_validators_num; ++i) {
//            auto b = common::xeth_address_t::build_from(std::next(std::begin(new_validators_bytes), static_cast<intptr_t>(i * common::xeth_address_t::size())),
//                                                        std::next(std::begin(new_validators_bytes), static_cast<intptr_t>((i + 1) * common::xeth_address_t::size())));
//            last_validators.insert(b);
//        }
//    }
//    {
//        number = static_cast<uint64_t>(header2.number);
//        hash = header2.calc_hash();
//        xbytes_t new_validators_bytes{header2.extra.begin() + extraVanity, header2.extra.end() - extraSeal};
//        if (new_validators_bytes.size() % common::xeth_address_t::size() != 0) {
//            xwarn("[xvalidators_snapshot_t::init_with_epoch] new_validators_bytes size error: %zu", new_validators_bytes.size());
//            return false;
//        }
//        auto const new_validators_num = new_validators_bytes.size() / common::xeth_address_t::size();
//        for (uint32_t i = 0; i < new_validators_num; ++i) {
//            auto b = common::xeth_address_t::build_from(std::next(std::begin(new_validators_bytes), static_cast<intptr_t>(i * common::xeth_address_t::size())),
//                                                        std::next(std::begin(new_validators_bytes), static_cast<intptr_t>((i + 1) * common::xeth_address_t::size())));
//            validators.insert(b);
//        }
//    }
//
//    return true;
//}
//
//bool xvalidators_snapshot_t::apply(xeth_header_t const & header, bool const check_inturn) {
//    std::error_code ec;
//
//    auto const height = header.number;
//    if (height != number + 1) {
//        xwarn("[xvalidators_snapshot_t::apply] number mismatch %" PRIu64 ", %" PRIu64, height, number + 1);
//        return false;
//    }
//    auto limit = validators.size() / 2 + 1;
//    if (height >= limit) {
//        recents.erase(height - limit);
//    }
//    auto const validator = ecrecover(header, ec);
//    if (ec) {
//        xwarn("[xvalidators_snapshot_t::apply] ecrecover failed: category %s errc %d msg %s", ec.category().name(), ec.value(), ec.message().c_str());
//        return false;
//    }
//
//    xinfo("[xvalidators_snapshot_t::apply] number: %" PRIu64 ", validator: %s", height, validator.to_hex_string().c_str());
//
//    if (!validators.count(validator)) {
//        xwarn("[xvalidators_snapshot_t::apply] validator %s not in validators", validator.to_hex_string().c_str());
//        return false;
//    }
//    if (validator != header.miner) {
//        xwarn("[xvalidators_snapshot_t::apply] validator %s is not miner %s", validator.to_hex_string().c_str(), header.miner.to_hex_string().c_str());
//        return false;
//    }
//    for (auto const & r : recents) {
//        if (r.second == validator) {
//            xwarn("[xvalidators_snapshot_t::apply] validator %s is in recent", validator.to_hex_string().c_str());
//            return false;
//        }
//    }
//    recents[static_cast<uint64_t>(height)] = validator;
//    if (height > 0 && height % epoch == 0) {
//        xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
//        if (new_validators_bytes.size() % common::xeth_address_t::size() != 0) {
//            xwarn("[xvalidators_snapshot_t::apply] new_validators_bytes size error: %zu", new_validators_bytes.size());
//            return false;
//        }
//        auto const new_validators_num = new_validators_bytes.size() / common::xeth_address_t::size();
//        std::set<common::xeth_address_t> new_validators;
//        for (uint32_t i = 0; i < new_validators_num; ++i) {
//            new_validators.insert(common::xeth_address_t::build_from(std::next(std::begin(new_validators_bytes), static_cast<intptr_t>(i * common::xeth_address_t::size())),
//                                                                     std::next(std::begin(new_validators_bytes), static_cast<intptr_t>((i + 1) * common::xeth_address_t::size()))));
//        }
//        limit = new_validators.size() / 2 + 1;
//        for (auto i = 0u; i < validators.size() / 2 - new_validators.size() / 2; i++) {
//            recents.erase(height - limit - i);
//        }
//        validators = new_validators;
//    }
//    number += 1;
//
//    bigint const diff_in_turn = 2;
//    bigint const diff_no_turn = 1;
//    if (check_inturn) {
//        auto const turn = inturn(number, validator, false);
//        if (turn && header.difficulty != diff_in_turn) {
//            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
//            return false;
//        }
//        if (!turn && header.difficulty != diff_no_turn) {
//            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
//            return false;
//        }
//    }
//
//    hash = header.calc_hash();
//    return true;
//}
//
//bool xvalidators_snapshot_t::apply_with_chainid(xeth_header_t const & header, bigint const & chainid, bool const check_inturn) {
//    auto const height = header.number;
//    if (height != number + 1) {
//        xwarn("[xvalidators_snapshot_t::apply_with_chainid] number mismatch %" PRIu64 ", %" PRIu64, height, number + 1);
//        return false;
//    }
//    auto limit = validators.size() / 2 + 1;
//    if (height >= limit) {
//        recents.erase(height - limit);
//    }
//    std::error_code ec;
//    auto const validator = ecrecover(header, chainid, ec);
//    if (ec) {
//        xwarn("[xvalidators_snapshot_t::apply_with_chainid] ecrecover failed: category %s errc %d msg %s", ec.category().name(), ec.value(), ec.message().c_str());
//        return false;
//    }
//    xinfo("[xvalidators_snapshot_t::apply_with_chainid] number: %" PRIu64 ", validator: %s", height, validator.to_hex_string().c_str());
//
//    auto const pos = height % 200;
//    if (pos >= 1 && pos <= 10) {
//        if (!last_validators.count(validator)) {
//            xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s not in last_validators", validator.to_hex_string().c_str());
//            return false;
//        }
//    } else {
//        if (!validators.count(validator)) {
//            xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s not in validators", validator.to_hex_string().c_str());
//            return false;
//        }
//    }
//    if (validator != header.miner) {
//        xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s is not miner %s", validator.to_hex_string().c_str(), header.miner.to_hex_string().c_str());
//        return false;
//    }
//    for (auto const & r : recents) {
//        if (r.second == validator) {
//            xwarn("[xvalidators_snapshot_t::apply_with_chainid] validator %s is in recent", validator.to_hex_string().c_str());
//            return false;
//        }
//    }
//    recents[static_cast<uint64_t>(height)] = validator;
//    if (height > 0 && height % epoch == 0) {
//        xbytes_t new_validators_bytes{header.extra.begin() + extraVanity, header.extra.end() - extraSeal};
//        if (new_validators_bytes.size() % common::xeth_address_t::size() != 0) {
//            xwarn("[xvalidators_snapshot_t::apply_with_chainid] new_validators_bytes size error: %zu", new_validators_bytes.size());
//            return false;
//        }
//        auto const new_validators_num = new_validators_bytes.size() / common::xeth_address_t::size();
//        std::set<common::xeth_address_t> new_validators;
//        for (uint32_t i = 0; i < new_validators_num; ++i) {
//            // new_validators.insert(xbytes_t{new_validators_bytes.begin() + i * common::xeth_address_t::size(), new_validators_bytes.begin() + (i + 1) * common::xeth_address_t::size()});
//            new_validators.insert(common::xeth_address_t::build_from(std::next(std::begin(new_validators_bytes), static_cast<intptr_t>(i * common::xeth_address_t::size())),
//                                                                     std::next(std::begin(new_validators_bytes), static_cast<intptr_t>((i + 1) * common::xeth_address_t::size()))));
//        }
//        limit = new_validators.size() / 2 + 1;
//        for (auto i = 0u; i < validators.size() / 2 - new_validators.size() / 2; i++) {
//            recents.erase(height - limit - i);
//        }
//        last_validators = validators;
//        validators = new_validators;
//    }
//    number += 1;
//
//    bigint const diff_in_turn = 2;
//    bigint const diff_no_turn = 1;
//    if (check_inturn) {
//        bool turn{false};
//        if (pos >= 0 && pos <= 10) {
//            turn = inturn(number, validator, true);
//        } else {
//            turn = inturn(number, validator, false);
//        }
//        if (turn && header.difficulty != diff_in_turn) {
//            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
//            return false;
//        }
//        if (!turn && header.difficulty != diff_no_turn) {
//            xwarn("[xvalidators_snapshot_t::apply] check inturn failed, turn: %d, difficulty: %s", turn, header.difficulty.str().c_str());
//            return false;
//        }
//    }
//
//    hash = header.calc_hash();
//    return true;
//}
//
//bool xvalidators_snapshot_t::inturn(uint64_t const num, common::xeth_address_t const & validator, bool const use_old) const {
//    std::vector<common::xeth_address_t> addrs;
//    if (use_old) {
//        for (auto const & address : last_validators) {
//            addrs.push_back(address);
//        }
//    } else {
//        for (auto const & address : validators) {
//            addrs.push_back(address);
//        }
//    }
//
//    std::sort(addrs.begin(), addrs.end());
//    uint32_t index{0};
//    for (; index < addrs.size(); ++index) {
//        if (addrs[index] == validator) {
//            break;
//        }
//    }
//    assert(index < addrs.size());
//    return (num % addrs.size()) == index;
//}
//
//h256 xvalidators_snapshot_t::digest() const {
//    RLPStream stream;
//    stream << number << hash << validators;
//    for (auto const & p : recents) {
//        stream << p.first;
//        stream << p.second;
//    }
//    auto const & v = stream.out();
//    auto const hash_value = utl::xkeccak256_t::digest(v.data(), v.size());
//    return h256(hash_value.data(), h256::ConstructFromPointer);
//}
//
//void xvalidators_snapshot_t::print() const {
//    printf("number: %lu\n", number);
//    printf("hash: %s\n", hash.hex().c_str());
//    printf("validators:\n");
//    for (auto const & address : validators) {
//        printf("%s\n", address.to_hex_string().c_str());
//    }
//    printf("recents:\n");
//    for (auto const & recent : recents) {
//        printf("%lu:%s\n", recent.first, recent.second.to_hex_string().c_str());
//    }
//    printf("digest: %s\n", digest().hex().c_str());
//}
//
//xvalidators_snap_info_t::xvalidators_snap_info_t(h256 const & snap_hash, h256 const & parent_hash, uint64_t const number) : snap_hash(snap_hash), parent_hash(parent_hash), number(number) {
//}
//
//xbytes_t xvalidators_snap_info_t::encode_rlp() const {
//    xbytes_t out;
//    {
//        auto tmp = RLP::encode(snap_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(parent_hash.asBytes());
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    {
//        auto tmp = RLP::encode(static_cast<u256>(number));
//        out.insert(out.end(), tmp.begin(), tmp.end());
//    }
//    return RLP::encodeList(out);
//}
//
//bool xvalidators_snap_info_t::decode_rlp(xbytes_t const & input) {
//    auto l = RLP::decodeList(input);
//    if (l.decoded.size() != 3) {
//        return false;
//    }
//    if (!l.remainder.empty()) {
//        return false;
//    }
//    snap_hash = xh256_t{xspan_t<xbyte_t const>{l.decoded[0]}};
//    parent_hash = xh256_t{xspan_t<xbyte_t const>{l.decoded[1]}};
//    number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[2]));
//    return true;
//}
//
//NS_END2
