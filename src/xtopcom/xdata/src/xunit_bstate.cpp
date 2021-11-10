// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xunit_bstate.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xdata_common.h"
#include "xdata/xdata_error.h"
#include "xdata/xfullunit.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmetrics/xmetrics.h"
#include <assert.h>
#include <string>
#include <vector>

namespace top {
namespace data {

xunit_bstate_t::xunit_bstate_t(base::xvbstate_t* bstate) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_unit_state, 1);
    bstate->add_ref();
    m_bstate.attach(bstate);
}

xunit_bstate_t::~xunit_bstate_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_unit_state, -1);
    m_bstate->close();  // must do close firstly
    m_bstate = nullptr;
}

uint64_t xunit_bstate_t::get_free_tgas() const {
    uint64_t total_asset = balance() + lock_balance() + tgas_balance() + disk_balance() + vote_balance();
    if (total_asset >= XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset)) {
        return XGET_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
    } else {
        return 0;
    }
}

// how many tgas you can get from pledging 1TOP
uint32_t xunit_bstate_t::get_token_price(uint64_t onchain_total_pledge_token) {
    uint64_t initial_total_pledge_token = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
    xdbg("tgas_disk get total pledge token from beacon: %llu, %llu", initial_total_pledge_token, onchain_total_pledge_token);
    uint64_t total_pledge_token = onchain_total_pledge_token + initial_total_pledge_token;
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard) * XGET_CONFIG(validator_group_count) * TOP_UNIT / total_pledge_token;
}

uint64_t xunit_bstate_t::get_total_tgas(uint32_t token_price) const {
    uint64_t pledge_token = tgas_balance();
    uint64_t total_tgas = pledge_token * token_price / TOP_UNIT + get_free_tgas();
    uint64_t max_tgas;
    // contract account, max tgas is different
    if (is_user_contract_address(common::xaccount_address_t{get_account()})) {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
    } else {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
    }
    return std::min(total_tgas, max_tgas);
}

uint64_t xunit_bstate_t::get_last_tx_hour() const {
    std::string v;
    string_get(XPROPERTY_LAST_TX_HOUR_KEY, v);
    if (!v.empty()) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xunit_bstate_t::get_used_tgas() const {
    std::string v;
    string_get(XPROPERTY_USED_TGAS_KEY, v);
    if (!v.empty()) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xunit_bstate_t::calc_decayed_tgas(uint64_t timer_height) const {
    uint32_t last_hour = get_last_tx_hour();
    uint64_t used_tgas{0};
    uint32_t decay_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
    if (timer_height <= last_hour) {
        used_tgas = get_used_tgas();
    } else if (timer_height - last_hour < decay_time) {
        used_tgas = (decay_time - (timer_height - last_hour)) * get_used_tgas() / decay_time;
    }
    return used_tgas;
}

uint64_t xunit_bstate_t::get_available_tgas(uint64_t timer_height, uint32_t token_price) const {
    uint64_t used_tgas = calc_decayed_tgas(timer_height);
    uint64_t total_tgas = get_total_tgas(token_price);
    uint64_t available_tgas{0};
    if (total_tgas > used_tgas) {
        available_tgas = total_tgas - used_tgas;
    }
    return available_tgas;
}

bool xunit_bstate_t::string_get(const std::string& prop, std::string& value) const {
    if (false == get_bstate()->find_property(prop)) {
        xwarn("xunit_bstate_t::string_get fail-find property.account=%s,height=%ld,propname=%s", get_account().c_str(), get_block_height(), prop.c_str());
        return false;
    }
    auto propobj = get_bstate()->load_string_var(prop);
    if (nullptr == propobj) {
        xerror("xunit_bstate_t::string_get fail-find load string var.account=%s,propname=%s", get_account().c_str(), prop.c_str());
        return false;
    }
    value = propobj->query();
    return true;
}

bool xunit_bstate_t::deque_get(const std::string& prop, std::deque<std::string> & deque) const {
    if (false == get_bstate()->find_property(prop)) {
        xwarn("xunit_bstate_t::deque_get fail-find property.account=%s,propname=%s", get_account().c_str(), prop.c_str());
        return false;
    }
    auto propobj = get_bstate()->load_string_deque_var(prop);
    if (nullptr == propobj) {
        xerror("xunit_bstate_t::deque_get fail-find load deque var.account=%s,propname=%s", get_account().c_str(), prop.c_str());
        return false;
    }
    deque = propobj->query();
    return true;
}

bool xunit_bstate_t::map_get(const std::string& prop, std::map<std::string, std::string> & map) const {
    if (false == get_bstate()->find_property(prop)) {
        xwarn("xunit_bstate_t::map_get fail-find property.account=%s,propname=%s", get_account().c_str(), prop.c_str());
        return false;
    }
    auto propobj = get_bstate()->load_string_map_var(prop);
    if (nullptr == propobj) {
        xerror("xunit_bstate_t::map_get fail-find load map var.account=%s,propname=%s", get_account().c_str(), prop.c_str());
        return false;
    }
    map = propobj->query();
    return true;
}

uint64_t xunit_bstate_t::token_get(const std::string& prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return 0;
    }
    auto propobj = get_bstate()->load_token_var(prop);
    base::vtoken_t balance = propobj->get_balance();
    if (balance < 0) {
        xerror("xunit_bstate_t::token_get fail-should not appear. balance=%ld", balance);
        return 0;
    }
    return (uint64_t)balance;
}

uint64_t xunit_bstate_t::uint64_property_get(const std::string& prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return 0;
    }
    auto propobj = get_bstate()->load_uint64_var(prop);
    uint64_t value = propobj->get();
    return value;
}

std::string xunit_bstate_t::native_map_get(const std::string & prop, const std::string & field) const {
    if (false == get_bstate()->find_property(prop)) {
        return {};
    }
    auto propobj = get_bstate()->load_string_map_var(prop);
    return propobj->query(field);
}

std::string xunit_bstate_t::native_string_get(const std::string & prop) const {
    if (false == get_bstate()->find_property(prop)) {
        return {};
    }
    auto propobj = get_bstate()->load_string_var(prop);
    return propobj->query();
}

uint64_t xunit_bstate_t::get_account_create_time() const {
    uint64_t create_time = uint64_property_get(XPROPERTY_ACCOUNT_CREATE_TIME);
    if (create_time < base::TOP_BEGIN_GMTIME) {
        // tackle create_time set to be clock bug
        return create_time * 10 + base::TOP_BEGIN_GMTIME;
    } else {
        return create_time;
    }
}

uint32_t xunit_bstate_t::get_unconfirm_sendtx_num() const {
    std::string value = native_map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint32(value);
}
uint64_t xunit_bstate_t::get_latest_send_trans_number() const {
    std::string value = native_map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint64(value);
}

uint64_t xunit_bstate_t::account_recv_trans_number() const {
    std::string value = native_map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_RECVTX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint64(value);
}

uint256_t xunit_bstate_t::account_send_trans_hash() const {
    std::string value = native_map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
    if (value.empty()) {
        uint256_t default_value;
        return default_value;
    }
    return uint256_t((uint8_t*)value.c_str());
}
uint64_t xunit_bstate_t::account_send_trans_number() const {
    return get_latest_send_trans_number();
}

}  // namespace data
}  // namespace top
