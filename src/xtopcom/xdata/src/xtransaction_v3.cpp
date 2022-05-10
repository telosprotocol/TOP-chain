// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xbase/xhash.h"
#include "xbasic/xmodule_type.h"
#include "xdata/xaction_parse.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xdata_defines.h"
#include "xdata/xdata_error.h"
#include "xdata/xerror/xerror.h"
#include "xdata/xmemcheck_dbg.h"
#include "xcrypto/xckey.h"
#include "xevm_common/rlp.h"

using namespace std;
using namespace top::evm_common;
using namespace top::evm_common::rlp;

namespace top { namespace data {

using namespace top::base;

xtransaction_v3_t::xtransaction_v3_t() {
    MEMCHECK_ADD_TRACE(this, "tx_create");
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, 1);
}

xtransaction_v3_t::~xtransaction_v3_t() {
    MEMCHECK_REMOVE_TRACE(this);
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, -1);
}

void xtransaction_v3_t::construct_tx(enum_xtransaction_type tx_type,
                                     const uint16_t expire_duration,
                                     const uint32_t deposit,
                                     const uint32_t nonce,
                                     const std::string & memo,
                                     const xtx_action_info & info) {
    return;
}

int32_t xtransaction_v3_t::do_write_without_hash_signature(base::xstream_t & out) const {
    const int32_t begin_pos = out.size();
    uint8_t szEipVersion = m_EipVersion;
    out.write_compact_var(szEipVersion);
    out.write_compact_var(m_origindata);

    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_uncompact_write_without_hash_signature(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();

    uint8_t szEipVersion = m_EipVersion;
    stream << szEipVersion;
    stream << m_origindata;

    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_read_without_hash_signature(base::xstream_t & in, eth_error &ec) {
    const int32_t begin_pos = in.size();
    uint8_t szEipVersion;
    in.read_compact_var(szEipVersion);
    in.read_compact_var(m_origindata);
    if (m_origindata.empty())
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: value size exceeds available input length")
        return -1;
    }
    m_EipVersion = (EIP_XXXX)szEipVersion;
    bytes encoded;
    bool bIsCreation;
    byte recoveryID;
    Address to;
    int ret = 0;
    try {
        if (m_EipVersion == EIP_XXXX::EIP_TOP_V3) {
            m_eip_xxxx_tx = make_object_ptr<eip_top_v3_tx>();
            ret = unserialize_top_v3_transaction(encoded, bIsCreation, recoveryID, to, ec);
        } else if (m_EipVersion == EIP_XXXX::EIP_LEGACY) {
            m_eip_xxxx_tx = make_object_ptr<eip_legacy_tx>();
            ret = unserialize_eth_legacy_transaction(encoded, bIsCreation, recoveryID, to, ec);
        } else if (m_EipVersion == EIP_1559) {
            m_eip_xxxx_tx = make_object_ptr<eip_1559_tx>();
            ret = unserialize_eth_1559_transaction(encoded, bIsCreation, recoveryID, to, ec);
        } else {
            xwarn("xtransaction_v3_t::do_read_without_hash_signature unsupport eip version:%d", m_EipVersion);
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "transaction type not supported")
            return -1;
        }
    } catch (...) {
        xwarn("xtransaction_v3_t::do_read_without_hash_signature unserialize transaction error:%d", ret);
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: value size exceeds available input length")
        return -2;
    }
    if (ret < 0)
    {
        xwarn("xtransaction_v3_t::do_read_without_hash_signature unserialize transaction error:%d", ret);
        return -3;
    }

    string strFrom;
    char szDigest[32] = {0};
    string strEncoded;
    strEncoded.append((char*)encoded.data(), encoded.size());
    m_unsign_hash = top::utl::xkeccak256_t::digest(strEncoded);
    string strTxString;
    if (m_EipVersion != EIP_XXXX::EIP_LEGACY) {
         strTxString.append((char*)&m_EipVersion, 1);
         strTxString.append(m_origindata);
    }
    else {
         strTxString.append(m_origindata);
    }
    top::uint256_t origin_hash = top::utl::xkeccak256_t::digest(strTxString);
    string strOriginDigest;
    strOriginDigest.append((char*)origin_hash.data(), origin_hash.size());
    m_hash = top::base::xstring_utl::to_hex(strOriginDigest);
    xdbg("xtransaction_v3_t::do_read_without_hash_signature txhash:%s", m_hash.c_str());
    char szSign[65] = {0};
    memcpy(szSign, (char *)&recoveryID, 1);
    memcpy(szSign + 1, (char *)m_eip_xxxx_tx->get_signR().data(), m_eip_xxxx_tx->get_signR().asBytes().size());
    memcpy(szSign + 33, (char *)m_eip_xxxx_tx->get_signS().data(), m_eip_xxxx_tx->get_signS().asBytes().size());
    m_authorization.append((char *)szSign, 65);
    top::utl::xecdsasig_t sig((uint8_t *)szSign);
    top::utl::xkeyaddress_t pubkey1("");
    uint8_t szOutput[65] = {0};
    top::utl::xsecp256k1_t::get_publickey_from_signature(sig, m_unsign_hash, szOutput);
    top::utl::xecpubkey_t pubkey(szOutput);
    strFrom = pubkey.to_raw_eth_address();
    string strPublicKey((char *)szOutput, 65);
    string strHex = top::base::xstring_utl::to_hex(strPublicKey);
    xdbg("xtransaction_v3_t::do_read_without_hash_signature:%s Source Address:%s", strHex.c_str(), strFrom.c_str());
    if (m_EipVersion == EIP_XXXX::EIP_TOP_V3)
    {
        m_transaction_type = (enum_xtransaction_type)reinterpret_cast<eip_top_v3_tx*>(m_eip_xxxx_tx.get())->transaction_type;
        m_source_addr = reinterpret_cast<eip_top_v3_tx*>(m_eip_xxxx_tx.get())->from;
        m_target_addr = reinterpret_cast<eip_top_v3_tx*>(m_eip_xxxx_tx.get())->to;
    }
    else {
        if (bIsCreation) {
            m_transaction_type = xtransaction_type_deploy_evm_contract;
        } else {
            if (m_eip_xxxx_tx->get_data().empty()) {
                m_transaction_type = xtransaction_type_transfer;
            } else {
                m_transaction_type = xtransaction_type_run_contract;
            }
        }

        m_source_addr = ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN + strFrom.substr(2);
        string strTo = m_eip_xxxx_tx->get_to();
        if (!strTo.empty()) {
            m_target_addr = ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN + strTo;
        }
    }
    const int32_t end_pos = in.size();
    return (begin_pos - end_pos);
}

