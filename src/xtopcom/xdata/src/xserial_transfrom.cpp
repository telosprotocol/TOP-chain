#include "xdata/xserial_transfrom.h"
#include "xdata/xtransaction.h"

static bool isZeroSignature(u256 const & _r, u256 const & _s) {
    return !_r && !_s;
}

bool hasZeroSignature(SignatureStruct & m_vrs) {
    return isZeroSignature(m_vrs.r, m_vrs.s);
}

bool SignatureStruct::isValid() const noexcept {
    static const h256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"};
    static const h256 s_zero;

    return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
}
namespace top {
namespace data {
int serial_transfrom::eth_to_top(string strEth, string & strTop) {
    TransactionSkeleton tx;
    RLP::DecodedItem decoded = RLP::decode(::data(strEth));
    CheckTransaction _checkSig = CheckTransaction::Everything;
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }

    tx.nonce = fromBigEndian<u256>(vecData[0]);
    xdbg("serial_transfrom::eth_to_top nonce:%s", tx.nonce.str().c_str());
    tx.gasPrice = fromBigEndian<u256>(vecData[1]);
    xdbg("serial_transfrom::eth_to_top gasprice:%s", tx.gasPrice.str().c_str());
    tx.gas = fromBigEndian<u256>(vecData[2]);
    xdbg("serial_transfrom::eth_to_top gas:%s", tx.gas.str().c_str());
    Type type = vecData[3].empty() ? ContractCreation : MessageCall;
    xdbg("serial_transfrom::eth_to_top type:%d", type);
    tx.creation = type == ContractCreation;
    tx.to = vecData[3].empty() ? Address() : Address(vecData[3], FixedHash<20>::FromBinary);
    xdbg("serial_transfrom::eth_to_top target address:%s", tx.to.hex().c_str());
    tx.value = fromBigEndian<u256>(vecData[4]);
    xdbg("serial_transfrom::eth_to_top value:%s", tx.value.str().c_str());
    tx.data.insert(tx.data.begin(), vecData[5].begin(), vecData[5].end());
    xdbg("serial_transfrom::eth_to_top data:%s", string(tx.data.begin(), tx.data.end()).c_str());
    u256 const v = fromBigEndian<u256>(vecData[6]);
    xdbg("serial_transfrom::eth_to_top v:%s", v.str().c_str());
    h256 const r = fromBigEndian<u256>(vecData[7]);
    xdbg("serial_transfrom::eth_to_top r:%s", r.hex().c_str());
    h256 const s = fromBigEndian<u256>(vecData[8]);
    xdbg("serial_transfrom::eth_to_top s:%s", s.hex().c_str());
    byte recoveryID;
    uint64_t m_chainId;
    if (isZeroSignature(r, s)) {
        m_chainId = static_cast<uint64_t>(v);
        tx.vrs = SignatureStruct{r, s, 0};
    } else {
        if (v > 36) {
            auto const chainId = (v - 35) / 2;
            if (chainId > std::numeric_limits<uint64_t>::max())
                return -1;
            m_chainId = static_cast<uint64_t>(chainId);
        }
        // only values 27 and 28 are allowed for non-replay protected transactions
        else if (v != 27 && v != 28) {
            return -2;
        }

        recoveryID = m_chainId ? static_cast<byte>(v - (u256{m_chainId} * 2 + 35)) : static_cast<byte>(v - 27);
        tx.vrs = SignatureStruct{r, s, recoveryID};
        xdbg("serial_transfrom::eth_to_top  chainId:%d recoveryID:%d", m_chainId, recoveryID);
        if (_checkSig >= CheckTransaction::Cheap && !tx.vrs.isValid()) {
            return -3;
        }
    }
    string strFrom;
    string strHash;
    if (_checkSig == CheckTransaction::Everything) {
        if (hasZeroSignature(tx.vrs))
            strFrom = "ffffffffffffffffffffffffffffffffffffffff";
        else {
            bytes encoded = bytes();
            for (int i = 0; i < 6; i++) {
                append(encoded, RLP::encode(vecData[i]));
            }
            append(encoded, RLP::encode(m_chainId));
            append(encoded, RLP::encode(0));
            append(encoded, RLP::encode(0));
            bytes encoded1 = RLP::encodeList(encoded);
            char szDigest[32] = {0};
            keccak_256((const unsigned char *)encoded1.data(), encoded1.size(), (unsigned char *)szDigest);

            string strDigest;
            strDigest.append(szDigest, 32);
            strHash = top::base::xstring_utl::to_hex(strDigest);
            xdbg("serial_transfrom::eth_to_top txhash:%s", strHash.c_str());
            top::uint256_t hash((uint8_t *)(szDigest));
            char szSign[65] = {0};
            memcpy(szSign, (char *)&recoveryID, 1);
            memcpy(szSign + 1, (char *)r.data(), 32);
            memcpy(szSign + 33, (char *)s.data(), 32);
            top::utl::xecdsasig_t sig((uint8_t *)szSign);
            top::utl::xkeyaddress_t pubkey1("");
            uint8_t szOutput[65] = { 0 };
            top::utl::xsecp256k1_t::get_publickey_from_signature(sig, hash, szOutput);
            top::utl::xecpubkey_t pubkey(szOutput);
            strFrom = pubkey.to_raw_eth_address();
            string strPublicKey((char *)szOutput, 65);
            string strHex = top::base::xstring_utl::to_hex(strPublicKey);
            xdbg("serial_transfrom::eth_to_top piblicKey:%s Source Address:%s", strHex.c_str(), strFrom.c_str());
        }
    }
    if (vecData.size() > 9)
        return -7;

