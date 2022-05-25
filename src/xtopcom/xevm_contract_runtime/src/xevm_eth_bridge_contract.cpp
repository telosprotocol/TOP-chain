// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xeth/xeth_difficulty.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xethash_util.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace top::evm_common;

constexpr uint64_t LondonBlock = 12965000;
constexpr uint64_t ArrowGlacierBlock = 13773000;
constexpr uint64_t LondonBombDelay = 9700000;
constexpr uint64_t ArrowGlacierBombDelay = 10700000;

constexpr uint64_t GasLimitBoundDivisor = 1024;
constexpr uint64_t MinGasLimit = 5000;
constexpr uint64_t MaxGasLimit = 0x7fffffffffffffff;
constexpr uint64_t MaximumExtraDataSize = 32;

constexpr uint64_t ElasticityMultiplier = 2;
constexpr uint64_t InitialBaseFee = 1000000000;
constexpr uint64_t BaseFeeChangeDenominator = 8;

bool xtop_evm_eth_bridge_contract::execute(xbytes_t input,
                                           uint64_t target_gas,
                                           sys_contract_context const & context,
                                           bool is_static,
                                           observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                           sys_contract_precompile_output & output,
                                           sys_contract_precompile_error & err) {
    assert(state_ctx);
    // m_contract_state = state_ctx->load_unit_state(m_contract_address.vaccount());
    // if (m_contract_state == nullptr) {
    //     return false;
    // }
    xwarn("[xtop_evm_eth_bridge_contract::execute]");
    std::string s;
    int cnt{0};
    for (auto c : input) {
        s += top::to_string(int(c));
        s += ' ';
        cnt++;
        if (cnt == 10) {
            xinfo("%s", s.c_str());
            cnt = 0;
            s.clear();
        }
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::init(std::string headerContent, std::string emitter) {
    // step 0: check init
    bigint height{0};
    if (!get_height(height)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] get_height error");
        return false;
    }
    if (height != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::init] init already");
        return false;
    }
    // step 1: check emitter
    if (!verifyOwner(emitter)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] emitter: %s invalid", emitter.c_str());
        return false;
    }
    // step 2: header
    eth::xeth_block_header_t header;
    if (!header.fromJson(headerContent)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] decode header error");
        return false;
    }
    // step 3: check exist
    h256 hash{0};
    if (get_hash(header.number(), hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] get_hash existed, height: %s, hash: %s", header.number().str().c_str(), hash.hex().c_str());
        return false;
    }
    // step 4: store block
    if (!set_header(header, header.difficulty())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 5: store mainchain header
    if (!set_hash(header.number(), header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_hash failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 6: store height
    if (!set_height(header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_height failed, height: %s", header.number().str().c_str());
        return false;
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::sync(std::string headerContent) {
    // step 1: check init
    bigint last_height{0};
    if (!get_height(last_height)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_height failed, height: %s", last_height.str().c_str());
        return false;
    }
    if (last_height == 0) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not init yet");
        return false;
    }
    // step 2: decode header
    eth::xeth_block_header_t header;
    if (!header.fromJson(headerContent)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
        return false;
    }
    // step 3: check exist
    h256 exist_hash{0};
    if (get_hash(header.number(), exist_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash existed, height: %s, hash: %s", header.number().str().c_str(), exist_hash.hex().c_str());
        return false;
    }
    // step 4: get parent header
    eth::xeth_block_header_t parentHeader;
    bigint preSumOfDifficult{0};
    if (!get_header(header.parentHash(), parentHeader, preSumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, hash: %s", header.parentHash().hex().c_str());
        return false;
    }
    // step 5: verify header common
    if (!verifyCommon(parentHeader, header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] verify header common failed, header: %s, parrent header: %s", header.hash().hex().c_str(), parentHeader.hash().hex().c_str());
        return false;
    }
    if (isLondon(header)) {
        // if (!verifyEip1559Header(parentHeader, header)) {
        //     return false;
        // }
    } else {
        if (!verifyGaslimit(parentHeader.gasLimit(), header.gasLimit())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] gaslimit mismatch, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
            return false;
        }
    }
    // step 6: verify difficulty
    bigint diff;
    if (isArrowGlacier(header)) {
        diff = eth::difficulty::calculate(header.time(), &parentHeader, ArrowGlacierBombDelay);
    } else if (isLondon(header)) {
        diff = eth::difficulty::calculate(header.time(), &parentHeader, LondonBombDelay);
    } else {
        xwarn("[xtop_evm_eth_bridge_contract::sync] unexpected fork");
        xassert(false);
    }
    if (diff != header.difficulty()) {
        return false;
    }
    // step 7: verify ethash
    if (!eth::ethash_util::verify(&header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] ethash verify failed, header: %s", header.hash().hex().c_str());
        return false;
    }
    // step 8: set header
    bigint newSumOfDifficult = preSumOfDifficult + header.difficulty();
    if (!set_header(header, newSumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] set_header failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 9: get last header
    h256 last_hash;
    if (!get_hash(last_height, last_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash failed, height: %s, hash: %s", last_height.str().c_str(), last_hash.hex().c_str());
        return false;
    }
    eth::xeth_block_header_t last_header;
    bigint last_difficulty{0};
    if (!get_header(last_hash, last_header, last_difficulty)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get last header failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    // step 10: set or rebuild
    if (last_header.hash() == header.parentHash()) {
        if (!set_hash(header.number(), header.hash())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_hash failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
            return false;
        }
        if (!set_height(header.number())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_height failed, height: %s", header.number().str().c_str());
            return false;
        }
    } else {
        if (newSumOfDifficult > last_difficulty) {
            if (!rebuild(last_header, header)) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] rebuild failed");
                return false;
            }
        }
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::verifyOwner(const std::string & owner) const {
    // TODO: add whitelist
    return true;
}

bool xtop_evm_eth_bridge_contract::verifyCommon(const eth::xeth_block_header_t & prev_header, const eth::xeth_block_header_t & new_header) const {
    if (new_header.number() != prev_header.number() + 1) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] height mismatch, new: %s, old: %s", new_header.number().str().c_str(), prev_header.number().str().c_str());
        return false;
    }
    if (new_header.extra().size() > MaximumExtraDataSize) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] extra size too big: %zu > 32", new_header.extra().size());
        return false;
    }
    if (new_header.time() <= prev_header.time()) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] time mismatch, new: %lu, old: %lu", new_header.time(), prev_header.time());
        return false;
    }
    if (new_header.gasLimit() > MaxGasLimit) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gaslimit too big: %lu > 0x7fffffffffffffff", new_header.gasLimit());
        return false;
    }
    if (new_header.gasUsed() > new_header.gasLimit()) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gasUsed: %lu > gasLimit: %lu", new_header.gasUsed(), new_header.gasLimit());
        return false;
    }
    if ((new_header.gasLimit() >= prev_header.gasLimit() * 1025 / 1024) || (new_header.gasLimit() <= prev_header.gasLimit() * 1023 / 1024)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gaslimit mismatch, new: %lu, old: %lu", new_header.gasLimit(), prev_header.gasLimit());
        return false;
    }
    return true;
}

