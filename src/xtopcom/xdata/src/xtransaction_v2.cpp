// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "xdata/xtransaction_v2.h"
#include "xbase/xmem.h"
#include "xdata/xaction_parse.h"
#include "xcrypto/xckey.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xdata_defines.h"
#include "xbasic/xmodule_type.h"
#include "xdata/xmemcheck_dbg.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "xcrypto/xckey.h"

#include <iostream>

namespace top { namespace data {

using namespace top::base;

xtransaction_v2_t::xtransaction_v2_t() {
    MEMCHECK_ADD_TRACE(this, "tx_create");
    XMETRICS_GAUGE(metrics::dataobject_xtransaction_t, 1);
}

xtransaction_v2_t::~xtransaction_v2_t() {
    MEMCHECK_REMOVE_TRACE(this);
    XMETRICS_GAUGE(metrics::dataobject_xtransaction_t, -1);
}

int32_t xtransaction_v2_t::do_write_without_hash_signature(base::xstream_t & out) const {
    const int32_t begin_pos = out.size();
    out.write_compact_var(static_cast<uint16_t>(m_transaction_type));
    out.write_compact_var(m_expire_duration);
    out.write_compact_var(m_fire_timestamp);

    std::string compact_source_account = xvaccount_t::compact_address_to(m_source_addr);
    out << compact_source_account;
    std::string compact_target_account = xvaccount_t::compact_address_to(m_target_addr);
    out << compact_target_account;
    out.write_compact_var(m_edge_nodeid);

    out.write_compact_var(m_amount);
    out.write_compact_var(m_token_name);
    out.write_compact_var(m_last_trans_nonce);
    out.write_compact_var(m_deposit);
    out.write_compact_var(m_premium_price);
    out.write_compact_var(m_memo);
    out.write_compact_var(m_ext);

    if(m_transaction_type != xtransaction_type_transfer) {
        out.write_compact_var(m_source_action_name);
        out.write_compact_var(m_source_action_para);
        out.write_compact_var(m_target_action_name);
        out.write_compact_var(m_target_action_para);
    }

    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v2_t::do_uncompact_write_without_hash_signature(base::xstream_t & stream) const {
    const int32_t begin_pos = stream.size();
    stream << static_cast<uint16_t>(m_transaction_type);
    stream << m_expire_duration;
    stream << m_fire_timestamp;
    stream << m_source_addr;
    stream << m_target_addr;
    stream << m_edge_nodeid;

    stream << m_amount;
    stream << m_token_name;
    stream << m_last_trans_nonce;
    stream << m_deposit;
    stream << m_premium_price;
    stream << m_memo;
    stream << m_ext;

    if(m_transaction_type != xtransaction_type_transfer) {
        stream << m_source_action_name;
        stream << m_source_action_para;
        stream << m_target_action_name;
        stream << m_target_action_para;
    }

    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v2_t::do_read_without_hash_signature(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    uint16_t tmp_transaction_type;
    in.read_compact_var(tmp_transaction_type);
    m_transaction_type = static_cast<enum_xtransaction_type>(tmp_transaction_type);
    in.read_compact_var(m_expire_duration);
    in.read_compact_var(m_fire_timestamp);
    
    std::string compact_source_account;
    in >> compact_source_account;
    m_source_addr = xvaccount_t::compact_address_from(compact_source_account);
    std::string compact_target_account;
    in >> compact_target_account;
    m_target_addr = xvaccount_t::compact_address_from(compact_target_account);
    in.read_compact_var(m_edge_nodeid);
    
    in.read_compact_var(m_amount);
    in.read_compact_var(m_token_name);
    in.read_compact_var(m_last_trans_nonce);
    in.read_compact_var(m_deposit);
    in.read_compact_var(m_premium_price);
    in.read_compact_var(m_memo);
    in.read_compact_var(m_ext);
        
    if(m_transaction_type != xtransaction_type_transfer) {
        in.read_compact_var(m_source_action_name);
        in.read_compact_var(m_source_action_para);
        in.read_compact_var(m_target_action_name);
        in.read_compact_var(m_target_action_para);
    }

    const int32_t end_pos = in.size();
    return (begin_pos - end_pos);
}

int xtransaction_v2_t::do_write(base::xstream_t & out) {
    const int32_t begin_pos = out.size();
    do_write_without_hash_signature(out);
    out.write_compact_var(m_authorization);
    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int xtransaction_v2_t::do_read(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    do_read_without_hash_signature(in);
    in.read_compact_var(m_authorization);
    set_digest();
    set_tx_len(begin_pos - in.size());
    set_action_type();
    m_source_action.set_account_addr(m_source_addr);
    m_target_action.set_account_addr(m_target_addr);
    if (is_sys_sharding_contract_address(common::xaccount_address_t{get_target_addr()})) {
        auto tableid = data::account_map_to_table_id(common::xaccount_address_t{get_source_addr()});
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

void xtransaction_v2_t::adjust_target_address(uint32_t table_id) {
    if (m_adjust_target_addr.empty()) {
        m_adjust_target_addr = make_address_by_prefix_and_subaddr(m_target_addr, table_id).value();
        xdbg("xtransaction_v2_t::adjust_target_address hash=%s,origin_addr=%s,new_addr=%s",
            get_digest_hex_str().c_str(), m_target_addr.c_str(), m_adjust_target_addr.c_str());        
    }
}

void xtransaction_v2_t::set_digest() {
    if (m_transaction_hash == uint256_t()) {
        base::xstream_t stream(base::xcontext_t::instance());
        do_uncompact_write_without_hash_signature(stream);
        m_transaction_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    }
}

bool xtransaction_v2_t::transaction_len_check() const {
    if (get_memo().size() > MAX_TRANSACTION_MEMO_SIZE) {
        xwarn("xtransaction_v2_t::transaction_len_check memo size too long.size:%d", get_memo().size());
        return false;
    }

    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_pos = stream.size();
    do_write_without_hash_signature(stream);
    stream.write_compact_var(m_authorization);

    uint16_t len = stream.size() - begin_pos;
    if (len != get_tx_len()) {
        xwarn("xtransaction_v2_t::transaction_len_check tx length not match. %d %d", len, get_tx_len());
        return false;
    }
    return true;
}

int32_t xtransaction_v2_t::make_tx_create_user_account(const std::string & addr) {
    set_tx_type(xtransaction_type_create_user_account);
    m_target_addr = addr;
    return xsuccess;
}

int32_t xtransaction_v2_t::make_tx_transfer(const data::xproperty_asset & asset) {
    set_tx_type(xtransaction_type_transfer);
    set_amount(asset.m_amount);
    return xsuccess;
}

int32_t xtransaction_v2_t::make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) {
    set_tx_type(xtransaction_type_run_contract);

    xstream_t stream(xcontext_t::instance());
    stream << asset_out.m_token_name;
    stream << asset_out.m_amount;
    m_source_action_para = std::string((char *)stream.data(), stream.size());

    m_target_action_name = function_name;
    m_target_action_para = para;
    return xsuccess;
}

int32_t xtransaction_v2_t::make_tx_run_contract(std::string const & function_name, std::string const & param) {
    data::xproperty_asset asset_out{0};
    return make_tx_run_contract(asset_out, function_name, param);
}

int32_t xtransaction_v2_t::set_different_source_target_address(const std::string & src_addr, const std::string & dst_addr) {
    m_source_addr = src_addr;
    m_target_addr = dst_addr;
    return xsuccess;
}

int32_t xtransaction_v2_t::set_same_source_target_address(const std::string & addr) {
    return set_different_source_target_address(addr, addr);
}

void xtransaction_v2_t::set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) {
    set_last_nonce(last_nonce);
}

void xtransaction_v2_t::set_len() {
    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_pos = stream.size();
    do_write_without_hash_signature(stream);
    stream.write_compact_var(m_authorization);
    set_tx_len(stream.size() - begin_pos);
}

bool xtransaction_v2_t::digest_check() const {
    return true;
}

std::string xtransaction_v2_t::get_digest_hex_str() const {
    if (m_transaction_hash_str.empty()) {
        m_transaction_hash_str = to_hex_str(m_transaction_hash);
    }
    return m_transaction_hash_str;
}

bool xtransaction_v2_t::check_last_nonce(uint64_t account_nonce) {
    return (get_last_nonce() == account_nonce);
}

void xtransaction_v2_t::set_fire_and_expire_time(uint16_t expire_duration) {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    set_fire_timestamp((uint64_t)val.tv_sec);
    set_expire_duration(expire_duration);
}

bool xtransaction_v2_t::sign_check() const {
    std::string addr_prefix;
    if (std::string::npos != get_source_addr().find_last_of('@')) {
        uint16_t subaddr;
        base::xvaccount_t::get_prefix_subaddr_from_account(get_source_addr(), addr_prefix, subaddr);
    } else {
        addr_prefix = get_source_addr();
    }

    utl::xkeyaddress_t key_address(addr_prefix);
    uint8_t     addr_type{255};
    uint16_t    network_id{65535};
    //get param from config
    uint16_t config_network_id = 0;//xchain_param.network_id
    if (!key_address.get_type_and_netid(addr_type, network_id) || config_network_id != network_id) {
        xwarn("network_id error:%d,%d", config_network_id, network_id);
        return false;
    }

    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.c_str());
    return key_address.verify_signature(signature_obj, m_transaction_hash);
}

bool xtransaction_v2_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    auto pub_data = base::xstring_utl::base64_decode(pub_key.to_string());
    xdbg("xtransaction_v2_t::pub_key_sign_check pub_key: %s , decode.size():%d,m_transaction_hash:%s", pub_key.to_string().c_str(), pub_data.size(),get_digest_hex_str().c_str());
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.data());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, pub_data.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)pub_data.size()));
    return utl::xsecp256k1_t::verify_signature(signature_obj, m_transaction_hash, out_publickey_data, false);
}

