// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xbasic/xmodule_type.h"
#include "xdata/xaction_parse.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xdata_defines.h"
#include "xdata/xdata_error.h"
#include "xdata/xmemcheck_dbg.h"
#include "xcrypto/xckey.h"

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

    out.write_compact_var(m_origindata);

    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_uncompact_write_without_hash_signature(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();

    stream << m_origindata;

    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_read_without_hash_signature(base::xstream_t & in) {
    const int32_t begin_pos = in.size();

    in.read_compact_var(m_origindata);

    RLP::DecodedItem decoded = RLP::decode(::data(m_origindata));
    CheckTransaction _checkSig = CheckTransaction::Everything;
    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
    }

    m_nonce = fromBigEndian<u256>(vecData[0]);
    xdbg("serial_transfrom::eth_to_top nonce:%s", m_nonce.str().c_str());
    m_gasprice = fromBigEndian<u256>(vecData[1]);
    xdbg("serial_transfrom::eth_to_top gasprice:%s", m_gasprice.str().c_str());
    m_gas = fromBigEndian<u256>(vecData[2]);
    xdbg("serial_transfrom::eth_to_top gas:%s", m_gas.str().c_str());
    Type type = vecData[3].empty() ? ContractCreation : MessageCall;
    xdbg("serial_transfrom::eth_to_top type:%d", type);
    bool bIsCreation = type == ContractCreation;
    Address to = vecData[3].empty() ? Address() : Address(vecData[3], FixedHash<20>::FromBinary);
    xdbg("serial_transfrom::eth_to_top target address:%s", to.hex().c_str());
    m_amount = fromBigEndian<u256>(vecData[4]);
    xdbg("serial_transfrom::eth_to_top value:%s", m_amount.str().c_str());
    m_data.insert(m_data.begin(), vecData[5].begin(), vecData[5].end());
    xdbg("serial_transfrom::eth_to_top data:%s", string(m_data.begin(), m_data.end()).c_str());
    m_SignV = fromBigEndian<u256>(vecData[6]);
    xdbg("serial_transfrom::eth_to_top v:%s", m_SignV.str().c_str());
    m_SignR = fromBigEndian<u256>(vecData[7]);
    xdbg("serial_transfrom::eth_to_top r:%s", m_SignR.hex().c_str());
    m_SignS = fromBigEndian<u256>(vecData[8]);
    xdbg("serial_transfrom::eth_to_top s:%s", m_SignS.hex().c_str());
    byte recoveryID;
    uint64_t m_chainId;
    SignatureStruct vrs;
    if (isZeroSignature(m_SignR, m_SignS)) {
        m_chainId = static_cast<uint64_t>(m_SignV);
        vrs = SignatureStruct{m_SignR, m_SignS, 0};
    } else {
        if (m_SignV > 36) {
            auto const chainId = (m_SignV - 35) / 2;
            if (chainId > std::numeric_limits<uint64_t>::max())
                return -1;
            m_chainId = static_cast<uint64_t>(chainId);
        }
        // only values 27 and 28 are allowed for non-replay protected transactions
        else if (m_SignV != 27 && m_SignV != 28) {
            return -2;
        }

        recoveryID = m_chainId ? static_cast<byte>(m_SignV - (u256{m_chainId} * 2 + 35)) : static_cast<byte>(m_SignV - 27);
        vrs = SignatureStruct{m_SignR, m_SignS, recoveryID};
        xdbg("serial_transfrom::eth_to_top  chainId:%d recoveryID:%d", m_chainId, recoveryID);
        if (_checkSig >= CheckTransaction::Cheap && !vrs.isValid()) {
            return -3;
        }
    }
    string strFrom;
    if (_checkSig == CheckTransaction::Everything) {
        if (isZeroSignature(m_SignR, m_SignS))
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
            m_hash = top::base::xstring_utl::to_hex(strDigest);
            xdbg("serial_transfrom::eth_to_top txhash:%s", m_hash.c_str());
            top::uint256_t hash((uint8_t *)(szDigest));
            char szSign[65] = {0};
            memcpy(szSign, (char *)&recoveryID, 1);
            memcpy(szSign + 1, (char *)m_SignR.data(), 32);
            memcpy(szSign + 33, (char *)m_SignS.data(), 32);
            m_authorization.append((char *)szSign, 65);
            top::utl::xecdsasig_t sig((uint8_t *)szSign);
            top::utl::xkeyaddress_t pubkey1("");
            uint8_t szOutput[65] = {0};
            top::utl::xsecp256k1_t::get_publickey_from_signature(sig, hash, szOutput);
            top::utl::xecpubkey_t pubkey(szOutput);
            strFrom = pubkey.to_raw_eth_address();
            string strPublicKey((char *)szOutput, 65);
            string strHex = top::base::xstring_utl::to_hex(strPublicKey);
            xdbg("serial_transfrom::eth_to_top piblicKey:%s Source Address:%s", strHex.c_str(), strFrom.c_str());
        }
    }

    if (bIsCreation) {
        m_transaction_type = xtransaction_type_deploy_evm_contract;
    } else {
        m_transaction_type = xtransaction_type_transfer;
    }

    m_source_addr = "T60004" + strFrom.substr(2);
    string strTo = to.hex();
    m_target_addr = "T60004" + strTo;

    const int32_t end_pos = in.size();
    return (begin_pos - end_pos);
}

