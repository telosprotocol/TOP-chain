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
    out.write_compact_var((int)m_transaction_type);
    out.write_compact_var(m_source_addr);
    out.write_compact_var(m_target_addr);

    out.write_compact_var(toBigEndianString(m_amount));
    out.write_compact_var(toBigEndianString(m_nonce));
    out.write_compact_var(toBigEndianString(m_gas));
    out.write_compact_var(toBigEndianString(m_gasprice));
    out.write_compact_var(m_hash);
    out.write_compact_var(m_data);
    out.write_compact_var(m_origindata);

    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_uncompact_write_without_hash_signature(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
   
    stream << (int)m_transaction_type;
    stream << m_source_addr;
    stream << m_target_addr;
    stream << toBigEndianString(m_amount);
    stream << toBigEndianString(m_nonce);
    stream << toBigEndianString(m_gas);
    stream << toBigEndianString(m_gasprice);
    stream << m_hash;
    stream << m_data;
    stream << m_origindata;

    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v3_t::do_read_without_hash_signature(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    int tmp_transaction_type;
    in.read_compact_var(tmp_transaction_type);
    m_transaction_type = (enum_xtransaction_type)tmp_transaction_type;
    in.read_compact_var(m_source_addr);
    in.read_compact_var(m_target_addr);

    string strAmount;
    in.read_compact_var(strAmount);
    m_amount = fromBigEndian<u256>(strAmount);

    string strNonce;
    in.read_compact_var(strNonce);
    m_nonce = fromBigEndian<u256>(strNonce);

    string strGas;
    in.read_compact_var(strGas);
    m_gas = fromBigEndian<u256>(strGas);

    string strGasPrice;
    in.read_compact_var(strGasPrice);
    m_gasprice = fromBigEndian<u256>(strGasPrice);

    in.read_compact_var(m_hash);
    in.read_compact_var(m_data);
    in.read_compact_var(m_origindata);

    uint8_t szSign[65] = {0};
    serial_transfrom::getSign(m_origindata, (char *)szSign);
    m_authorization.append((char *)szSign, 65);

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
    uint8_t szSign[65] = {0};
    serial_transfrom::getSign(m_origindata, (char*)szSign);
    utl::xecdsasig_t signature_obj(szSign);
    top::uint256_t hash((uint8_t *)fromHex(m_hash).data());
    return key_address.verify_signature(signature_obj, hash);
}

bool xtransaction_v3_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    //TODO ETH sign check


    return true;
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
            data::xproperty_asset asset_out{m_token_name, (uint64_t)m_amount};
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