int xtransaction_v3_t::unserialize_eth_legacy_transaction(bytes & encoded, bool & bIsCreation, byte & recoveryID, Address & to, eth_error& ec) {
    if (m_eip_xxxx_tx == nullptr)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "top node not enough storage")
        return -1;
    }
    eip_legacy_tx* eip_legacy_tx_ptr = reinterpret_cast<eip_legacy_tx*>(m_eip_xxxx_tx.get());
    RLP::DecodedItem decoded = RLP::decode(top::evm_common::rlp::data(m_origindata));
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }
    if (vecData.size() != 9) {
        if (vecData.size() > 9)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: input list has too many elements for types.DynamicFeeTx")
        }
        else
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: too few elements for types.DynamicFeeTx")
        }
        return -2;
    }
    if (vecData[0].length() > 32)
    {
       ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: input string too long for common.Nonce, decoding into (types.DynamicFeeTx).Nonce")
        return -3;
    }
    eip_legacy_tx_ptr->nonce = fromBigEndian<u256>(vecData[0]);
    if (vecData[1].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Nonce, decoding into (types.DynamicFeeTx).Nonce")
        return -4;
    }
    eip_legacy_tx_ptr->gasprice = fromBigEndian<u256>(vecData[1]);
    if (vecData[2].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.GasPrice, decoding into (types.DynamicFeeTx).GasPrice")
        return -5;
    }
    eip_legacy_tx_ptr->gas = fromBigEndian<u256>(vecData[2]);
    if (vecData[3].length() != 20 && vecData[3].length() != 0)
    {
        if (vecData[3].length() > 20)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Address, decoding into (types.DynamicFeeTx).To")
        }
        else {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too short for common.Address, decoding into (types.DynamicFeeTx).To")
        }
        return -6;
    }
    bIsCreation = vecData[3].empty();
    eip_legacy_tx_ptr->to = vecData[3].empty() ? Address().hex() : Address(vecData[3], FixedHash<20>::FromBinary).hex();
    if (vecData[4].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params,"rlp: input string too long for common.Data, decoding into (types.DynamicFeeTx).Data")
        return -7;
    }
    eip_legacy_tx_ptr->value = fromBigEndian<u256>(vecData[4]);
    eip_legacy_tx_ptr->data = vecData[5];
    if (vecData[6].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params,"rlp: input string too long for common.signV, decoding into (types.DynamicFeeTx).SignV")
        return -8;
    }
    eip_legacy_tx_ptr->signV = fromBigEndian<u256>(vecData[6]);
    if (vecData[7].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signR, decoding into (types.DynamicFeeTx).SignR")
        return -9;
    }
    eip_legacy_tx_ptr->signR = fromBigEndian<u256>(vecData[7]);
    if (vecData[8].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signS, decoding into (types.DynamicFeeTx).SignS")
        return -10;
    }
    eip_legacy_tx_ptr->signS = fromBigEndian<u256>(vecData[8]);
    uint64_t m_chainId;
    if (!eip_legacy_tx_ptr->signR && !eip_legacy_tx_ptr->signS) {
        m_chainId = static_cast<uint64_t>(eip_legacy_tx_ptr->signV);
        recoveryID = 0;
    } else {
        if (eip_legacy_tx_ptr->signV > 36) {
            auto const chainId = (eip_legacy_tx_ptr->signV - 35) / 2;
            if (chainId > std::numeric_limits<uint64_t>::max())
            {
                ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "chainid is not top chainid")
                return -11;
            }
            m_chainId = static_cast<uint64_t>(chainId);
        }
        // only values 27 and 28 are allowed for non-replay protected transactions
        else if (eip_legacy_tx_ptr->signV != 27 && eip_legacy_tx_ptr->signV != 28) {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "transaction sign error")
            return -12;
        }
        static const h256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"};
        static const h256 s_zero;
        recoveryID = m_chainId ? static_cast<byte>(eip_legacy_tx_ptr->signV - (u256{m_chainId} * 2 + 35)) : static_cast<byte>(eip_legacy_tx_ptr->signV - 27);
        xdbg("xtransaction_v3_t::unserialize_eth_legacy_transaction  chainId:%d recoveryID:%d", m_chainId, recoveryID);
        if (!(recoveryID <= 1 && eip_legacy_tx_ptr->signR > s_zero && eip_legacy_tx_ptr->signS > s_zero && eip_legacy_tx_ptr->signR < s_max && eip_legacy_tx_ptr->signS < s_max)) {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "transaction sign error")
            return -13;
        }
    }
    if (m_chainId != 1023)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "chainid is not top chainid")
        return -14;
    }
    string strFrom;
    if (!eip_legacy_tx_ptr->signR && !eip_legacy_tx_ptr->signS) {
        strFrom = "ffffffffffffffffffffffffffffffffffffffff";
    }
    else {
        bytes encodedtmp = bytes();
        for (int i = 0; i < 6; i++) {
            append(encodedtmp, RLP::encode(vecData[i]));
        }
        append(encodedtmp, RLP::encode(m_chainId));
        append(encodedtmp, RLP::encode(0));
        append(encodedtmp, RLP::encode(0));
        encoded = RLP::encodeList(encodedtmp);
    }
    return 0;
}

