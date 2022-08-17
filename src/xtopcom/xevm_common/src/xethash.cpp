#include "xevm_common/xcrosschain/xethash.h"

#include "xbasic/xhex.h"
#include "xdepends/include/ethash/keccak.hpp"
#include "xdepends/include/ethash/src/ethash/endianness.hpp"
#include "xdepends/include/ethash/src/ethash/ethash-internal.hpp"
#include "xevm_common/rlp.h"
#include "xevm_common/src/xethash_data.cpp"
#include "xevm_common/xcrosschain/xeth_config.h"
#include "xutility/xhash.h"

NS_BEG3(top, evm_common, eth)

using ::ethash::hash1024;
using ::ethash::hash512;
using ::ethash::hash256;

xethash_t & xethash_t::instance() {
    static xethash_t * instance = new xethash_t();
    return *instance;
}

xethash_t::xethash_t() {
    std::error_code ec;
    for (auto merkle_root_hex : dag_merkle_roots) {
        auto merkle_root_bytes = from_hex(merkle_root_hex, ec);
        top::error::throw_error(ec);
        m_dag_merkle_roots.emplace_back(static_cast<h128>(merkle_root_bytes));
    }
}

bigint xethash_t::calc_difficulty(const uint64_t time, const xeth_header_t & header, bigint bomb_delay) {
    // algorithm:
    // diff = (parent_diff +
    //         (parent_diff / 2048 * max((2 if len(parent.uncles) else 1) - ((timestamp - parent.timestamp) // 9), -99))
    //        ) + 2^(periodCount - 2)

    // base difficult
    bigint target = header.difficulty;

    // adjust difficult
    bigint current_time =  bigint(time);
    bigint parent_time = bigint(header.time);
    bigint timestampDiff = current_time - parent_time;
    bigint adjFactor;
    bytes out = RLP::encodeList<bytes>({});
    h256 hash = FixedHash<32>(utl::xkeccak256_t::digest(out.data(), out.size()).data(), h256::ConstructFromPointer);
    if (header.uncle_hash == hash) {
        adjFactor = 1 - timestampDiff / 9;
    } else {
        adjFactor = 2 - timestampDiff / 9;
    }

    if (adjFactor < -99) {
        adjFactor = -99;
    }

    target = target + target / 2048 * adjFactor;

    if (target <  131072) {
        target =  131072;
    }

    // bomb difficult
    bigint numbers = 0;
    if ((header.number + 1) >= bomb_delay) {
        numbers = header.number + 1 - bomb_delay;
    }

    bigint periodCount = numbers / 100000;
    if (periodCount > 1) {
        target += (bigint(1) << unsigned(periodCount - 2));
    }
    return target;
}

bigint xethash_t::calc_difficulty(const uint64_t time, const xeth_header_t & parent) {
    bigint next{parent.number + 1};
    if (eth::config::is_arrow_glacier(next)) {
        return calc_difficulty(time, parent, 10700000);
    } else if (eth::config::is_london(next)) {
       return calc_difficulty(time, parent, 9700000);
    } else {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not London fork");
        return {};
    }

    return {};
}

static uint32_t fnv1(uint32_t u, uint32_t v) noexcept {
    static const uint32_t fnv_prime = 0x01000193;
    return (u * fnv_prime) ^ v;
}

static hash512 hash_seed(const hash256 & header_hash, uint64_t nonce) noexcept {
    nonce = ::ethash::le::uint64(nonce);
    uint8_t init_data[sizeof(header_hash) + sizeof(nonce)];
    std::memcpy(&init_data[0], &header_hash, sizeof(header_hash));
    std::memcpy(&init_data[sizeof(header_hash)], &nonce, sizeof(nonce));

    return ::ethash::keccak512(init_data, sizeof(init_data));
}

static hash256 hash_final(const hash512 & seed, const hash256 & mix_hash) {
    uint8_t final_data[sizeof(seed) + sizeof(mix_hash)];
    std::memcpy(&final_data[0], seed.bytes, sizeof(seed));
    std::memcpy(&final_data[sizeof(seed)], mix_hash.bytes, sizeof(mix_hash));
    return ::ethash::keccak256(final_data, sizeof(final_data));
}