size_t xtransaction_v2_t::get_serialize_size() const {
    base::xstream_t stream(base::xcontext_t::instance());
    xassert(stream.size() == 0);
    const_cast<xtransaction_v2_t*>(this)->serialize_to(stream);
    return stream.size();
}

std::string xtransaction_v2_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,    sizeof(local_param_buf),
    "{transaction:hash=%s,type=%u,from=%s,to=%s,nonce=%" PRIu64 ",refcount=%d,this=%p}",
    get_digest_hex_str().c_str(), (uint32_t)get_tx_type(), get_source_addr().c_str(), get_target_addr().c_str(),
    get_tx_nonce(), get_refcount(), this);
    return std::string(local_param_buf);
}

void xtransaction_v2_t::set_action_type() {
    switch (get_tx_type())
    {
    case xtransaction_type_create_user_account:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_create_user_account);
        break;
    
    case xtransaction_type_run_contract:
        m_source_action.set_action_type(xaction_type_asset_out);
        m_target_action.set_action_type(xaction_type_run_contract);
        break;
    
    case xtransaction_type_transfer:
        m_source_action.set_action_type(xaction_type_asset_out);
        m_target_action.set_action_type(xaction_type_asset_in);
        break;
    
    case xtransaction_type_vote:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_run_contract);
        break;
    
    case xtransaction_type_abolish_vote:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_run_contract);
        break;
    
    case xtransaction_type_pledge_token_tgas:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_pledge_token);
        break;
    
    case xtransaction_type_redeem_token_tgas:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_redeem_token);
        break;
    
    case xtransaction_type_pledge_token_vote:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_pledge_token_vote);
        break;
    
    case xtransaction_type_redeem_token_vote:
        m_source_action.set_action_type(xaction_type_source_null);
        m_target_action.set_action_type(xaction_type_redeem_token_vote);
        break;

    default:
        break;
    }
}