int xtransaction_v3_t::unserialize_eth_1559_transaction(bytes & encoded, bool & bIsCreation, byte & recoveryID, Address & to, eth_error& ec) {
    if (m_eip_xxxx_tx == nullptr)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "top node not enough storage")
        return -1;
    }
    eip_1559_tx* eip_1559_tx_ptr = reinterpret_cast<eip_1559_tx*>(m_eip_xxxx_tx.get());
    RLP::DecodedItem decoded = RLP::decode(top::evm_common::rlp::data(m_origindata));
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }

    if (vecData.size() != 12) {
        if (vecData.size() > 12)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: input list has too many elements for types.DynamicFeeTx")
        }
        else
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: too few elements for types.DynamicFeeTx")
        }
        return -2;
    }

    if (vecData[0].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.ChainId, decoding into (types.DynamicFeeTx).ChainId")
        return -3;
    }
    eip_1559_tx_ptr->chainid = fromBigEndian<u256>(vecData[0]);
    if (eip_1559_tx_ptr->chainid != 1023)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "chainid is not top chainid")
        return -4;
    }
    if (vecData[1].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.ChainId, decoding into (types.DynamicFeeTx).ChainId")
        return -5;
    }
    eip_1559_tx_ptr->nonce = fromBigEndian<u256>(vecData[1]);
    if (vecData[2].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.MaxPriorityFeePerGas, decoding into (types.DynamicFeeTx).MaxPriorityFeePerGas")
        return -6;
    }
    eip_1559_tx_ptr->max_priority_fee_per_gas = fromBigEndian<u256>(vecData[2]);
    if (vecData[3].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.MaxFeePerGas, decoding into (types.DynamicFeeTx).MaxFeePerGas")
        return -7;
    }
    eip_1559_tx_ptr->max_fee_per_gas = fromBigEndian<u256>(vecData[3]);
    if (vecData[4].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Gas, decoding into (types.DynamicFeeTx).Gas")
        return -8;
    }
    eip_1559_tx_ptr->gas = fromBigEndian<u256>(vecData[4]);
    if (vecData[5].length() != 20 && vecData[5].length() != 0)
    {
        if (vecData[5].length() > 20)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Address, decoding into (types.DynamicFeeTx).Address")
        }
        else 
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too short for common.Address, decoding into (types.DynamicFeeTx).Address")
        }
        return -9;
    }
    bIsCreation = vecData[5].empty();
    eip_1559_tx_ptr->to = vecData[5].empty() ? Address().hex() : Address(vecData[5], FixedHash<20>::FromBinary).hex();
    if (vecData[6].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.value, decoding into (types.DynamicFeeTx).value")
        return -10;
    }
    eip_1559_tx_ptr->value = fromBigEndian<u256>(vecData[6]);
    eip_1559_tx_ptr->data = vecData[7];
    eip_1559_tx_ptr->accesslist = vecData[8];
    if (vecData[9].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signV, decoding into (types.DynamicFeeTx).signV")
        return -11;
    }
    eip_1559_tx_ptr->signV = fromBigEndian<u256>(vecData[9]);
    if (vecData[10].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signR, decoding into (types.DynamicFeeTx).signR")
        return -12;
    }
    eip_1559_tx_ptr->signR = fromBigEndian<u256>(vecData[10]);
    if (vecData[11].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signS, decoding into (types.DynamicFeeTx).signS")
        return -13;
    }
    eip_1559_tx_ptr->signS = fromBigEndian<u256>(vecData[11]);

    bytes encodedtmp = bytes();
    for (int i = 0; i < 8; i++) {
        append(encodedtmp, RLP::encode(vecData[i]));
    }
    append(encodedtmp, fromHex("c0"));

    append(encoded, static_cast<uint8_t>(m_EipVersion));
    append(encoded, RLP::encodeList(encodedtmp));
    recoveryID = (byte)eip_1559_tx_ptr->signV;
    return 0;
}

