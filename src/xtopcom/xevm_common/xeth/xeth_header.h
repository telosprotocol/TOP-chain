#pragma once

#include <vector>
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"
#include "xdepends/include/json/reader.h"

NS_BEG3(top, evm_common, eth)

// The log bloom's size (2048-bit).
using namespace top::evm_common;
using Hash = h256;
using Address = h160;
using LogBloom = h2048;
using BlockNonce = h64;
class xeth_block_header_t {
#define convertStringFromJson(jsonValue, type, address) \
    if ((jsonValue).empty()) {\
        return false;\
    }\
    (address) = (type)((jsonValue).asString());

#define convertUint64FromJson(jsonValue, address) \
    if ((jsonValue).empty()) {\
        return false;\
    }\
    (address) = strtoull((jsonValue).asString().c_str(), 0, 0);

public:
    xeth_block_header_t(){}
    bool fromJson(const std::string& content);

    // encode and decode
    std::string to_string();
    int from_string(const std::string & s);
    int from_rlp(const xbytes_t & bytes);

    // member of header
    Hash parentHash() const;
    Hash uncle_hash() const;
    Address miner() const;
    Hash stateMerkleRoot() const;
    Hash txMerkleRoot() const;
    Hash receiptMerkleRoot() const;
    LogBloom logBloom() const;
    bigint difficulty() const;
    bigint number() const;
    uint64_t gasLimit() const;
    uint64_t gasUsed() const;
    uint64_t time() const;
    bytes extra() const;
    Hash mixDigest() const;
    BlockNonce nonce() const;
    bigint baseFee() const;
    Hash hash();
    bool isBaseFee() const;
    Hash hashWithoutSeal();

private:
    bytes encode_rlp();
    bytes encode_rlp_withoutseal();
private:
    Hash m_parentHash;
    Hash m_uncleHash;
    Address m_miner;
    Hash m_stateMerkleRoot;
    Hash m_txMerkleRoot;
    Hash m_receiptMerkleRoot;
    LogBloom m_bloom;
    bigint m_difficulty;
    bigint m_number;
    uint64_t m_gasLimit;
    uint64_t m_gasUsed;
    uint64_t m_time;
    bytes m_extra;
    Hash m_mixDigest;
    BlockNonce m_nonce;

    // BaseFee was added by EIP-1559 and is ignored in legacy headers.
    bigint m_baseFee;

    // Peculiar data
    Hash m_hash;
    bool m_hashed = false;
    bool m_isBaseFee = false;
};

class xeth_block_header_with_difficulty_t {
public:
    xeth_block_header_with_difficulty_t() = default;
    xeth_block_header_with_difficulty_t(xeth_block_header_t header, bigint difficulty);

    std::string to_string();
    int from_string(const std::string & s);

    xeth_block_header_t m_header;
    bigint m_difficult_sum;
};

NS_END3