// VerifyEip1559Header verifies some header attributes which were changed in EIP-1559,
// - gas limit check
// - basefee check
bool xtop_evm_eth_bridge_contract::verifyEip1559Header(const eth::xeth_block_header_t & parentHeader, const eth::xeth_block_header_t & header) const {
    // Verify that the gas limit remains within allowed bounds
    auto parentGasLimit = parentHeader.gasLimit();
    if (!isLondon(parentHeader)) {
        parentGasLimit = parentHeader.gasLimit() * ElasticityMultiplier;
    }
    if (!verifyGaslimit(parentGasLimit, header.gasLimit())) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
        return false;
    }
    // Verify the header is not malformed
    if (!header.isBaseFee()) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] is not basefee: %d", header.isBaseFee());
        return false;
    }
    // Verify the baseFee is correct based on the parent header.
    auto expectedBaseFee = calcBaseFee(parentHeader);
    if (header.baseFee() != expectedBaseFee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %s, should be: %s", header.baseFee().str().c_str(), expectedBaseFee.str().c_str());
        return false;
    }
    return true;
}

// VerifyGaslimit verifies the header gas limit according increase/decrease
// in relation to the parent gas limit.
bool xtop_evm_eth_bridge_contract::verifyGaslimit(const u256 parentGasLimit, const u256 headerGasLimit) const {
    // Verify that the gas limit remains within allowed bounds
    bigint diff = bigint(parentGasLimit) - bigint(headerGasLimit);
    if (diff < 0) {
        diff *= -1;
    }
    bigint limit = parentGasLimit / GasLimitBoundDivisor;
    if (uint64_t(diff) >= limit) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyGaslimit] diff: %lu >= limit: %s", uint64_t(diff), limit.str().c_str());
        return false;
    }

    if (headerGasLimit < 5000) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyGaslimit] headerGasLimit: %s too small < 5000", headerGasLimit.str().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::isLondon(const eth::xeth_block_header_t & header) const {
    if (header.number() >= LondonBlock) {
        xassert(header.isBaseFee());
        return true;
    }
    return false;
}

bool xtop_evm_eth_bridge_contract::isArrowGlacier(const eth::xeth_block_header_t & header) const {
    return (header.number() >= ArrowGlacierBlock);
}

bigint xtop_evm_eth_bridge_contract::calcBaseFee(const eth::xeth_block_header_t & parentHeader) const {
    if (!isLondon(parentHeader)) {
        return bigint(InitialBaseFee);
    }
    auto parentGasTarget = parentHeader.gasLimit() / ElasticityMultiplier;
    bigint parentGasTargetBig = parentGasTarget;
    bigint baseFeeChangeDenominator = BaseFeeChangeDenominator;
    // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    if (parentHeader.gasUsed() == parentGasTarget) {
        return bigint(parentHeader.baseFee());
    }
    if (parentHeader.gasUsed() > parentGasTarget) {
        bigint gasUsedDelta = bigint(parentHeader.gasUsed() - parentGasTarget);
        bigint x = parentHeader.baseFee() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        if (baseFeeDelta < 1) {
            baseFeeDelta = 1;
        }
        return parentHeader.baseFee() + baseFeeDelta;
    } else {
        // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
        bigint gasUsedDelta = bigint(parentGasTarget - parentHeader.gasUsed());
        bigint x = parentHeader.baseFee() - gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        x = parentHeader.baseFee() - baseFeeDelta;
        if (x < 0) {
            x = 0;
        }

        return x;
    }
    return 0;
}