int xtransaction_v3_t::unserialize_top_v3_transaction(bytes & encoded, bool & bIsCreation, byte & recoveryID, Address & to, eth_error& ec) {
    if (m_eip_xxxx_tx == nullptr)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "top node not enough storage")
        return -1;
    }
    eip_top_v3_tx* eip_top_v3_tx_ptr = reinterpret_cast<eip_top_v3_tx*>(m_eip_xxxx_tx.get());
    RLP::DecodedItem decoded = RLP::decode(top::evm_common::rlp::data(m_origindata));
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }
    if (vecData.size() != 13) {
        if (vecData.size() > 13)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: input list has too many elements for types.DynamicFeeTx")
        }
        else
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp: too few elements for types.DynamicFeeTx")
        }
        return -2;
    }

    if (vecData[0].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.ChainId, decoding into (types.DynamicFeeTx).ChainId")
        return -3;
    }
    eip_top_v3_tx_ptr->chainid = fromBigEndian<u256>(vecData[0]);
    if (eip_top_v3_tx_ptr->chainid != 1023)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "chainid is not top chainid")
        return -4;
    }
    if (vecData[1].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Nonce, decoding into (types.DynamicFeeTx).Nonce")
        return -5;
    }
    eip_top_v3_tx_ptr->nonce = fromBigEndian<u256>(vecData[1]);
    if (vecData[2].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.MaxPriorityFeePerGas, decoding into (types.DynamicFeeTx).MaxPriorityFeePerGas")
        return -6;
    }
    eip_top_v3_tx_ptr->max_priority_fee_per_gas = fromBigEndian<u256>(vecData[2]);
    if (vecData[3].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.MaxFeePerGas, decoding into (types.DynamicFeeTx).MaxFeePerGas")
        return -7;
    }
    eip_top_v3_tx_ptr->max_fee_per_gas = fromBigEndian<u256>(vecData[3]);
    if (vecData[4].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Gas, decoding into (types.DynamicFeeTx).Gas")
        return -8;
    }
    eip_top_v3_tx_ptr->gas = fromBigEndian<u256>(vecData[4]);
    eip_top_v3_tx_ptr->to = vecData[5];
    if (vecData[6].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Value, decoding into (types.DynamicFeeTx).Value")
        return -9;
    }
    eip_top_v3_tx_ptr->value = fromBigEndian<u256>(vecData[6]);
    eip_top_v3_tx_ptr->data = vecData[7];
    eip_top_v3_tx_ptr->accesslist = vecData[8];
    std::string extra = vecData[9];
    if (vecData[10].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signV, decoding into (types.DynamicFeeTx).signV")
        return -10;
    }
    eip_top_v3_tx_ptr->signV = fromBigEndian<u256>(vecData[10]);
    if (vecData[11].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signR, decoding into (types.DynamicFeeTx).signR")
        return -11;
    }
    eip_top_v3_tx_ptr->signR = fromBigEndian<u256>(vecData[11]);
    if (vecData[12].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.signS, decoding into (types.DynamicFeeTx).signS")
        return -12;
    }
    eip_top_v3_tx_ptr->signS = fromBigEndian<u256>(vecData[12]);


    RLP::DecodedItem extradecode = RLP::decode(top::evm_common::rlp::data(extra));
    std::vector<std::string> vecExtraData;
    for (int i = 0; i < (int)extradecode.decoded.size(); i++) {
        std::string str(extradecode.decoded[i].begin(), extradecode.decoded[i].end());
        vecExtraData.push_back(str);
    }
    if (vecData.size() != 7)
    {
        if (vecData.size() > 7)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp:extra input list has too many elements for types.DynamicFeeTx")
        }
        else
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "rlp:extra too few elements for types.DynamicFeeTx")
        }
        return -13;
    }

    if (vecExtraData[0].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.SubTransactionVersion, decoding into (types.DynamicFeeTx).SubTransactionVersion")
        return -14;
    }
    eip_top_v3_tx_ptr->sub_transaction_version = (uint8_t)fromBigEndian<u256>(vecExtraData[0]);

    if (vecExtraData[1].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.TransactionType, decoding into (types.DynamicFeeTx).TransactionType")
        return -15;
    }
    eip_top_v3_tx_ptr->transaction_type = (uint8_t)fromBigEndian<u256>(vecExtraData[1]);
    eip_top_v3_tx_ptr->from = vecExtraData[2];

    if (vecExtraData[3].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.TokenId, decoding into (types.DynamicFeeTx).TokenId")
        return -17;
    }
    eip_top_v3_tx_ptr->token_id = (uint32_t)fromBigEndian<u256>(vecExtraData[3]);

    if (vecExtraData[4].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.FireTimestamp, decoding into (types.DynamicFeeTx).FireTimestamp")
        return -18;
    }
    eip_top_v3_tx_ptr->fire_timestamp = fromBigEndian<u256>(vecExtraData[4]);
    if (vecExtraData[5].length() > 32)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.ExpireDuration, decoding into (types.DynamicFeeTx).ExpireDuration")
        return -19;
    }
    eip_top_v3_tx_ptr->expire_duration = fromBigEndian<u256>(vecExtraData[5]);

    if (vecExtraData[6].length() > MAX_TRANSACTION_MEMO_SIZE)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: input string too long for common.Memo, decoding into (types.DynamicFeeTx).Memo")
        return -20;
    }
    eip_top_v3_tx_ptr->memo = vecExtraData[6];


    bytes encodedtmp = bytes();
    for (int i = 0; i < 8; i++) {
        append(encodedtmp, RLP::encode(vecData[i]));
    }
    append(encodedtmp, fromHex("c0"));
    append(encodedtmp, RLP::encode(vecData[9]));

    append(encoded, static_cast<uint8_t>(m_EipVersion));
    append(encoded, RLP::encodeList(encodedtmp));
    recoveryID = (byte)eip_top_v3_tx_ptr->signV;
    return 0;
}