void xtransaction_v2_t::parse_to_json(xJson::Value& result_json) const {
    result_json["tx_structure_version"] = 2;
    // result_json["source_account"] = m_source_addr;
    // result_json["target_account"] = m_target_addr;
    result_json["amount"] = static_cast<xJson::UInt64>(m_amount);
    result_json["token_name"] = m_token_name;

    result_json["tx_deposit"] = get_deposit();
    result_json["tx_type"] = get_tx_type();
    result_json["tx_len"] = get_tx_len();
    result_json["tx_expire_duration"] = get_expire_duration();
    result_json["send_timestamp"] = static_cast<xJson::UInt64>(get_fire_timestamp());
    result_json["premium_price"] = get_premium_price();
    result_json["last_tx_nonce"] = static_cast<xJson::UInt64>(get_last_nonce());
    result_json["note"] = get_memo();

    result_json["edge_nodeid"] = m_edge_nodeid;

    result_json["ext"] = data::uint_to_str(get_ext().data(), get_ext().size());
    result_json["tx_hash"] = data::uint_to_str(digest().data(), digest().size());
    result_json["authorization"] = get_authorization();

    // result_json["source_action_name"] = m_source_action_name;
    // result_json["source_action_para"] = data::uint_to_str(m_source_action_para.data(), m_source_action_para.size());
    // result_json["target_action_name"] = m_target_action_name;
    // result_json["target_action_para"] = data::uint_to_str(m_target_action_para.data(), m_target_action_para.size());
    // xdbg("authorization: %s", to_hex_str(get_authorization()).c_str());

    // just for compatibility    
    xJson::Value& s_action_json = result_json["sender_action"];
    s_action_json["action_hash"] = m_source_action.get_action_hash();
    s_action_json["action_type"] = m_source_action.get_action_type();
    s_action_json["action_size"] = m_source_action.get_action_size();
    s_action_json["tx_sender_account_addr"] = m_source_addr;
    s_action_json["action_name"] = m_source_action_name;
    s_action_json["action_param"] = data::uint_to_str(m_source_action_para.data(), m_source_action_para.size());
    s_action_json["action_ext"] = data::uint_to_str(m_source_action.get_action_ext().data(), m_source_action.get_action_ext().size());
    s_action_json["action_authorization"] = m_source_action.get_action_authorization();

    xJson::Value& t_action_json = result_json["receiver_action"];
    t_action_json["action_hash"] = m_target_action.get_action_hash();
    t_action_json["action_type"] = m_target_action.get_action_type();
    t_action_json["action_size"] = m_target_action.get_action_size();
    t_action_json["tx_receiver_account_addr"] = m_target_addr;
    t_action_json["action_name"] = m_target_action_name;
    t_action_json["action_param"] = data::uint_to_str(m_target_action_para.data(), m_target_action_para.size());
    t_action_json["action_ext"] = data::uint_to_str(m_target_action.get_action_ext().data(), m_target_action.get_action_ext().size());
    t_action_json["action_authorization"] = m_target_action.get_action_authorization();
}

