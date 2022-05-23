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
#define CONFIRM_HEIGHTS 25

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace top::evm_common::eth;

struct xeth_block_header_with_difficulty_t {
    xeth_block_header_t m_header;
    u256 m_difficult_sum;
};

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
    // step 1: check emitter
    if (!validateOwner(emitter)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] emitter: %s invalid", emitter.c_str());
        return false;
    }
    // step 2: header
    xeth_block_header_t header;
    if (!header.fromJson(headerContent)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] decode header error");
        return false;
    }
    // step 3: check exist
    h256 hash{0};
    if (get_hash(ETH_CHAIN_ID, header.number(), hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] chain: %d, height: %lu, hash existed", ETH_CHAIN_ID, header.number());
        return false;
    }
    // step 4: store block
    if (!m_store.saveBlock(header, u256(header.difficulty()), ETH_CHAIN_ID)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] chain: %d, header of height: %lu, store failed", ETH_CHAIN_ID, header.number());
        return false;
    }
    // step 5: store mainchain header
    if (!set_hash(ETH_CHAIN_ID, header.number(), header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] chain: %d, height: %lu, hash store failed", ETH_CHAIN_ID, header.number());
        return false;
    }
    // step 6: store height
    if (!set_height(ETH_CHAIN_ID, header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] chain: %d, height: %lu, store failed", ETH_CHAIN_ID, header.number());
        return false;
    }
    // step 6: set init flag
    m_initialized = true;

    return true;
}