int xtransaction_v3_t::do_write(base::xstream_t & out) {
    const int32_t begin_pos = out.size();
    do_write_without_hash_signature(out);
    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int xtransaction_v3_t::do_read(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    eth_error eth_ec;
    int32_t ret = do_read_without_hash_signature(in, eth_ec);
    if (ret < 0) {
        return enum_xerror_code_bad_transaction;
    }
    set_tx_len(begin_pos - in.size());
    std::error_code ec;
    auto const target_account_address = common::xaccount_address_t::build_from(get_target_addr(), ec);
    if (ec) {
        return enum_xerror_code_bad_address;
    }
    auto const source_account_address = common::xaccount_address_t::build_from(get_source_addr(), ec);
    if (ec) {
        return enum_xerror_code_bad_address;
    }

    if (is_sys_sharding_contract_address(target_account_address)) {
        auto tableid = data::account_map_to_table_id(source_account_address);
        adjust_target_address(tableid.get_subaddr());
    }
    const int32_t end_pos = in.size();
    return (begin_pos - end_pos);
}

#ifdef XENABLE_PSTACK  // tracking memory
int32_t xtransaction_v2_t::add_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_add");
    return base::xdataunit_t::add_ref();
}
int32_t xtransaction_v2_t::release_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_release");
    xdbgassert(get_refcount() > 0);
    return base::xdataunit_t::release_ref();
}
#endif