int xtransaction_v3_t::do_write(base::xstream_t & out) {
    const int32_t begin_pos = out.size();
    do_write_without_hash_signature(out);
    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int xtransaction_v3_t::do_read(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    do_read_without_hash_signature(in);
    set_digest();
    set_tx_len(begin_pos - in.size());
    if (get_tx_type() == xtransaction_type_transfer) {
        data::xproperty_asset asset_out{(uint64_t)m_amount};
        data::xaction_t action;
        xaction_asset_out::serialze_to(action, asset_out);
    }
    std::error_code ec;
    auto const target_account_address = common::xaccount_address_t::build_from(get_target_addr(), ec);
    if (ec) {
        throw enum_xerror_code_bad_transaction;
    }
    auto const source_account_address = common::xaccount_address_t::build_from(get_source_addr(), ec);
    if (ec) {
        throw enum_xerror_code_bad_transaction;
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
    if (m_transaction_hash_str.empty()) {
        m_transaction_hash_str.append("0x").append(m_hash);
    }
    return m_transaction_hash_str;
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

    utl::xkeyaddress_t key_address(addr_prefix);
    utl::xecdsasig_t signature_obj((uint8_t*)m_authorization.c_str());
    top::uint256_t hash((uint8_t *)fromHex(m_hash).data());
    return key_address.verify_signature(signature_obj, hash);
}

bool xtransaction_v3_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    //TODO ETH sign check
    auto pub_data = base::xstring_utl::base64_decode(pub_key.to_string());
    xdbg("xtransaction_v3_t::pub_key_sign_check pub_key: %s , decode.size():%d,m_transaction_hash:%s", pub_key.to_string().c_str(), pub_data.size(), get_digest_hex_str().c_str());
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.data());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, pub_data.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)pub_data.size()));
    top::uint256_t hash((uint8_t *)fromHex(m_hash).data());
    return utl::xsecp256k1_t::verify_signature(signature_obj, hash, out_publickey_data, false);
}

size_t xtransaction_v3_t::get_serialize_size() const {
    base::xstream_t stream(base::xcontext_t::instance());
    xassert(stream.size() == 0);
    const_cast<xtransaction_v3_t*>(this)->serialize_to(stream);
    return stream.size();
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
    result_json["send_timestamp"] = 0;
    result_json["premium_price"] = get_premium_price();
    result_json["last_tx_nonce"] = static_cast<xJson::UInt64>(get_last_nonce());
    result_json["note"] = get_memo();
    result_json["ext"] = data::uint_to_str(get_ext().data(), get_ext().size());
    result_json["authorization"] = get_authorization();

    if (tx_version == RPC_VERSION_V2) {
        result_json["sender_account"] = m_source_addr;
        result_json["receiver_account"] = m_target_addr;
        result_json["amount"] = 0;
        result_json["eth_amount"] = m_amount.str();
        result_json["token_name"] = "ETH";
        result_json["edge_nodeid"] = "";

        result_json["sender_action_name"] = m_source_action_name;
        result_json["sender_action_param"] = data::uint_to_str(m_source_action_para.data(), m_source_action_para.size());
        result_json["receiver_action_name"] = m_target_action_name;
        result_json["receiver_action_param"] = data::uint_to_str(m_target_action_para.data(), m_target_action_para.size());
    } else {
        result_json["to_ledger_id"] = 0;
        result_json["from_ledger_id"] = 0;
        result_json["tx_random_nonce"] = 0;
        result_json["last_tx_hash"] = "";
        result_json["challenge_proof"] = "";

        std::string source_action_para = m_source_action_para;
        std::string target_action_para = m_target_action_para;
        if (get_tx_type() == xtransaction_type_transfer) {
            data::xproperty_asset asset_out{XPROPERTY_ASSET_ETH, (uint64_t)m_amount};
            data::xaction_t action;
            xaction_asset_out::serialze_to(action, asset_out);
            source_action_para = action.get_action_param();
            target_action_para = action.get_action_param();
        }
        xJson::Value & s_action_json = result_json["sender_action"];
        s_action_json["action_hash"] = "";
        s_action_json["action_type"] = m_source_action_type;
        s_action_json["action_size"] = 0;
        s_action_json["tx_sender_account_addr"] = m_source_addr;
        s_action_json["action_name"] = m_source_action_name;
        s_action_json["action_param"] = data::uint_to_str(source_action_para.data(), source_action_para.size());
        s_action_json["action_ext"] = "";
        s_action_json["action_authorization"] = "";

        xJson::Value & t_action_json = result_json["receiver_action"];
        t_action_json["action_hash"] = "";
        t_action_json["action_type"] = m_target_action_type;
        t_action_json["action_size"] = 0;
        t_action_json["tx_receiver_account_addr"] = m_target_addr;
        t_action_json["action_name"] = m_target_action_name;
        t_action_json["action_param"] = data::uint_to_str(target_action_para.data(), target_action_para.size());
        t_action_json["action_ext"] = "";
        t_action_json["action_authorization"] = "";
    }
}

void xtransaction_v3_t::construct_from_json(xJson::Value & request) {
    string strEth = base::xstring_utl::from_hex(request["params"][0].asString().substr(2));
    string strTop;
    int nRet = serial_transfrom::eth_to_top(strEth, strTop);
    if (nRet < 0) {
        return;
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)strTop.data(), strTop.size());

    do_read_without_hash_signature(stream);

    set_tx_len(strTop.size());
}

int32_t xtransaction_v3_t::parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) {
    if (get_tx_type() == xtransaction_type_transfer) {
        // todo: eth original amount is u256 
        xdbg("xtransaction_v3_t::parse tx:%s,amount=%llu", dump().c_str(), get_amount());
        tx_parse_data.m_asset = xproperty_asset(XPROPERTY_ASSET_ETH, get_amount());
    } else {
        xerror("xtransaction_v3_t::parse not support other transction types!tx:%s", dump().c_str());
        return xchain_error_action_param_empty;
    }

    return 0;
}

}  // namespace data
}  // namespace top
