#include "xevm_common/xeth/xeth_header.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xcontext.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xdepends/include/json/reader.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xeth/xeth_util.h"
#include "xutility/xhash.h"
// The log bloom's size (2048-bit).
NS_BEG3(top, evm_common, eth)

using namespace top::evm_common;
bool xeth_block_header_t::fromJson(const std::string& content) {
    xJson::Reader reader;
    xJson::Value root;
    std::string value;
    // xJson::value value;
    auto result = reader.parse(content, root);
    if (!result) {
        return false;
    }

    convertStringFromJson(root["parentHash"], Hash, m_parentHash)
    convertStringFromJson(root["sha3Uncles"], Hash, m_uncleHash)
    convertStringFromJson(root["miner"], Address, m_miner)
    convertStringFromJson(root["stateRoot"], Hash, m_stateMerkleRoot)
    convertStringFromJson(root["transactionsRoot"], Hash, m_txMerkleRoot)
    convertStringFromJson(root["receiptsRoot"], Hash, m_receiptMerkleRoot)
    convertStringFromJson(root["logsBloom"], LogBloom, m_bloom)
    convertStringFromJson(root["difficulty"], bigint, m_difficulty)
    convertStringFromJson(root["number"], bigint, m_number)
    convertUint64FromJson(root["gasLimit"], m_gasLimit)
    convertUint64FromJson(root["gasUsed"], m_gasUsed)
    convertUint64FromJson(root["timestamp"], m_time)
    if (root["extraData"].empty()) {
        return false;
    }
    value = root["extraData"].asString().substr(2);
    m_extra = util::hex_to_bytes(value);
    convertStringFromJson(root["mixHash"], Hash, m_mixDigest)
    convertStringFromJson(root["nonce"], BlockNonce, m_nonce)
    if (!root["baseFeePerGas"].empty()) {
        convertStringFromJson(root["baseFeePerGas"], bigint, m_baseFee)
        m_isBaseFee = true;
    }
    return true;
}

Hash xeth_block_header_t::parentHash() const {
    return m_parentHash;
}

Hash xeth_block_header_t::uncle_hash() const {
    return m_uncleHash;
}

Address xeth_block_header_t::miner() const {
    return m_miner;
}

Hash xeth_block_header_t::stateMerkleRoot() const {
    return m_stateMerkleRoot;
}

Hash xeth_block_header_t::txMerkleRoot() const {
    return m_txMerkleRoot;
}

Hash xeth_block_header_t::receiptMerkleRoot() const {
    return m_receiptMerkleRoot;
}

LogBloom xeth_block_header_t::logBloom() const {
    return m_bloom;
}

bigint xeth_block_header_t::difficulty() const {
    return m_difficulty;
}

bigint xeth_block_header_t::number() const {
    return m_number;
}

uint64_t xeth_block_header_t::gasLimit() const {
    return m_gasLimit;
}

uint64_t xeth_block_header_t::gasUsed() const {
    return m_gasUsed;
}

uint64_t xeth_block_header_t::time() const {
    return m_time;
}

bytes xeth_block_header_t::extra() const {
    return m_extra;
}

Hash xeth_block_header_t::mixDigest() const {
    return m_mixDigest;
}

BlockNonce xeth_block_header_t::nonce() const {
    return m_nonce;
}

bigint xeth_block_header_t::baseFee() const {
    return m_baseFee;
}

Hash xeth_block_header_t::hash() {
    if (!m_hashed) {
        auto value = encode_rlp();
        auto hashValue = utl::xkeccak256_t::digest(value.data(), value.size());
        m_hash = FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
        m_hashed = true;
    }
    return m_hash;
}