void xtransaction_v3_t::adjust_target_address(uint32_t table_id) {
    if (m_adjust_target_addr.empty()) {
        m_adjust_target_addr = make_address_by_prefix_and_subaddr(m_target_addr, table_id).value();
        xdbg("xtransaction_v2_t::adjust_target_address hash=%s,origin_addr=%s,new_addr=%s",
            get_digest_hex_str().c_str(), m_target_addr.c_str(), m_adjust_target_addr.c_str());        
    }
}

void xtransaction_v3_t::set_digest() {
    
}

bool xtransaction_v3_t::transaction_len_check() const {
   
    return true;
}

bool xtransaction_v3_t::verify_tx(xJson::Value & request, eth_error& ec)
{
    if (request["params"].empty())
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "missing value for required argument 0")
        return false;
    }
    if (!request["params"].isArray())
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "non-array args")
        return false;
    }
    if (!request["params"][0].isString())
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "invalid argument 0: json: cannot unmarshal non-string into Go value of type hexutil.Bytes")
        return false;
    }
    string strParams = request["params"][0].asString();

    if (strParams.size() % 2)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "invalid argument 0: json: cannot unmarshal hex string of odd length into Go value of type hexutil.Bytes")
        return false;
    }

    if (strParams.size() <= 10)
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: value size exceeds available input length")
        return false;
    }
    if (strParams[0] != '0' || strParams[1] != 'x')
    {
        ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "invalid argument 0: json: cannot unmarshal hex string without 0x prefix into Go value of type hexutil.Bytes")
        return false;
    }
    string strEth = base::xstring_utl::from_hex(strParams.substr(2));
    if (strEth[0] == '\01') {
        m_EipVersion = EIP_XXXX::EIP_2930;
        xdbg("xtransaction_v3_t::construct_from_json unsupport tx type :%d", strEth[0]);
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "transaction type not supported")
        return false;
    } else if (strEth[0] == '\02') {
        m_EipVersion = EIP_XXXX::EIP_1559;
    } else if (strEth[0] == 'y') {
        m_EipVersion = EIP_XXXX::EIP_TOP_V3;
    } else if ((unsigned char)strEth[0] >= ((unsigned char)128)) {
        m_EipVersion = EIP_XXXX::EIP_LEGACY;
    } else {
        xdbg("xtransaction_v3_t::construct_from_json unsupport tx type :%d", strEth[0]);
        ETH_RPC_ERROR(error::xenum_errc::eth_server_error, "transaction type not supported")
        return false;
    }
    string strTop;
    if (m_EipVersion == EIP_XXXX::EIP_LEGACY) {
        int nRet = serial_transfrom::eth_to_top(strEth, m_EipVersion, strTop);
        if (nRet < 0) {
            xdbg("xtransaction_v3_t::construct_from_json eth_to_top error Ret:%d", nRet);
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: value size exceeds available input length")
            return false;
        }
    } else {
        if (strEth.size() <= 1)
        {
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: value size exceeds available input length")
            return false;
        }
        int nRet = serial_transfrom::eth_to_top(strEth.substr(1), m_EipVersion, strTop);
        if (nRet < 0) {
            xdbg("xtransaction_v3_t::construct_from_json eth_to_top error Ret:%d", nRet);
            ETH_RPC_ERROR(error::xenum_errc::eth_invalid_params, "rlp: value size exceeds available input length")
            return false;
        }
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)strTop.data(), strTop.size());

    do_read_without_hash_signature(stream, ec);
    set_tx_len(strTop.size());
    return true;
}

int32_t xtransaction_v3_t::make_tx_create_user_account(const std::string & addr) {
    return xsuccess;
}

int32_t xtransaction_v3_t::make_tx_transfer(const data::xproperty_asset & asset) {
    return xsuccess;
}

int32_t xtransaction_v3_t::make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string & function_name, const std::string & para) {
    return xsuccess;
}

int32_t xtransaction_v3_t::make_tx_run_contract(std::string const & function_name, std::string const & param) {
    return 0;
}

int32_t xtransaction_v3_t::set_different_source_target_address(const std::string & src_addr, const std::string & dst_addr) {
    m_source_addr = src_addr;
    m_target_addr = dst_addr;
    return xsuccess;
}

int32_t xtransaction_v3_t::set_same_source_target_address(const std::string & addr) {
    return set_different_source_target_address(addr, addr);
}

void xtransaction_v3_t::set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) {
    set_last_nonce(last_nonce);
}

