#include "xdepends/include/json/reader.h"
#include "xeth_header.h"
#include "xevm_common/rlp.h"
#include "util.h"
#include "xutility/xhash.h"
// The log bloom's size (2048-bit).
NS_BEG4(top, xvm, system_contracts, xeth)

using namespace top::evm_common;
using namespace top::evm_common::rlp;
bool xeth_block_header_t::fromJson(const std::string& content) {
    xJson::Reader reader;
    xJson::Value root;
    std::string value;
    // xJson::value value;
    auto result = reader.parse(content, root);
    if (result != 0) {
        return false;
    }

    convertStringFromJson(root["parentHash"], h256, m_parentHash)
    convertStringFromJson(root["sha3Uncles"], h256, m_uncleHash)
    convertStringFromJson(root["miner"], Address, m_miner)
    convertStringFromJson(root["stateRoot"], h256, m_stateMerkleRoot)
    convertStringFromJson(root["transactionsRoot"], h256, m_txMerkleRoot)
    convertStringFromJson(root["receiptsRoot"], h256, m_receiptMerkleRoot)
    convertStringFromJson(root["logsBloom"], LogBloom, m_bloom)
    convertStringFromJson(root["difficulty"], bigint, m_difficulty)
    convertInt64FromJson(root["number"], m_number)
    convertStringFromJson(root["gasLimit"], u256, m_gasLimit)
    convertStringFromJson(root["gasUsed"], u256, m_gasUsed)
    convertInt64FromJson(root["timestamp"], m_time)
    convertStringFromJson(root["gasLimit"], u256, m_gasLimit)
    if (root["extraData"].empty()) {
        return false;
    }
    value = root["extraData"].asString();
    m_extra.assign(value.begin(), value.end()); 
    convertStringFromJson(root["mixHash"], h256, m_mixDigest)
    convertInt64FromJson(root["nonce"], m_nonce)
    convertStringFromJson(root["gasLimit"], u256, m_gasLimit)
    convertStringFromJson(root["gasLimit"], u256, m_gasLimit)
    if (!root["baseFeePerGas"].empty()) {
        convertStringFromJson(root["baseFeePerGas"], bigint, m_baseFee)
    }
    return true;
}

h256 xeth_block_header_t::parentHash() {
    return m_parentHash;
}

h256 xeth_block_header_t::uncle_hash() {
    return m_uncleHash;
}

Address xeth_block_header_t::miner() {
    return m_miner;
}

h256 xeth_block_header_t::stateMerkleRoot() {
    return m_stateMerkleRoot;
}

h256 xeth_block_header_t::txMerkleRoot() {
    return m_txMerkleRoot;
}

h256 xeth_block_header_t::receiptMerkleRoot() {
    return m_receiptMerkleRoot;
}

LogBloom xeth_block_header_t::logBloom() {
    return m_bloom;
}

bigint xeth_block_header_t::difficulty(){
    return m_difficulty;
}

int64_t xeth_block_header_t::number(){
    return m_number;
}

u256 xeth_block_header_t::gasLimit(){
    return m_gasLimit;
}

u256 xeth_block_header_t::gasUsed(){
    return m_gasUsed;
}

int64_t xeth_block_header_t::time(){
    return m_time;
}

std::vector<uint8_t> xeth_block_header_t::extra(){
    return m_extra;
}

h256 xeth_block_header_t::mixDigest(){
    return m_mixDigest;
}

int64_t xeth_block_header_t::nonce() {
    return m_nonce;
}

bigint xeth_block_header_t::baseFee(){
    return m_baseFee;
}

h256 xeth_block_header_t::hash() {
    if (!m_hashed) {
        auto value = encode_rlp();
        auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size() - 1);
        m_hash = FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
        m_hashed = true;
    }
    return m_hash;
}

bytes xeth_block_header_t::encode_rlp() {
    bytes out;
    std::array<uint8_t, 32> arr;
    // parentHash
    {
        auto tmp = RLP::encode(m_parentHash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // uncle_hash
    {
        auto tmp = RLP::encode(m_uncleHash.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // miner
    {
        auto tmp = RLP::encode(m_miner.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // stateMerkleRoot
    {
        auto tmp = RLP::encode(m_stateMerkleRoot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // txMerkleRoot
    {
        auto tmp = RLP::encode(m_txMerkleRoot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // receiptMerkleRoot
    {
        auto tmp = RLP::encode(m_receiptMerkleRoot.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // logBloom
    {
        auto tmp = RLP::encode(m_bloom.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // difficulty
    {
        auto vec = util::fromU256((u256)m_difficulty);
        std::copy(vec.begin(), vec.end(), arr.begin());
        auto tmp = RLP::encode<32>(arr);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // number
    {
        auto tmp = RLP::encode(m_number);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasLimit
    {
        auto vec = util::fromU256(m_gasLimit);
        std::copy(vec.begin(), vec.end(), arr.begin());
        auto tmp = RLP::encode<32>(arr);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasUsed
    {
        auto vec = util::fromU256(m_gasUsed);
        std::copy(vec.begin(), vec.end(), arr.begin());
        auto tmp = RLP::encode<32>(arr);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    // time
    {
        auto tmp = RLP::encode(m_time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    // mixDigest
    {
        auto tmp = RLP::encode(m_mixDigest.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // nonce
    {
        auto tmp = RLP::encode(m_nonce);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    return out;
}

NS_END4