Hash xeth_block_header_t::hashWithoutSeal() {
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
        auto tmp = RLP::encode(static_cast<u256>(m_difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // number
    {
        auto tmp = RLP::encode(static_cast<u256>(m_number));
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
            auto tmp = RLP::encode(static_cast<u256>(m_baseFee));
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
        auto tmp = RLP::encode(static_cast<u256>(m_difficulty));
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    // number
    {
        auto tmp = RLP::encode(static_cast<u256>(m_number));
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
            auto tmp = RLP::encode(static_cast<u256>(m_baseFee));
            out.insert(out.end(), tmp.begin(), tmp.end());
        }
    }

    {
        out = RLP::encodeList(out);
    }
    
    return out;
}

bool xeth_block_header_t::isBaseFee() const {
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
    stream << evm_common::toBigEndian(static_cast<u256>(m_difficulty));
    stream << evm_common::toBigEndian(static_cast<u256>(m_number));
    stream << m_gasLimit;
    stream << m_gasUsed;
    stream << m_time;
    stream << m_extra;
    stream << m_mixDigest.asBytes();
    stream << m_nonce.asBytes();
    stream << evm_common::toBigEndian(static_cast<u256>(m_baseFee));
    stream << m_hash.asBytes();
    stream << m_hashed;
    stream << m_isBaseFee;
    return std::string(reinterpret_cast<const char *>(stream.data()), stream.size());
}

int xeth_block_header_t::from_string(const std::string & s) {
    if (s.empty()) {
        xwarn("[xeth_block_header_t::from_string] invalid input");
        return -1;
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)s.data(), (uint32_t)s.size());
    const int begin_pos = stream.size();
    {
        bytes bytes;
        stream >> bytes;
        m_parentHash = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_uncleHash = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_miner = static_cast<Address>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_stateMerkleRoot = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_txMerkleRoot = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_receiptMerkleRoot = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_bloom = static_cast<LogBloom>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_difficulty = static_cast<bigint>(evm_common::fromBigEndian<u256>(bytes));
    }
    {
        bytes bytes;
        stream >> bytes;
        m_number = static_cast<bigint>(evm_common::fromBigEndian<u256>(bytes));
    }
    stream >> m_gasLimit;
    stream >> m_gasUsed;
    stream >> m_time;
    stream >> m_extra;
    {
        bytes bytes;
        stream >> bytes;
        m_mixDigest = static_cast<Hash>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_nonce = static_cast<BlockNonce>(bytes);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_baseFee = static_cast<bigint>(evm_common::fromBigEndian<u256>(bytes));
    }
    {
        bytes bytes;
        stream >> bytes;
        m_hash = static_cast<Hash>(bytes);
    }
    stream >> m_hashed;
    stream >> m_isBaseFee;
    const int end_pos = stream.size();
    return (begin_pos - end_pos);
}

int xeth_block_header_t::from_rlp(const xbytes_t & bytes) {
    auto l = RLP::decodeList(bytes);
    if (!l.remainder.empty()) {
        xassert(false);
        return -1;
    }
    m_parentHash = static_cast<Hash>(l.decoded[0]);
    m_uncleHash = static_cast<Hash>(l.decoded[1]);
    m_miner = static_cast<Address>(l.decoded[2]);
    m_stateMerkleRoot = static_cast<Hash>(l.decoded[3]);
    m_txMerkleRoot = static_cast<Hash>(l.decoded[4]);
    m_receiptMerkleRoot = static_cast<Hash>(l.decoded[5]);
    m_bloom = static_cast<LogBloom>(l.decoded[6]);
    m_difficulty = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[7]));
    m_number = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[8]));
    m_gasLimit = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[9]));
    m_gasUsed = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[10]));
    m_time = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(l.decoded[11]));
    m_extra = l.decoded[12];
    m_mixDigest = static_cast<Hash>(l.decoded[13]);
    m_nonce = static_cast<BlockNonce>(l.decoded[14]);
    if (l.decoded.size() > 15) {
        m_isBaseFee = true;
        m_baseFee = static_cast<bigint>(evm_common::fromBigEndian<u256>(l.decoded[15]));
    }
    return l.decoded.size();
}

std::string xeth_block_header_t::dump() {
    char local_param_buf[256];
    xprintf(local_param_buf,
            sizeof(local_param_buf),
            "miner: %s, height: %s, hash: %s, basefee: %d, %s",
            m_miner.hex().c_str(),
            m_number.str().c_str(),
            hash().hex().c_str(),
            m_isBaseFee,
            m_baseFee.str().c_str());
    return std::string(local_param_buf);
}

xeth_block_header_with_difficulty_t::xeth_block_header_with_difficulty_t(xeth_block_header_t header, bigint difficulty) : m_header{header}, m_difficult_sum{difficulty} {
}

std::string xeth_block_header_with_difficulty_t::to_string() {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header_str = m_header.to_string();
    stream << header_str;
    stream << evm_common::toBigEndian(static_cast<u256>(m_difficult_sum));
    return std::string(reinterpret_cast<const char *>(stream.data()), stream.size());
}

int xeth_block_header_with_difficulty_t::from_string(const std::string & s) {
    if (s.empty()) {
        xwarn("[xeth_block_header_t::from_string] invalid input");
        return -1;
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)s.data(), (uint32_t)s.size());
    const int begin_pos = stream.size();
    {
        std::string header_str;
        stream >> header_str;
        m_header.from_string(header_str);
    }
    {
        bytes bytes;
        stream >> bytes;
        m_difficult_sum = static_cast<bigint>(evm_common::fromBigEndian<u256>(bytes));
    }
    const int end_pos = stream.size();
    return (begin_pos - end_pos);
}

NS_END3
