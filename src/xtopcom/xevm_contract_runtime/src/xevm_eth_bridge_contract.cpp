// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xeth/xeth_difficulty.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xethash_util.h"

#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xdata_common.h"

#include <cinttypes>

#define ETH_CHAIN_ID 1

#define ARROWGLACIER_BLOCK 13773000
#define ARROWGLACIER_BOMBDELAY 10700000
#define LONDON_BLOCK 12965000
#define LONDON_BOMBDELAY 9700000

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace top::evm_common;

bool xtop_evm_eth_bridge_contract::execute(xbytes_t input,
                                           uint64_t target_gas,
                                           sys_contract_context const & context,
                                           bool is_static,
                                           observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                           sys_contract_precompile_output & output,
                                           sys_contract_precompile_error & err) {
    assert(state_ctx);
    m_contract_state = state_ctx->load_unit_state(m_contract_address.vaccount());
    if (m_contract_state == nullptr) {
        return false;
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::init(std::string headerContent, std::string emitter) {
    // step 0: check int flag
    if (m_initialized) {
        xwarn("[xtop_evm_eth_bridge_contract::init] init already");
        return false;
    }
    // step 1: check emitter
    if (!validateOwner(emitter)) {
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
    if (get_hash(ETH_CHAIN_ID, header.number(), hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] get_hash existed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), hash.hex().c_str());
        return false;
    }
    // step 4: store block
    if (!set_header(ETH_CHAIN_ID, header, header.difficulty())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header failed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 5: store mainchain header
    if (!set_hash(ETH_CHAIN_ID, header.number(), header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_hash failed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 6: store height
    if (!set_height(ETH_CHAIN_ID, header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_height failed, chain: %d, height: %s", ETH_CHAIN_ID, header.number().str().c_str());
        return false;
    }
    // step 7: set init flag
    m_initialized = true;

    return true;
}

bool xtop_evm_eth_bridge_contract::sync(std::string headerContent) {
    // step 1: check init flag
    if (!m_initialized) {
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
    if (get_hash(ETH_CHAIN_ID, header.number(), exist_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash existed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), exist_hash.hex().c_str());
        return false;
    }
    // step 4: get parent header
    eth::xeth_block_header_t parentHeader;
    bigint preSumOfDifficult{0};
    if (!get_header(ETH_CHAIN_ID, header.parentHash(), parentHeader, preSumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, chain: %d, hash: %s", ETH_CHAIN_ID, header.parentHash().hex().c_str());
        return false;
    }
    // step 5: verify header common
    if (!verifyCommon(parentHeader, header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] verify header common failed, header: %s, parrent header: %s", header.hash().hex().c_str(), parentHeader.hash().hex());
        return false;
    }
    // if (isLondon(header)) {
    //     if (!verifyEip1559Header(parentHeader, header)) {
    //         return false;
    //     }
    // } else {
    //     if (!verifyGaslimit(parentHeader.gasLimit(), header.gasLimit())) {
    //         return false;
    //     }
    // }
    // step 6: verify difficulty
    bigint diff;
    if (isArrowGlacier(header)) {
        diff = eth::difficulty::calculate(header.time(), &parentHeader, ARROWGLACIER_BOMBDELAY);
    } else if (isLondon(header)) {
        diff = eth::difficulty::calculate(header.time(), &parentHeader, LONDON_BOMBDELAY);
    } else {
        xwarn("[xtop_evm_eth_bridge_contract::sync] unexpected fork");
        xassert(false);
    }
    if (diff != header.difficulty()) {
        return false;
    }
    // step 7: verify ethash
    if (!eth::ethash_util::verify(&header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] ethash verify failed, header: %s", ETH_CHAIN_ID, header.hash().hex().c_str());
        return false;
    }
    // step 8: set header
    bigint newSumOfDifficult = preSumOfDifficult + header.difficulty();
    if (!set_header(ETH_CHAIN_ID, header, newSumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] set_header failed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    // step 9: get last header
    bigint height{0};
    if (!get_height(ETH_CHAIN_ID, height)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_height failed, chain: %d, height: %s", ETH_CHAIN_ID, height.str().c_str());
        return false;
    }
    h256 hash;
    if (!get_hash(ETH_CHAIN_ID, height, hash)){
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash failed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, height.str().c_str(), hash.hex().c_str());
        return false;
    }
    eth::xeth_block_header_t last_header;
    bigint last_difficulty{0};
    if (!get_header(ETH_CHAIN_ID, hash, last_header, last_difficulty)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get last header failed, chain: %d, hash: %s", ETH_CHAIN_ID, hash.hex().c_str());
        return false;
    }
    // step 10: set or rebuild
    if (last_header.hash() == header.parentHash()) {
        if (!set_hash(ETH_CHAIN_ID, header.number(), header.hash())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_hash failed, chain: %d, height: %s, hash: %s", ETH_CHAIN_ID, header.number().str().c_str(), header.hash().hex().c_str());
            return false;
        }
        if (!set_height(ETH_CHAIN_ID, header.number())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_height failed, chain: %d, height: %s", ETH_CHAIN_ID, header.number().str().c_str());
            return false;
        }
    } else {
        if (newSumOfDifficult > last_difficulty) {
            if (!rebuild(ETH_CHAIN_ID, last_header, header)) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] rebuild failed");
                return false;
            }
        }
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::validateOwner(std::string owner) {
    return true;
}

bool xtop_evm_eth_bridge_contract::verifyCommon(eth::xeth_block_header_t prev_header, eth::xeth_block_header_t new_header) {
    if (new_header.number() != prev_header.number() + 1) {
        return false;
    }

    if (new_header.extra().size() > 32) {
        return false;
    }

    if (new_header.time() <= prev_header.time()) {
        return false;
    }

    if (new_header.gasLimit() > (uint64_t)0x7fffffffffffffff) {
        return false;
    }

    if (new_header.gasUsed() > new_header.gasLimit()) {
        return false;
    }

    if ((new_header.gasLimit() > prev_header.gasLimit() * 1025/1024) || 
        (new_header.gasLimit() <= prev_header.gasLimit() * 1023 / 1024)) {
        return false;
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::verifyEip1559Header(eth::xeth_block_header_t &parentHeader, eth::xeth_block_header_t &header) {
	// Verify that the gas limit remains within allowed bounds
	u256 parentGasLimit = parentHeader.gasLimit();
	if (!isLondon(parentHeader)) {
		parentGasLimit = parentHeader.gasLimit() * 2;
	}

	if (!verifyGaslimit(parentGasLimit, header.gasLimit())) {
		return false;
	}

	// Verify the header is not malformed
	if (!header.isBaseFee()) {
		return false;
	}
	// Verify the baseFee is correct based on the parent header.
	bigint expectedBaseFee = calcBaseFee(parentHeader);
	if (header.baseFee() != expectedBaseFee) {
		return false;
	}
	return true;
}

// VerifyGaslimit verifies the header gas limit according increase/decrease
// in relation to the parent gas limit.
bool xtop_evm_eth_bridge_contract::verifyGaslimit(u256 parentGasLimit, u256 headerGasLimit ){
    // Verify that the gas limit remains within allowed bounds
    bigint diff = (bigint)parentGasLimit - (bigint)headerGasLimit;
    if (diff < 0) {
        diff *= -1;
    }

    bigint limit = (bigint)parentGasLimit / 1024;
    if (diff >= limit) {
        return false;
    }

    if (headerGasLimit < 5000) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::isLondon(eth::xeth_block_header_t &header) const {
    if (header.number() >= LONDON_BLOCK) {
        xassert(header.isBaseFee());
        return true;
    }

    return false;
}

bool xtop_evm_eth_bridge_contract::isArrowGlacier(eth::xeth_block_header_t &header) const {
    return (header.number() >= ARROWGLACIER_BLOCK);
}

bigint xtop_evm_eth_bridge_contract::calcBaseFee(eth::xeth_block_header_t &parentHeader)  {
    if (!isLondon(parentHeader)) {
        return bigint(1000000000);
    }

    bigint parentGasTarget = ((bigint)parentHeader.gasLimit()) / 2;
    bigint parentGasTargetBig = (bigint)parentGasTarget;
    bigint baseFeeChangeDenominator = bigint(8);

    // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    if (parentHeader.gasUsed() == parentGasTarget) {
        return bigint(parentHeader.baseFee());
    }

    if ((bigint)parentHeader.gasUsed() > parentGasTarget) {
        bigint gasUsedDelta = (bigint)parentHeader.gasUsed() - parentGasTarget;
        bigint x = parentHeader.baseFee() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        if (baseFeeDelta < 1) {
            baseFeeDelta = 1;
        }
        return parentHeader.baseFee() + baseFeeDelta;
    } else {
        // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
        bigint gasUsedDelta = parentGasTarget - (bigint)parentHeader.gasUsed();
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

bool xtop_evm_eth_bridge_contract::get_header(const uint64_t chain_id, const h256 hash, eth::xeth_block_header_t & header, bigint & difficulty) const {
    std::map<xbytes_t, std::string> header_map;
    auto header_map_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, top::to_string(chain_id));
    if (header_map_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] chain: %d not exist", chain_id);
        return false;
    }
    base::xstream_t header_map_stream(base::xcontext_t::instance(), (uint8_t *)header_map_str.data(), header_map_str.size());
    MAP_DESERIALIZE_SIMPLE(header_map_stream, header_map);
    if (!header_map.count(hash.asBytes())) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] get_header not exist, chain: %d, hash: %s", chain_id, hash.hex().c_str());
        return false;
    }
    eth::xeth_block_header_with_difficulty_t header_with_difficulty;
    header_with_difficulty.from_string(header_map[hash.asBytes()]);
    header = header_with_difficulty.m_header;
    difficulty = header_with_difficulty.m_difficult_sum;
    xinfo("[xtop_evm_eth_bridge_contract::get_header] get_header success, chain: %d, hash: %lu", chain_id, hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header(const uint64_t chain_id, eth::xeth_block_header_t header, bigint difficulty) {
    std::map<xbytes_t, std::string> header_map;
    h256 hash = header.hash();
    eth::xeth_block_header_with_difficulty_t header_with_difficulty{header, difficulty};
    auto k = hash.asBytes();
    auto v = header_with_difficulty.to_string();
    auto header_map_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, top::to_string(chain_id));
    if (header_map_str.empty()) {
        header_map.insert({k, v});
    } else {
        base::xstream_t header_map_stream(base::xcontext_t::instance(), (uint8_t *)header_map_str.data(), header_map_str.size());
        MAP_DESERIALIZE_SIMPLE(header_map_stream, header_map);
        if (header_map.count(k)) {
            header_map[k] = v;
        } else {
            header_map.insert({k, v});
        }
    }
    base::xstream_t header_map_stream(base::xcontext_t::instance());
    MAP_SERIALIZE_SIMPLE(header_map_stream, header_map);
    auto header_map_stream_str = std::string{reinterpret_cast<const char *>(header_map_stream.data()), static_cast<std::string::size_type>(header_map_stream.size())};
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, top::to_string(chain_id), header_map_stream_str);
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::set_header] set_header failed, chain: %d, header: %s", chain_id, hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_header] set_header success, chain: %d, set header: %s", chain_id, hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::get_hash(const uint64_t chain_id, const bigint height, h256 & hash) const {
    std::map<xbytes_t, xbytes_t> info;
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id));
    if (info_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_hash] chain: %d not exist", chain_id);
        return false;
    }
    base::xstream_t info_stream(base::xcontext_t::instance(), (uint8_t *)info_str.data(), info_str.size());
    MAP_DESERIALIZE_SIMPLE(info_stream, info);
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (!info.count(k)) {
        xwarn("[xtop_evm_eth_bridge_contract::get_hash] height not exist, chain: %d, height: %s ", chain_id, height.str().c_str());
        return false;
    }
    hash = static_cast<h256>(info[k]);
    xinfo("[xtop_evm_eth_bridge_contract::get_hash] get_hash success, chain: %d, height: %s, hash: %s", chain_id, height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_hash(const uint64_t chain_id, const bigint height, const h256 hash) {
    std::map<xbytes_t, xbytes_t> info;
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id));
    if (info_str.empty()) {
        info.insert({k, hash.asBytes()});
    } else {
        base::xstream_t info_stream(base::xcontext_t::instance(), (uint8_t *)info_str.data(), info_str.size());
        MAP_DESERIALIZE_SIMPLE(info_stream, info);
        if (info.count(k)) {
            info[k] = hash.asBytes();
        } else {
            info.insert({k, hash.asBytes()});
        }
    }
    base::xstream_t info_stream(base::xcontext_t::instance());
    MAP_SERIALIZE_SIMPLE(info_stream, info);
    auto info_stream_str = std::string{reinterpret_cast<const char *>(info_stream.data()), static_cast<std::string::size_type>(info_stream.size())};
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id), info_stream_str);
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::set_hash] set_hash failed, chain: %d, height: %s, hash: %s", chain_id, height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_hash] set_hash success, chain: %d, height: %s, hash: %s", chain_id, height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::get_height(const uint64_t chain_id, bigint & height) const {
    auto height_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, top::to_string(chain_id));
    if (height_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_height] chain: %d height not exist", chain_id);
        return false;
    }
    height = static_cast<bigint>(evm_common::fromBigEndian<u256>(height_str));
    xinfo("[xtop_evm_eth_bridge_contract::get_height] chain: %d, get height: %s, get success", chain_id, height.str().c_str(), height);
    return true;
}

