// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xbase/xhash.h"
#include "xbasic/xbasic_size.hpp"
#include "xbasic/xmodule_type.h"
#include "xbasic/xhex.h"
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


namespace top { namespace data {

using namespace top::base;

xtransaction_v3_t::xtransaction_v3_t() : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_tx_v3) {
    MEMCHECK_ADD_TRACE(this, "tx_create");
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, 1);
}
xtransaction_v3_t::xtransaction_v3_t(xeth_transaction_t const& ethtx)
: xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_tx_v3), m_ethtx(ethtx) {
    MEMCHECK_ADD_TRACE(this, "tx_create");
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, 1);
    update_cache();
}

xtransaction_v3_t::~xtransaction_v3_t() {
    statistic_del();
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

void xtransaction_v3_t::update_cache() {
    // TODO(jimmy)
    if (!m_ethtx.get_data().empty()) {
        if (m_ethtx.get_to().is_zero()) {
            m_transaction_type = xtransaction_type_deploy_evm_contract;
        } else {
            m_transaction_type = xtransaction_type_run_contract;
        }
    } else {
        if (!m_ethtx.get_to().is_zero()) {
            m_transaction_type = xtransaction_type_transfer;
        } else {
            xassert(false);
            m_transaction_type = xtransaction_type_invalid;
        }
    }

    std::error_code ec;
    auto const from_address = common::xaccount_address_t::build_from(m_ethtx.get_from(), base::enum_vaccount_addr_type_secp256k1_evm_user_account);
    if (ec) {xassert(false);}
    m_source_addr = from_address.to_string();
    auto const to_address = common::xaccount_address_t::build_from(m_ethtx.get_to(), base::enum_vaccount_addr_type_secp256k1_evm_user_account);
    if (ec) {xassert(false);}
    m_target_addr = to_address.to_string();

    m_transaction_len = (uint16_t)m_ethtx.encodeBytes().size();  // TODO(jimmy)

    char szSign[65] = {0};
    xbyte_t recoveryID = (xbyte_t)m_ethtx.get_signV();
    memcpy(szSign, (char *)&recoveryID, 1);
    memcpy(szSign + 1, (char *)m_ethtx.get_signR().data(), m_ethtx.get_signR().asBytes().size());
    memcpy(szSign + 33, (char *)m_ethtx.get_signS().data(), m_ethtx.get_signS().asBytes().size());
    m_authorization.append((char *)szSign, 65);
}

int xtransaction_v3_t::do_write(base::xstream_t & out) {
    const int32_t begin_pos = out.size();
    out.write_compact_var(m_version);
    std::string ethtx_str = m_ethtx.serialize_to_string();
    out.write_compact_var(ethtx_str);
    const int32_t end_pos = out.size();
    return (end_pos - begin_pos);
}

int xtransaction_v3_t::do_read(base::xstream_t & in) {
    const int32_t begin_pos = in.size();
    in.read_compact_var(m_version);
    std::string ethtx_str;
    in.read_compact_var(ethtx_str);
    eth_error ec;
    m_ethtx.serialize_from_string(ethtx_str, ec);
    if (ec.error_code) {
        xerror("xtransaction_v3_t::do_read-fail serialize eth tx");
        return -1;
    }

    update_cache();
    const int32_t end_pos = in.size();
    return (begin_pos - end_pos);
}

void xtransaction_v3_t::adjust_target_address(uint32_t table_id) {
    // TODO(jimmy) no need
}

void xtransaction_v3_t::set_digest() {
    // TODO(jimmy) no need
}

bool xtransaction_v3_t::transaction_len_check() const {
    // TODO(jimmy) no need
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
    // TODO(jimmy) no need
}

bool xtransaction_v3_t::digest_check() const {
    return true;
}

std::string xtransaction_v3_t::get_digest_str()const {
    xbytes_t txhash_bs = top::to_bytes(m_ethtx.get_tx_hash());
    return top::to_string(txhash_bs);
}
std::string xtransaction_v3_t::get_digest_hex_str() const {
    xbytes_t txhash_bs = top::to_bytes(m_ethtx.get_tx_hash());
    return top::to_hex_prefixed(txhash_bs);
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
    // TODO(jimmy) no need
    return true;
}

bool xtransaction_v3_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    xassert(false);
    // TODO(jimmy) no need
    return false;
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
            data::xproperty_asset asset_out{XPROPERTY_ASSET_ETH, (uint64_t)m_ethtx.get_value()};
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
    // TODO(jimmy)
    xassert(false);
}

int32_t xtransaction_v3_t::parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) {
    // TODO(jimmy)
    xerror("xtransaction_v3_t::parse not support!tx:%s", dump().c_str());
    return xchain_error_action_param_empty;
}

int32_t xtransaction_v3_t::get_object_size_real() const {
    int32_t total_size = sizeof(*this);
    // add string member variable alloc size.
    total_size += get_size(m_source_addr) + get_size(m_target_addr) + get_size(m_authorization) + m_ethtx.get_to().get_ex_alloc_size() +
                  m_ethtx.get_from().get_ex_alloc_size() + m_ethtx.get_data().capacity() + m_ethtx.get_accesslist().capacity()*sizeof(xeth_accesstuple_t);
    return total_size;
}

}  // namespace data
}  // namespace top
