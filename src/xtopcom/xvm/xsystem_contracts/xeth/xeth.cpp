#include "xeth.h"
#include "ethash_util.h"
#include "xevm_common/rlp.h"
#include <array>
NS_BEG4(top, xvm, system_contracts, xeth)

using namespace top::evm_common;
using namespace top::evm_common::rlp;

bool xeth_bridge_t::init_genesis_block_header(std::string headerContent, std::string emitter) {
    xeth_block_header_t header;
    bool result = header.fromJson(headerContent);
    if (!result) {
        return false;
    }
    
    h256 hash;
    result = m_store.getHashOfMainChainByHeight((uint64_t)ETH_CHAIN_ID, header.number(), hash); 
    if (result) {
        return false;
    }

    if (!validateOwner(emitter)) {
        return false;
    }

    bigint sumOfDifficult = header.difficulty();
    m_store.saveBlock(header, (u256)sumOfDifficult, ETH_CHAIN_ID);

    result = m_store.saveMainChain(ETH_CHAIN_ID, header.number(), header.hash());
    if (!result) {
        return false;
    }

    return true;
}

bool xeth_bridge_t::sync_block_header(std::string headerContent){
    xeth_block_header_t header;
    bool result = header.fromJson(headerContent);
    if (!result) {
        return false;
    }

    /* calculate block header hash */
    result = m_store.isBlockExist(ETH_CHAIN_ID, header.hash());
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

    auto diff = difficulty::calculate(parentHeader.time(), &header);
    if (diff != header.difficulty()) {
        return false;
    }

    result = ethash_util::verify(&header);
    if (!result) {
        return false;
    }

    bigint bigSumOfDifficult;
    bigSumOfDifficult = bigint(sumOfDifficult) + header.difficulty();
    m_store.saveBlock(header, (u256)bigSumOfDifficult, ETH_CHAIN_ID);

    int64_t height;
    result = m_store.getCurrentHeightOfMainChain(ETH_CHAIN_ID, height);
    if (!result) {
        return false;
    }

    h256 hash;
    result = m_store.getHashOfMainChainByHeight(ETH_CHAIN_ID, height, hash); {
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

uint64_t xeth_bridge_t::getCurrentHeightOfMainChain(uint64_t chainID) {
    int64_t mainHeight = 0;
    bool result = m_store.getCurrentHeightOfMainChain(chainID, mainHeight);
    if (!result) {
        return 0;
    }

    return (uint64_t)mainHeight;
}

uint8_t* xeth_bridge_t::getHashOfMainChainByHeight(uint64_t chainID, int64_t height) {
    h256 hash;
    bool result = m_store.getHashOfMainChainByHeight(chainID, height, hash);
    if (!result) {
        return NULL;
    }

    return hash.data();
}

bool xeth_bridge_t::getHeaderIfHeightConfirmed(int64_t height, xeth_block_header_t header, uint64_t chainID) {
    int64_t mainHeight = 0;
    xeth_block_header_t chainHeader;
    bool result = m_store.getCurrentHeightOfMainChain(chainID, mainHeight);
    if (!result) {
        return false;
    }

    if (mainHeight <= header.number() + CONFIRM_HEIGHTS) {
        return false;
    }

    h256 hash;
    result = m_store.getHashOfMainChainByHeight(chainID, header.number(), hash); 
    if (result) {
        return false;
    }

    xeth_block_header_t mainHeader;
    u256 sumOfDifficult1 = 0;
    result = m_store.getBlockbyHash(ETH_CHAIN_ID, hash, mainHeader, sumOfDifficult1);
    if (result) {
        return false;
    }

    if (header.stateMerkleRoot() != mainHeader.stateMerkleRoot()) {
        return false;
    }

    if (header.txMerkleRoot() != mainHeader.txMerkleRoot()) {
        return false;
    }

    if (header.receiptMerkleRoot() != mainHeader.receiptMerkleRoot()) {
        return false;
    }

    return true;
}

bool xeth_bridge_t::verify(xeth_block_header_t prev_header, xeth_block_header_t new_header) {
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

bool xeth_bridge_t::validateOwner(std::string owner) {
    return true;
}

bool xeth_bridge_t::isLondonFork(int64_t height) {
    return true;
}

// VerifyGaslimit verifies the header gas limit according increase/decrease
// in relation to the parent gas limit.
bool xeth_bridge_t::VerifyGaslimit(u256 parentGasLimit, u256 headerGasLimit ){
    // Verify that the gas limit remains within allowed bounds
    u256 diff = parentGasLimit - headerGasLimit;
    if (diff < 0) {
        diff *= -1;
    }

    u256 limit = parentGasLimit / 1024;
    if (diff >= limit) {
        return false;
    }
    if (headerGasLimit < 5000) {
        return false;
    }
    return true;
}

bigint xeth_bridge_t::CalcBaseFee(xeth_block_header_t &parentHeader)  {
    if (!isLondonFork(parentHeader.number())) {
        return bigint(1000000000);
    }

    // uint64_t targetGas = parentHeader.gasLimit() / 2;
    // bigint targetGas = bitint(targetGas);
    // bigint baseFeeChangeDenominator = bigint(8);

    // // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    // if (parentHeader.gasUsed() == targetGas) {
    //     return bigint(parentHeader.baseFee());
    // }

    // if (parent.gasUsed > targetGas) {
    //     bigint gasUsedDelta = bigint(parent.gasUsed() - targetGas);
    //     x := new(big.Int).Mul(parent.BaseFee, gasUsedDelta)
    //     y := x.Div(x, parentGasTargetBig)
    //     baseFeeDelta := math.BigMax(
    //         x.Div(y, baseFeeChangeDenominator),
    //         common.Big1,
    //     )

    //     return x.Add(parent.BaseFee, baseFeeDelta)
    // } else {
    //     // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
    //     gasUsedDelta := new(big.Int).SetUint64(parentGasTarget - parent.GasUsed)
    //     x := new(big.Int).Mul(parent.BaseFee, gasUsedDelta)
    //     y := x.Div(x, parentGasTargetBig)
    //     baseFeeDelta := x.Div(y, baseFeeChangeDenominator)

    //     return math.BigMax(
    //         x.Sub(parent.BaseFee, baseFeeDelta),
    //         common.Big0,
    //     )
    // }
    return 0;
}
NS_END4