void xtransaction_v2_t::construct_from_json(xJson::Value& request) {
    const auto & from = request["sender_action"]["tx_sender_account_addr"].asString();
    if (from.empty()) {
        set_source_addr(request["source_account"].asString());
        set_target_addr(request["target_account"].asString());

        m_source_action_name = request["source_action_name"].asString();
        auto source_param_vec = hex_to_uint(request["source_action_para"].asString());
        m_source_action_para = std::string((char *)source_param_vec.data(), source_param_vec.size());

        m_target_action_name = request["target_action_name"].asString();
        auto target_param_vec = hex_to_uint(request["target_action_para"].asString());
        m_target_action_para = std::string((char *)target_param_vec.data(), target_param_vec.size());

        set_amount(request["amount"].asUInt64());
        m_token_name = request["token_name"].asString();
    } else {
        set_source_addr(from);
        const auto & to = request["receiver_action"]["tx_receiver_account_addr"].asString();
        set_target_addr(to);
// amount
        xaction_t source_action;
        source_action.set_action_hash(request["sender_action"]["action_hash"].asUInt());
        source_action.set_action_type(static_cast<enum_xaction_type>(request["sender_action"]["action_type"].asUInt()));
        source_action.set_action_size(static_cast<uint16_t>(request["sender_action"]["action_size"].asUInt()));
        source_action.set_account_addr(from);
        source_action.set_action_name(request["sender_action"]["action_name"].asString());
        auto source_param_vec = hex_to_uint(request["sender_action"]["action_param"].asString());
        std::string source_param((char *)source_param_vec.data(), source_param_vec.size());
        source_action.set_action_param(std::move(source_param));
        auto source_ext_vec = hex_to_uint(request["sender_action"]["action_ext"].asString());
        std::string source_ext((char *)source_ext_vec.data(), source_ext_vec.size());
        source_action.set_action_ext(std::move(source_ext));
        source_action.set_action_authorization(request["sender_action"]["action_authorization"].asString());

        data::xaction_asset_out sa;
        int32_t ret = sa.parse(source_action);
        set_amount(sa.m_asset_out.m_amount);
// amount end
        m_token_name = sa.m_asset_out.m_token_name;

        m_source_action_name = request["sender_action"]["action_name"].asString();
        m_source_action_para = source_param;

        m_target_action_name = request["receiver_action"]["action_name"].asString();
        auto target_param_vec = hex_to_uint(request["receiver_action"]["action_param"].asString());
        std::string target_param((char *)target_param_vec.data(), target_param_vec.size());
        m_target_action_para = target_param;
    }
    
    set_action_type();
    set_tx_type(static_cast<uint16_t>(request["tx_type"].asUInt()));
    set_expire_duration(static_cast<uint16_t>(request["tx_expire_duration"].asUInt()));
    set_deposit(request["tx_deposit"].asUInt());
    set_premium_price(request["premium_price"].asUInt());

    set_last_nonce(request["last_tx_nonce"].asUInt64());
    set_fire_timestamp(request["send_timestamp"].asUInt64());
    set_memo(request["note"].asString());
    auto ext_vec = hex_to_uint(request["ext"].asString());
    std::string ext((char *)ext_vec.data(), ext_vec.size());
    set_ext(ext);
    m_edge_nodeid = request["edge_nodeid"].asString();

    set_digest();
    auto signature = hex_to_uint(request["authorization"].asString());
    std::string signature_str((char *)signature.data(), signature.size());  // hex_to_uint client send xstream_t data 0xaaaa => string
    set_authorization(std::move(signature_str));
    
    set_len();
}