bool xtop_evm_eth_bridge_contract::sync(std::string headerContent) {
    if (!m_initialized) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not init yet");
        return false;
    }
    
    xeth_block_header_t header;
    if (!header.fromJson(headerContent)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] decode header error");
        return false;
    }

    /* calculate block header hash */
    bool result = m_store.isBlockExist(ETH_CHAIN_ID, header.hash());
    if (result) {
        return false;
    }

    xeth_block_header_t parentHeader;
    u256 sumOfDifficult = 0;
    result = m_store.getBlockbyHash(ETH_CHAIN_ID, header.parentHash(), parentHeader, sumOfDifficult);
    if (!result) {
        return false;
    }

    result = verify(parentHeader, header);
    if (!result) {
        return false;
    }

    if (isLondonFork(header)) {
        if (!verifyEip1559Header(parentHeader, header)) {
            return false;
        }
    } else {
        if (!verifyGaslimit(parentHeader.gasLimit(), header.gasLimit())) {
            return false;
        }
    }

    bigint diff;
    if (isArrowGlacier(header.number())) {
        diff = difficulty::calculate(header.time(), &parentHeader, 1070000);
    } else if (isLondonFork(header)) {
        diff = difficulty::calculate(header.time(), &parentHeader, 9700000);
    } else {
        diff = difficulty::calculate(header.time(), &parentHeader);
    }
    
    if (diff != header.difficulty()) {
        return false;
    }

    result = evm_common::eth::ethash_util::verify(&header);
    if (!result) {
        return false;
    }

    bigint bigSumOfDifficult;
    bigSumOfDifficult = bigint(sumOfDifficult) + header.difficulty();
    m_store.saveBlock(header, (u256)bigSumOfDifficult, ETH_CHAIN_ID);

    uint64_t height;
    if (!get_height(ETH_CHAIN_ID, height)) {
        return false;
    }

    h256 hash;
    if (!get_hash(ETH_CHAIN_ID, height, hash)){
        return false;
    }

    xeth_block_header_t mainHeader;
    u256 sumOfDifficult1 = 0;
    result = m_store.getBlockbyHash(ETH_CHAIN_ID, hash, mainHeader, sumOfDifficult1);
    if (!result) {
        return false;
    }

    if (mainHeader.hash() == header.parentHash()) {
        result = m_store.saveMainChain(ETH_CHAIN_ID, header.number(), header.hash());
        if (!result) {
            return false;
        }
    } else {
        if (bigSumOfDifficult > (bigint)sumOfDifficult1) {
            result = m_store.rebuildMainChain(ETH_CHAIN_ID, mainHeader, header);
            if (!result) {
                return false;
            }
        }
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::validateOwner(std::string owner) {
    return true;
}

bool xtop_evm_eth_bridge_contract::verify(xeth_block_header_t prev_header, xeth_block_header_t new_header) {
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

bool xtop_evm_eth_bridge_contract::verifyEip1559Header(xeth_block_header_t &parentHeader, xeth_block_header_t &header) {
	// Verify that the gas limit remains within allowed bounds
	u256 parentGasLimit = parentHeader.gasLimit();
	if (!isLondonFork(parentHeader)) {
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

bool xtop_evm_eth_bridge_contract::isLondonFork(xeth_block_header_t &header) {
    if (header.isBaseFee() || (header.number() >= ETH1559_HEIGHT)) {
        return true;
    }

    return false;
}

bool xtop_evm_eth_bridge_contract::isArrowGlacier(int64_t height) {
    if (height >= ETH4345_HEIGHT) {
        return true;
    }
	return false;
}

bigint xtop_evm_eth_bridge_contract::calcBaseFee(xeth_block_header_t &parentHeader)  {
    if (!isLondonFork(parentHeader)) {
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

bool xtop_evm_eth_bridge_contract::get_hash(const uint64_t chain_id, const uint64_t height, h256 & hash) const {
    std::map<uint64_t, xbytes_t> info;
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id));
    if (info_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_hash] chain: %d hash not exist", chain_id);
        return false;
    }
    base::xstream_t info_stream(base::xcontext_t::instance(), (uint8_t *)info_str.data(), info_str.size());
    MAP_DESERIALIZE_SIMPLE(info_stream, info);
    if (!info.count(height)) {
        xwarn("[xtop_evm_eth_bridge_contract::get_hash] chain: %d, height: %lu not exist", chain_id, height);
        return false;
    }
    hash = h256(info[height]);
    xinfo("[xtop_evm_eth_bridge_contract::get_hash] chain: %d, height: %lu, hash: %s", chain_id, height, hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::set_hash(const uint64_t chain_id, const uint64_t height, const h256 hash) {
    std::map<uint64_t, xbytes_t> info;
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(chain_id));
    if (info_str.empty()) {
        info.insert({height, hash.asBytes()});
    } else {
        base::xstream_t info_stream(base::xcontext_t::instance(), (uint8_t *)info_str.data(), info_str.size());
        MAP_DESERIALIZE_SIMPLE(info_stream, info);
        if (info.count(height)) {
            info[height] = hash.asBytes();
        } else {
            info.insert({height, hash.asBytes()});
        }
    }
    base::xstream_t info_stream(base::xcontext_t::instance());
    MAP_SERIALIZE_SIMPLE(info_stream, info);
    auto info_stream_str = std::string{reinterpret_cast<const char *>(info_stream.data()), static_cast<std::string::size_type>(info_stream.size())};
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, top::to_string(height), info_stream_str);
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::set_hash] chain: %d, height: %lu, set hash: %s failed", chain_id, height, hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_hash] chain: %d, height: %lu, set hash: %s success", chain_id, height, hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::get_height(const uint64_t chain_id, uint64_t & height) const {
    std::map<uint64_t, xbytes_t> info;
    auto height_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, top::to_string(chain_id));
    if (height_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_height] chain: %d height not exist", chain_id);
        return false;
    }
    height = top::from_string<uint64_t>(height_str);
    xinfo("[xtop_evm_eth_bridge_contract::get_height] chain: %d, get height: %lu success", chain_id, height, height);
    return true;
}

bool xtop_evm_eth_bridge_contract::set_height(const uint64_t chain_id, const uint64_t height) {
    auto ret = m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, top::to_string(chain_id), top::to_string(height));
    if (ret != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::set_height] chain: %d, height: %lu, set failed", chain_id, height);
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::set_height] chain: %d, height: %lu, set success", chain_id, height);
    return true;

    h256    m_parentHash;
    h256    m_uncleHash;
    Address   m_miner;
    h256    m_stateMerkleRoot;
    h256    m_txMerkleRoot;
    h256    m_receiptMerkleRoot;
    LogBloom  m_bloom;
    bigint    m_difficulty;
    int64_t    m_number;
    u256    m_gasLimit;
    u256    m_gasUsed;
    int64_t  m_time;
    std::vector<uint8_t>  m_extra;
    h256    m_mixDigest;
    h64   m_nonce;
}



NS_END4
