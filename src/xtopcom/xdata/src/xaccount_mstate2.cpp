// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <map>

#include "xbase/xint.h"
#include "xbase/xlog.h"
#include "xbasic/xversion.h"
#include "xbasic/xserializable_based_on.h"
#include "xdata/xdata_common.h"
#include "xdata/xnative_property.h"
#include "xdata/xdatautil.h"
#include "xdata/xaccount_mstate.h"

namespace top { namespace data {

int32_t xaccount_mstate2::do_write(base::xstream_t & stream) const {
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_latest_send_trans_number);
    SERIALIZE_FIELD_BT(m_latest_send_trans_hash);
    SERIALIZE_FIELD_BT(m_latest_recv_trans_number);
    SERIALIZE_FIELD_BT(m_latest_recv_trans_hash);
    SERIALIZE_FIELD_BT(m_account_balance);
    SERIALIZE_FIELD_BT(m_account_burn_balance);
    SERIALIZE_FIELD_BT(m_account_pledge_balance.tgas);
    SERIALIZE_FIELD_BT(m_account_pledge_balance.disk);
    SERIALIZE_FIELD_BT(m_account_pledge_balance.vote);
    SERIALIZE_FIELD_BT(m_account_lock_balance);
    SERIALIZE_FIELD_BT(m_account_lock_tgas);
    SERIALIZE_FIELD_BT(m_account_unvote_num);
    SERIALIZE_FIELD_BT(m_account_credit);
    SERIALIZE_FIELD_BT(m_account_nonce);
    SERIALIZE_FIELD_BT(m_account_create_time);
    SERIALIZE_FIELD_BT(m_account_status);
    SERIALIZE_FIELD_BT(m_property_hash);
    const_cast<xnative_property_t*>(&m_native_property)->serialize_to(stream);
    SERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    SERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

int32_t xaccount_mstate2::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_FIELD_BT(m_latest_send_trans_number);
    DESERIALIZE_FIELD_BT(m_latest_send_trans_hash);
    DESERIALIZE_FIELD_BT(m_latest_recv_trans_number);
    DESERIALIZE_FIELD_BT(m_latest_recv_trans_hash);
    DESERIALIZE_FIELD_BT(m_account_balance);
    DESERIALIZE_FIELD_BT(m_account_burn_balance);
    DESERIALIZE_FIELD_BT(m_account_pledge_balance.tgas);
    DESERIALIZE_FIELD_BT(m_account_pledge_balance.disk);
    DESERIALIZE_FIELD_BT(m_account_pledge_balance.vote);
    DESERIALIZE_FIELD_BT(m_account_lock_balance);
    DESERIALIZE_FIELD_BT(m_account_lock_tgas);
    DESERIALIZE_FIELD_BT(m_account_unvote_num);
    DESERIALIZE_FIELD_BT(m_account_credit);
    DESERIALIZE_FIELD_BT(m_account_nonce);
    DESERIALIZE_FIELD_BT(m_account_create_time);
    DESERIALIZE_FIELD_BT(m_account_status);
    DESERIALIZE_FIELD_BT(m_property_hash);
    m_native_property.serialize_from(stream);
    DESERIALIZE_FIELD_BT(m_unconfirm_sendtx_num);
    DESERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

std::string xaccount_mstate2::dump() const {
    std::stringstream ss;
    ss << " stx_num:"       << m_latest_send_trans_number;
    ss << " rtx_num:"       << m_latest_recv_trans_number;
    ss << " balance:"       << m_account_balance;
    ss << " burn_balance:"       << m_account_burn_balance;
    ss << " tgas:"          << m_account_pledge_balance.tgas;
    ss << " disk:"          << m_account_pledge_balance.disk;
    ss << " vote:"          << m_account_pledge_balance.vote;
    ss << " lock_balance:"  << m_account_lock_balance;
    ss << " unvote_num:"    << m_account_unvote_num;
    ss << " create_time:"   << m_account_create_time;
    // ss << " user_prop:"     << m_property_hash.size() << " ";
    // for (auto & v : m_property_hash) {
    //     ss << v.first << " ";
    // }
    auto native_prop = m_native_property.get_properties();
    ss << " native_prop:"   << native_prop.size() << " ";
    for (auto & v : native_prop) {
        ss << v.first << " ";
    }
    ss << " uncfm_n:"        << m_unconfirm_sendtx_num;

    return ss.str();
}

void xaccount_mstate2::set_propertys_hash_change(const std::map<std::string, std::string> & prop_hashs) {
    for (auto prop : prop_hashs) {
        m_property_hash[prop.first] = prop.second;
    }
}

void xaccount_mstate2::set_native_property_change(const xnative_property_t & native_prop) {
    if (false == native_prop.is_empty()) {
        m_native_property.add_property(native_prop);
    }
}

std::string xaccount_mstate2::get_property_hash(const std::string & prop_name) const {
    auto iter = m_property_hash.find(prop_name);
    if (iter != m_property_hash.end()) {
        return iter->second;
    }
    return {};
}
bool xaccount_mstate2::get_property_hash(const std::string & prop_name, std::string & hash) const {
    auto iter = m_property_hash.find(prop_name);
    if (iter != m_property_hash.end()) {
        hash = iter->second;
        return true;
    }
    return false;
}


}  // namespace data
}  // namespace top