int32_t xtransaction_v2_t::parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) {
#ifdef ENABLE_CREATE_USER  // debug use
    if (target_type == xaction_type_create_user_account) {
        tx_parse_data.m_new_account = m_source_addr;
    }
#endif

    if (source_type == xaction_type_asset_out) {
        tx_parse_data.m_asset = xproperty_asset(m_amount);
    }

    if (target_type == xaction_type_run_contract) {
        tx_parse_data.m_asset = xproperty_asset(m_amount);
        tx_parse_data.m_function_name = m_target_action_name;
        tx_parse_data.m_function_para = m_target_action_para;
    }

    if (target_type == xaction_type_pledge_token) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m_target_action_para.data(), m_target_action_para.size());
        stream >> tx_parse_data.m_asset.m_token_name;
        stream >> tx_parse_data.m_asset.m_amount;
    }

    if (target_type == xaction_type_redeem_token) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m_target_action_para.data(), m_target_action_para.size());
        stream >> tx_parse_data.m_asset.m_token_name;
        stream >> tx_parse_data.m_asset.m_amount;
    }

    if (target_type == xaction_type_pledge_token_vote) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m_target_action_para.data(), m_target_action_para.size());
        stream >> tx_parse_data.m_vote_num;
        stream >> tx_parse_data.m_lock_duration;
    }

    if (target_type == xaction_type_redeem_token_vote) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m_target_action_para.data(), m_target_action_para.size());
        stream >> tx_parse_data.m_vote_num;
    }

    return 0;
}

}  // namespace data
}  // namespace top