bool xtop_evm_eth_bridge_contract::set_height(const uint64_t chain_id, const bigint height) {
    auto bytes = evm_common::toBigEndian(static_cast<u256>(height));
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, top::to_string(chain_id), {bytes.begin(), bytes.end()});
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::set_height] set_height failed, chain: %d, height: %s", chain_id, height.str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_height] set_height success, chain: %d, height: %s", chain_id, height.str().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::rebuild(uint64_t chain_id, eth::xeth_block_header_t & current_header, eth::xeth_block_header_t & new_header) {
    auto const current_height = current_header.number();
    auto new_height = new_header.number();
    auto new_hash = new_header.hash();

    std::map<bigint, h256> add;
    bigint remove_cnt{0};
    
    if (new_height < current_height) {
        remove_cnt = current_height - new_height;
    } else if (new_height > current_height) {
        while (new_height > current_height) {
            eth::xeth_block_header_with_difficulty_t h;
            bigint difficult_sum;
            if (!get_header(chain_id, new_hash, h.m_header, difficult_sum)) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_header failed, chain_id: %d, hash: %s", chain_id, new_hash.hex().c_str());
                return false;
            }
            if (h.m_header.number() != new_height) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] header height mismatch, stored height: %s, new_height: %s", h.m_header.number().str().c_str(), new_height.str().c_str());
                return false;
            }
            add[new_height] = new_hash;
            new_hash = h.m_header.parentHash();
            new_height--;
        }
        xassert(new_height == current_height);
    }

    h256 new_fork_hash{new_hash};
    h256 same_height_hash{0};
    if (!get_hash(chain_id, new_height, same_height_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] get hash failed, chain: %d , height: %s", chain_id, new_height);
        return false;
    }
    
    while (new_fork_hash != same_height_hash) {
        add[new_height] = new_hash;
        new_height--;
        // get previous new_fork_hash
        eth::xeth_block_header_with_difficulty_t h;
        if (!get_header(chain_id, new_hash, h.m_header, h.m_difficult_sum)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get last header failed, chain: %d, hash: %s", chain_id, new_hash.hex().c_str());
            return false;
        }
        new_fork_hash = h.m_header.parentHash();
        // get previous same_height_hash
        if (!get_hash(chain_id, new_height, same_height_hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_hash failed, chain: %d, hash: %s", chain_id, new_height.str().c_str());
            return false;
        }
    }

    std::map<xbytes_t, xbytes_t> info;
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id));
    if (info_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] chain: %d hash not exist", chain_id);
        return false;
    }
    {
        base::xstream_t info_stream(base::xcontext_t::instance(), (uint8_t *)info_str.data(), info_str.size());
        MAP_DESERIALIZE_SIMPLE(info_stream, info);
    }
    for (bigint h = 0; h < remove_cnt; h++) {
        info.erase(evm_common::toBigEndian(static_cast<u256>(current_height - h)));
    }
    for (auto const & a : add) {
        info.insert({evm_common::toBigEndian(static_cast<u256>(a.first)), a.second.asBytes()});
    }
    base::xstream_t info_stream(base::xcontext_t::instance());
    MAP_SERIALIZE_SIMPLE(info_stream, info);
    auto info_stream_str = std::string{reinterpret_cast<const char *>(info_stream.data()), static_cast<std::string::size_type>(info_stream.size())};
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id), info_stream_str);
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] batch set_hash failed");
        return false;
    }

    if (!set_height(chain_id, new_header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] set_height failed, chain_id: %d, height: %s", chain_id, new_header.number().str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::rebuild] rebuild success");

    return true;
}

NS_END4