void xtransaction_v3_t::set_source(const std::string & addr, const std::string & action_name, const std::string & para) {
    m_source_addr = addr;
}

void xtransaction_v3_t::set_target(const std::string & addr, const std::string & action_name, const std::string & para) {
    m_target_addr = addr;
}

void xtransaction_v3_t::set_len() {
    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_pos = stream.size();
    do_write_without_hash_signature(stream);
    set_tx_len(stream.size() - begin_pos);
}

bool xtransaction_v3_t::digest_check() const {
    return true;
}

std::string xtransaction_v3_t::get_digest_hex_str() const {
    std::string transaction_hash_str;
    transaction_hash_str.append("0x").append(m_hash);
    return transaction_hash_str;
}

bool xtransaction_v3_t::check_last_nonce(uint64_t account_nonce) {
    return (get_last_nonce() == account_nonce);
}

void xtransaction_v3_t::set_fire_and_expire_time(uint16_t expire_duration) {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    set_fire_timestamp((uint64_t)val.tv_sec);
    set_expire_duration(expire_duration);
}

bool xtransaction_v3_t::sign_check() const {
    std::string addr_prefix;
    if (std::string::npos != get_source_addr().find_last_of('@')) {
        uint16_t subaddr;
        base::xvaccount_t::get_prefix_subaddr_from_account(get_source_addr(), addr_prefix, subaddr);
    } else {
        addr_prefix = get_source_addr();
    }

    if (m_unsign_hash.empty() || m_authorization.empty()) {
        return false;
    }

    utl::xkeyaddress_t key_address(addr_prefix);
    utl::xecdsasig_t signature_obj((uint8_t*)m_authorization.c_str());
    return key_address.verify_signature(signature_obj, m_unsign_hash);
}

bool xtransaction_v3_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    //TODO ETH sign check
    auto pub_data = base::xstring_utl::base64_decode(pub_key.to_string());
    if (pub_data.size() != utl::UNCOMPRESSED_PUBLICKEY_SIZE) {
        return false;
    }
    xdbg("xtransaction_v3_t::pub_key_sign_check pub_key: %s , decode.size():%d,m_transaction_hash:%s", pub_key.to_string().c_str(), pub_data.size(), get_digest_hex_str().c_str());
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.data());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, pub_data.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)pub_data.size()));
    return utl::xsecp256k1_t::verify_signature(signature_obj, m_unsign_hash, out_publickey_data, false);
}

std::string xtransaction_v3_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,    sizeof(local_param_buf),
    "{transaction:hash=%s,type=%u,from=%s,to=%s,nonce=%lu,refcount=%d,this=%p}",
    get_digest_hex_str().c_str(), (uint32_t)get_tx_type(), get_source_addr().c_str(), get_target_addr().c_str(),
    get_tx_nonce(), get_refcount(), this);
    return std::string(local_param_buf);
}

std::string xtransaction_v3_t::get_source_action_str() const {
    std::string str;
    str += m_source_addr;
    return str;
}

std::string xtransaction_v3_t::get_target_action_str() const {
    std::string str;
    str += m_target_addr;
    return str;
}

void xtransaction_v3_t::set_action_type() {
    xtransaction_t::set_action_type_by_tx_type(m_transaction_type);
}