bool xtop_evm_eth_bridge_contract::get_header(const h256 hash, eth::xeth_block_header_t & header, bigint & difficulty) const {
    auto k = hash.asBytes();
    auto header_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    eth::xeth_block_header_with_difficulty_t header_with_difficulty;
    if (header_with_difficulty.from_string(header_str) < 0) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    header = header_with_difficulty.m_header;
    difficulty = header_with_difficulty.m_difficult_sum;
    xinfo("[xtop_evm_eth_bridge_contract::get_header] get_header success, hash: %lu", hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header(eth::xeth_block_header_t & header, bigint difficulty) {
    eth::xeth_block_header_with_difficulty_t header_with_difficulty{header, difficulty};
    auto k = header.hash().asBytes();
    auto v = header_with_difficulty.to_string();
    if (!m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        xwarn("[xtop_evm_eth_bridge_contract::set_header] set_header failed, header: %s", header.hash().hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_header] set_header success, set header: %s", header.hash().hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::get_hash(const bigint height, h256 & hash) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()});
    if (hash_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_hash] height not exist, height: %s ", height.str().c_str());
        return false;
    }
    xbytes_t v{std::begin(hash_str), std::end(hash_str)};
    hash = static_cast<h256>(v);
    xinfo("[xtop_evm_eth_bridge_contract::get_hash] get_hash success, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_hash(const bigint height, const h256 hash) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (!m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        xwarn("[xtop_evm_eth_bridge_contract::set_hash] set_hash failed, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_hash] set_hash success, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::get_height(bigint & height) const {
    auto height_str = m_contract_state->string_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT);
    if (height_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_height] height empty");
        return false;
    }
    height = static_cast<bigint>(evm_common::fromBigEndian<u256>(height_str));
    xinfo("[xtop_evm_eth_bridge_contract::get_height] get height: %s, get success", height.str().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_height(const bigint height) {
    auto bytes = evm_common::toBigEndian(static_cast<u256>(height));
    if (!m_contract_state->string_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, {bytes.begin(), bytes.end()})) {
        xwarn("[xtop_evm_eth_bridge_contract::set_height] set_height failed, height: %s", height.str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_height] set_height success, height: %s", height.str().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::rebuild(eth::xeth_block_header_t & current_header, eth::xeth_block_header_t & new_header) {
    const bigint current_height = current_header.number();
    bigint new_height = new_header.number();
    h256 new_hash = new_header.hash();

    std::map<xbytes_t, xbytes_t> add;
    bigint remove_cnt{0};

    if (new_height < current_height) {
        for (bigint h = new_height + 1; new_height <= current_height; ++h) {
            auto k = evm_common::toBigEndian(static_cast<u256>(h));
            if (!m_contract_state->map_remove(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()})) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] remove hash error, height: %s", h.str().c_str());
            }
        }
    } else if (new_height > current_height) {
        while (new_height > current_height) {
            eth::xeth_block_header_with_difficulty_t h;
            bigint difficult_sum;
            if (!get_header(new_hash, h.m_header, difficult_sum)) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_header failed, hash: %s", new_hash.hex().c_str());
                return false;
            }
            if (h.m_header.number() != new_height) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] header height mismatch, stored height: %s, new_height: %s",
                      h.m_header.number().str().c_str(),
                      new_height.str().c_str());
                return false;
            }
            add[evm_common::toBigEndian(static_cast<u256>(new_height))] = new_hash.asBytes();
            new_hash = h.m_header.parentHash();
            new_height--;
        }
        xassert(new_height == current_height);
    }

    h256 new_fork_hash{new_hash};
    h256 same_height_hash{0};
    if (!get_hash(new_height, same_height_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] get hash failed, height: %s", new_height.str().c_str());
        return false;
    }

    while (new_fork_hash != same_height_hash) {
        add[evm_common::toBigEndian(static_cast<u256>(new_height))] = new_hash.asBytes();
        new_height--;
        // get previous new_fork_hash
        eth::xeth_block_header_with_difficulty_t h;
        if (!get_header(new_hash, h.m_header, h.m_difficult_sum)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get last header failed, hash: %s", new_hash.hex().c_str());
            return false;
        }
        new_fork_hash = h.m_header.parentHash();
        // get previous same_height_hash
        if (!get_hash(new_height, same_height_hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_hash failed, hash: %s", new_height.str().c_str());
            return false;
        }
    }

    for (auto const & a : add) {
        if (!m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {a.first.begin(), a.first.end()}, {a.second.begin(), a.second.end()})) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] set hash error");
        }
    }

    if (!set_height(new_header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] set_height failed, height: %s", new_header.number().str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::rebuild] rebuild success");

    return true;
}

NS_END4