    top::base::xstream_t stream(top::base::xcontext_t::instance());

    if (type == ContractCreation) {
        stream.write_compact_var((int)top::data::enum_xtransaction_type::xtransaction_type_deploy_evm_contract);
    } else if (type == MessageCall) {
        stream.write_compact_var((int)top::data::enum_xtransaction_type::xtransaction_type_transfer);
    }

    string strT6From = "T60004" + strFrom.substr(2);
    stream.write_compact_var(strT6From);

    string strTo = tx.to.hex();
    string strT6To = "T60004" + strTo;
    stream.write_compact_var(strT6To);

    stream.write_compact_var(vecData[4]);
    stream.write_compact_var(vecData[0]);
    stream.write_compact_var(vecData[2]);
    stream.write_compact_var(vecData[1]);
    stream.write_compact_var(strHash);
    stream.write_compact_var(vecData[5]);
    stream.write_compact_var(strEth);
    strTop.append((char *)stream.data(), stream.size());
    return 0;
}

int serial_transfrom::top_to_eth(string strEth, string & strTop) {
    top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)strTop.data(), strTop.size());
    int nType = 0;
    stream.read_compact_var(nType);
    string strFrom;
    string strT6From;
    stream.read_compact_var(strT6From);
    strFrom = strT6From.substr(6);

    string strTo;
    string strT6To;
    stream.read_compact_var(strT6To);
    strTo = strT6To.substr(6);

    string strValue;
    string strNonce;
    string strGas;
    string strGasPrice;
    string strHash;
    string strData;
    stream.read_compact_var(strValue);
    stream.read_compact_var(strNonce);
    stream.read_compact_var(strGas);
    stream.read_compact_var(strGasPrice);
    stream.read_compact_var(strHash);
    stream.read_compact_var(strData);
    stream.read_compact_var(strEth);
    xdbg("serial_transfrom::top_to_eth Eth:%s", top::base::xstring_utl::to_hex(strEth).c_str());
    return 0;
}


int serial_transfrom::getSign(string strEth, char* szSign) {
    TransactionSkeleton tx;
    RLP::DecodedItem decoded = RLP::decode(::data(strEth));
    CheckTransaction _checkSig = CheckTransaction::Everything;
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }

    tx.nonce = fromBigEndian<u256>(vecData[0]);
    xdbg("serial_transfrom::getSign nonce:%s", tx.nonce.str().c_str());
    tx.gasPrice = fromBigEndian<u256>(vecData[1]);
    xdbg("serial_transfrom::getSign gasprice:%s", tx.gasPrice.str().c_str());
    tx.gas = fromBigEndian<u256>(vecData[2]);
    xdbg("serial_transfrom::getSign gas:%s", tx.gas.str().c_str());
    Type type = vecData[3].empty() ? ContractCreation : MessageCall;
    xdbg("serial_transfrom::getSign type:%d", type);
    tx.creation = type == ContractCreation;
    tx.to = vecData[3].empty() ? Address() : Address(vecData[3], FixedHash<20>::FromBinary);
    xdbg("serial_transfrom::getSign target address:%s", tx.to.hex().c_str());
    tx.value = fromBigEndian<u256>(vecData[4]);
    xdbg("serial_transfrom::getSign value:%s", tx.value.str().c_str());
    tx.data.insert(tx.data.begin(), vecData[5].begin(), vecData[5].end());
    xdbg("serial_transfrom::getSign data:%s", string(tx.data.begin(), tx.data.end()).c_str());
    u256 const v = fromBigEndian<u256>(vecData[6]);
    xdbg("serial_transfrom::getSign v:%s", v.str().c_str());
    h256 const r = fromBigEndian<u256>(vecData[7]);
    xdbg("serial_transfrom::getSign r:%s", r.hex().c_str());
    h256 const s = fromBigEndian<u256>(vecData[8]);
    xdbg("serial_transfrom::getSign s:%s", s.hex().c_str());
    byte recoveryID;
    uint64_t m_chainId;
    if (isZeroSignature(r, s)) {
        m_chainId = static_cast<uint64_t>(v);
        tx.vrs = SignatureStruct{r, s, 0};
    } else {
        if (v > 36) {
            auto const chainId = (v - 35) / 2;
            if (chainId > std::numeric_limits<uint64_t>::max())
                return -1;
            m_chainId = static_cast<uint64_t>(chainId);
        }
        // only values 27 and 28 are allowed for non-replay protected transactions
        else if (v != 27 && v != 28) {
            return -2;
        }

        recoveryID = m_chainId ? static_cast<byte>(v - (u256{m_chainId} * 2 + 35)) : static_cast<byte>(v - 27);
        tx.vrs = SignatureStruct{r, s, recoveryID};
        xdbg("serial_transfrom::getSign  chainId:%d recoveryID:%d", m_chainId, recoveryID);
        if (_checkSig >= CheckTransaction::Cheap && !tx.vrs.isValid()) {
            return -3;
        }
    }
    memcpy(szSign, (char *)&recoveryID, 1);
    memcpy(szSign + 1, (char *)r.data(), 32);
    memcpy(szSign + 33, (char *)s.data(), 32);
    xdbg("serial_transfrom::getSign:%s", top::base::xstring_utl::to_hex(string(szSign, 65)).c_str());
    return 0;
}
}  // namespace data
}  // namespace top
