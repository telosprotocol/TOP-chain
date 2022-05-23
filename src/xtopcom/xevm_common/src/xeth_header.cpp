#include "xdepends/include/json/reader.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xeth_util.h"
#include "xevm_common/rlp.h"
#include "xutility/xhash.h"
#include "xbase/xcontext.h"
// The log bloom's size (2048-bit).
NS_BEG3(top, evm_common, eth)

using namespace top::evm_common;
using namespace top::evm_common::rlp;
bool xeth_block_header_t::fromJson(const std::string& content) {
    xJson::Reader reader;
    xJson::Value root;
    std::string value;
    // xJson::value value;
    auto result = reader.parse(content, root);
    if (!result) {
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
    if (root["extraData"].empty()) {
        return false;
    }
    value = root["extraData"].asString().substr(2);
    m_extra = util::hex_to_bytes(value);
    convertStringFromJson(root["mixHash"], h256, m_mixDigest)
    convertStringFromJson(root["nonce"], h64, m_nonce)
    if (!root["baseFeePerGas"].empty()) {
        convertStringFromJson(root["baseFeePerGas"], bigint, m_baseFee)
        m_isBaseFee = true;
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

h64 xeth_block_header_t::nonce() {
    return m_nonce;
}

bigint xeth_block_header_t::baseFee(){
    return m_baseFee;
}

h256 xeth_block_header_t::hash() {
    if (!m_hashed) {
        auto value = encode_rlp();
        auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size());
        m_hash = FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
        m_hashed = true;
    }
    return m_hash;
}

h256 xeth_block_header_t::hashWithoutSeal() {
    auto value = encode_rlp_withoutseal();
    auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size());
    return FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
}

bytes xeth_block_header_t::encode_rlp_withoutseal() {
    bytes out;

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
        auto tmp = RLP::encode((u256)m_difficulty);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // number
    {
        auto tmp = RLP::encode(m_number);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasLimit
    {
        auto tmp = RLP::encode(m_gasLimit);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasUsed
    {
        auto tmp = RLP::encode(m_gasUsed);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    // time
    {
        auto tmp = RLP::encode(m_time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    //extra
    {
        auto tmp = RLP::encode(m_extra);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    {
        if (m_isBaseFee) {
            auto tmp = RLP::encode((u256)m_baseFee);
            out.insert(out.end(), tmp.begin(), tmp.end());
        }
    }

    {
        out = RLP::encodeList(out);
    }
    
    return out;
}

bytes xeth_block_header_t::encode_rlp() {
    bytes out;

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
        auto tmp = RLP::encode((u256)m_difficulty);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // number
    {
        auto tmp = RLP::encode(m_number);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasLimit
    {
        auto tmp = RLP::encode(m_gasLimit);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // gasUsed
    {
        auto tmp = RLP::encode(m_gasUsed);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    // time
    {
        auto tmp = RLP::encode(m_time);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    
    //extra
    {
        auto tmp = RLP::encode(m_extra);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // mixDigest
    {
        auto tmp = RLP::encode(m_mixDigest.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // nonce
    {
        auto tmp = RLP::encode(m_nonce.asBytes());
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    {
        if (m_isBaseFee) {
            auto tmp = RLP::encode((u256)m_baseFee);
            out.insert(out.end(), tmp.begin(), tmp.end());
        }
    }

    {
        out = RLP::encodeList(out);
    }
    
    return out;
}

bool xeth_block_header_t::isBaseFee() {
    return m_isBaseFee;
}

std::string xeth_block_header_t::to_string() {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_parentHash.asBytes();
    stream << m_uncleHash.asBytes();
    stream << m_miner.asBytes();
    stream << m_stateMerkleRoot.asBytes();
    stream << m_txMerkleRoot.asBytes();
    stream << m_receiptMerkleRoot.asBytes();
    stream << m_bloom.asBytes();
    // stream << evm_common::toBigEndian(m_difficulty);
    m_difficulty.convert_to<std::string>();
    stream << m_number;
    stream << evm_common::toBigEndian(m_gasLimit);
    stream << evm_common::toBigEndian(m_gasUsed);
    stream << m_time;
    stream << m_extra;
    stream << m_mixDigest.asBytes();
    stream << m_nonce.asBytes();
    // stream << evm_common::toBigEndian(m_baseFee);
    stream << m_hash.asBytes();
    stream << m_hashed;
    stream << m_isBaseFee;
    return std::string(reinterpret_cast<const char *>(stream.data()), stream.size());
}
int32_t xeth_block_header_t::from_string() {
    return 0;
}

NS_END3