void xtransaction_v3_t::parse_to_json(xJson::Value & result_json, const std::string & tx_version) const {
    result_json["tx_structure_version"] = xtransaction_version_3;
    result_json["tx_deposit"] = get_deposit();
    result_json["tx_type"] = get_tx_type();
    result_json["tx_len"] = get_tx_len();
    result_json["tx_hash"] = data::uint_to_str(digest().data(), digest().size());
    result_json["tx_expire_duration"] = get_expire_duration();
    result_json["send_timestamp"] = static_cast<xJson::UInt64>(get_fire_timestamp());
    result_json["premium_price"] = get_premium_price();
    result_json["last_tx_nonce"] = static_cast<xJson::UInt64>(get_last_nonce());
    result_json["note"] = get_memo();
    result_json["ext"] = data::uint_to_str(get_ext().data(), get_ext().size());
    result_json["authorization"] = get_authorization();

    if (tx_version == RPC_VERSION_V2) {
        result_json["sender_account"] = m_source_addr;
        result_json["receiver_account"] = m_target_addr;
        result_json["amount"] = 0;
        result_json["token_name"] = XPROPERTY_ASSET_ETH;
        result_json["edge_nodeid"] = "";

        result_json["sender_action_name"] = "";
        result_json["sender_action_param"] = "";
        result_json["receiver_action_name"] = "";
        result_json["receiver_action_param"] = "";
    } else {
        result_json["to_ledger_id"] = 0;
        result_json["from_ledger_id"] = 0;
        result_json["tx_random_nonce"] = 0;
        result_json["last_tx_hash"] = "";
        result_json["challenge_proof"] = "";

        std::string source_action_para = "";
        std::string target_action_para = "";
        if (get_tx_type() == xtransaction_type_transfer) {
            data::xproperty_asset asset_out{XPROPERTY_ASSET_ETH, m_eip_xxxx_tx ? (uint64_t)m_eip_xxxx_tx->get_value() : 0};
            data::xaction_t action;
            xaction_asset_out::serialze_to(action, asset_out);
            source_action_para = action.get_action_param();
            target_action_para = action.get_action_param();
        }
        xJson::Value & s_action_json = result_json["sender_action"];
        s_action_json["action_hash"] = "";
        s_action_json["action_type"] = 0;
        s_action_json["action_size"] = 0;
        s_action_json["tx_sender_account_addr"] = m_source_addr;
        s_action_json["action_name"] = "";
        s_action_json["action_param"] = data::uint_to_str(source_action_para.data(), source_action_para.size());
        s_action_json["action_ext"] = "";
        s_action_json["action_authorization"] = "";

        xJson::Value & t_action_json = result_json["receiver_action"];
        t_action_json["action_hash"] = "";
        t_action_json["action_type"] = 0;
        t_action_json["action_size"] = 0;
        t_action_json["tx_receiver_account_addr"] = m_target_addr;
        t_action_json["action_name"] = "";
        t_action_json["action_param"] = data::uint_to_str(target_action_para.data(), target_action_para.size());
        t_action_json["action_ext"] = "";
        t_action_json["action_authorization"] = "";
    }
}

void xtransaction_v3_t::construct_from_json(xJson::Value & request) {
    if (request["params"].empty())
    {
        return ;
    }
    if (!request["params"].isArray())
    {
        return ;
    }
    if (!request["params"][0].isString())
    {
        return ;
    }
    string strParams = request["params"][0].asString();
    if (strParams.size() <= 10)
    {
        return ;
    }
    if (strParams[0] != '0' || strParams[1] != 'x')
    {
        return ;
    }
    string strEth = base::xstring_utl::from_hex(strParams.substr(2));
    if (strEth[0] == '\01') {
        m_EipVersion = EIP_XXXX::EIP_2930;
        xdbg("xtransaction_v3_t::construct_from_json unsupport tx type :%d", strEth[0]);
        return;
    } else if (strEth[0] == '\02') {
        m_EipVersion = EIP_XXXX::EIP_1559;
    } else if (strEth[0] == 'y') {
        m_EipVersion = EIP_XXXX::EIP_TOP_V3;
    } else if ((unsigned char)strEth[0] >= ((unsigned char)128)) {
        m_EipVersion = EIP_XXXX::EIP_LEGACY;
    } else {
        xdbg("xtransaction_v3_t::construct_from_json unsupport tx type :%d", strEth[0]);
        return;
    }
    string strTop;
    if (m_EipVersion == EIP_XXXX::EIP_LEGACY) {
        int nRet = serial_transfrom::eth_to_top(strEth, m_EipVersion, strTop);
        if (nRet < 0) {
            xdbg("xtransaction_v3_t::construct_from_json eth_to_top error Ret:%d", nRet);
            return;
        }
    } else {
        if (strEth.size() <= 1)
        {
            return ;
        }
        int nRet = serial_transfrom::eth_to_top(strEth.substr(1), m_EipVersion, strTop);
        if (nRet < 0) {
            xdbg("xtransaction_v3_t::construct_from_json eth_to_top error Ret:%d", nRet);
            return;
        }
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)strTop.data(), strTop.size());

    eth_error ec;
    do_read_without_hash_signature(stream, ec);

    set_tx_len(strTop.size());
}

int32_t xtransaction_v3_t::parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) {
    if (get_tx_type() == xtransaction_type_transfer) {
        // todo: eth original amount is u256 
        xdbg("xtransaction_v3_t::parse tx:%s,amount=%s", dump().c_str(), m_eip_xxxx_tx ? m_eip_xxxx_tx->get_value().str().c_str() : "0");
        tx_parse_data.m_asset = xproperty_asset(XPROPERTY_ASSET_ETH, 0);
        tx_parse_data.m_amount_256 = m_eip_xxxx_tx ? m_eip_xxxx_tx->get_value() : 0;
    } else {
        xerror("xtransaction_v3_t::parse not support other transction types!tx:%s", dump().c_str());
        return xchain_error_action_param_empty;
    }

    return 0;
}

}  // namespace data
}  // namespace top