h128 xethash_t::apply_merkle_proof(const uint64_t index, const double_node_with_merkle_proof & node) const {
    xbytes_t data{node.dag_nodes[0].begin(), node.dag_nodes[0].end()};
    data.insert(data.end(), node.dag_nodes[1].begin(), node.dag_nodes[1].end());
    auto hash = utl::xsha2_256_t::digest_bytes(data.data(), data.size());
    auto leaf = static_cast<h128>(xbytes_t{hash.begin() + 16, hash.end()});
    for (size_t i = 0; i < node.proof.size(); i++) {
        xbytes_t data(32, 0);
        if ((index >> i) % 2 == 0) {
            data.insert(data.begin() + 16, leaf.begin(), leaf.end());
            data.insert(data.begin() + 48, node.proof[i].begin(), node.proof[i].end());
        } else {
            data.insert(data.begin() + 16, node.proof[i].begin(), node.proof[i].end());
            data.insert(data.begin() + 48, leaf.begin(), leaf.end());
        }
        auto hash = utl::xsha2_256_t::digest_bytes(data.data(), data.size());
        leaf = static_cast<h128>(xbytes_t{hash.begin() + 16, hash.end()});
    }
    return leaf;
}

std::pair<hash256, hash256> xethash_t::hashimoto(const hash256 & hash, const uint64_t nonce, const size_t full_size, std::function<hash1024(uint64_t)> lookup) const {
    static constexpr size_t num_words = sizeof(hash1024) / sizeof(uint32_t);
    const hash512 seed = hash_seed(hash, nonce);
    const uint32_t seed_init = ::ethash::le::uint32(seed.word32s[0]);

    hash1024 mix{{::ethash::le::uint32s(seed), ::ethash::le::uint32s(seed)}};

    for (uint32_t i = 0; i < ::ethash::num_dataset_accesses; ++i)
    {
        const uint32_t p = fnv1(i ^ seed_init, mix.word32s[i % num_words]) % full_size;
        const hash1024 newdata = ::ethash::le::uint32s(lookup(p));

        for (size_t j = 0; j < num_words; ++j)
            mix.word32s[j] = fnv1(mix.word32s[j], newdata.word32s[j]);
    }

    hash256 mix_hash;
    for (size_t i = 0; i < num_words; i += 4)
    {
        const uint32_t h1 = fnv1(mix.word32s[i], mix.word32s[i + 1]);
        const uint32_t h2 = fnv1(h1, mix.word32s[i + 2]);
        const uint32_t h3 = fnv1(h2, mix.word32s[i + 3]);
        mix_hash.word32s[i / 4] = h3;
    }

    return {::ethash::le::uint32s(mix_hash), hash_final(seed, ::ethash::le::uint32s(mix_hash))};
}

std::pair<hash256, hash256> xethash_t::hashimoto_merkle(const hash256 & header_hash,
                                                        uint64_t nonce,
                                                        uint64_t header_number,
                                                        const std::vector<double_node_with_merkle_proof> & nodes) const {
    std::unique_ptr<int> index{new int(0)};
    auto epoch = header_number / ETHASH_EPOCH_LENGTH;
    auto merkle_root = m_dag_merkle_roots[epoch];
    if (epoch >= m_dag_merkle_roots.size()) {
        return {};
    }
    auto lookup = [&](uint64_t offset){
        auto node = nodes[*index];
        *index += 1;
        auto calc_root =  apply_merkle_proof(offset, node);
        if (merkle_root != calc_root) {
            throw std::invalid_argument{"merkle_root calculation mismatch!"};
        }
        hash1024 hash;
        {
            xbytes_t data = node.dag_nodes[0].asBytes();
            data.insert(data.end(), node.dag_nodes[1].begin(), node.dag_nodes[1].end());
            std::reverse(data.begin(), data.begin() + 32);
            std::reverse(data.begin() + 32, data.begin() + 64);
            std::reverse(data.begin() + 64, data.begin() + 96);
            std::reverse(data.begin() + 96, data.end());
            std::memcpy(hash.bytes, data.data(), 128);
        }
        return hash;
    };
    return hashimoto(header_hash, nonce, ethash_calculate_full_dataset_num_items(epoch), lookup);
}

bool xethash_t::verify_seal(const xeth_header_t & header, const std::vector<double_node_with_merkle_proof> & nodes) {
    hash256 hash;
    std::memcpy(hash.bytes, header.hash_without_seal().data(), 32);
    hash256 mix_hash;
    std::memcpy(mix_hash.bytes, header.mix_digest.data(), 32);
    hash256 difficulty;
    std::memcpy(difficulty.bytes, toBigEndian(static_cast<u256>(header.difficulty)).data(), 32);
    uint64_t nonce = std::stoull(header.nonce.hex(), nullptr, 16);
    uint64_t number = static_cast<uint64_t>(header.number);
    auto hashes = hashimoto_merkle(hash, nonce, number, nodes);
    if (!::ethash::equal(hashes.first, mix_hash)) {
        return false;
    }
    if (!::ethash::check_against_difficulty(hashes.second, difficulty)) {
        return false;
    }
    return true;
}

NS_END3