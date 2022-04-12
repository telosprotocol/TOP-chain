#pragma once

#include <vector>
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"
#include "xbase/xns_macro.h"
#include "xdepends/include/json/reader.h"

NS_BEG4(top, xvm, system_contracts, xeth)

// The log bloom's size (2048-bit).
using namespace top::evm_common;
using Address = h160;
using LogBloom = h2048;
class xeth_block_header_t {
#define convertStringFromJson(jsonValue, type, address) \
    if ((jsonValue).empty()) {\
        return false;\
    }\
    (address) = (type)((jsonValue).asString());

#define convertInt64FromJson(jsonValue, address) \
    if ((jsonValue).empty()) {\
        return false;\
    }\
    (address) = ((jsonValue).asInt64());

public:
    xeth_block_header_t(){}
    bool fromJson(const std::string& content);
    h256 parentHash();
    h256 uncle_hash();
    Address miner();
    h256 stateMerkleRoot();
    h256 txMerkleRoot();
    h256 receiptMerkleRoot();
    LogBloom logBloom();
    bigint difficulty();
    int64_t number();
    u256 gasLimit();
    u256 gasUsed();
    int64_t time();
    std::vector<uint8_t> extra();
    h256 mixDigest();
    int64_t nonce();
    bigint baseFee();
    h256 hash();

private:
    bytes encode_rlp();

private:
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
    int64_t   m_nonce;

	// BaseFee was added by EIP-1559 and is ignored in legacy headers.
	bigint    m_baseFee; //BaseFee *big.Int `json:"baseFeePerGas" rlp:"optional"`
    h256    m_hash;
    bool    m_hashed = false;
};

NS_END4
