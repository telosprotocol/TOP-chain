// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <cinttypes>

#include "xstore/xstore_error.h"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_property.h"

#include "xbase/xlog.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvledger.h"
#include "xcrypto/xckey.h"
#include "xdata/xaction_parse.h"
#include "xdata/xproperty.h"
#include "xdata/xdata_defines.h"
#include "xdata/xchain_param.h"
#include "xdata/xtransaction_maker.hpp"
#include "xbasic/xutility.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xconfig_register.h"
#include "xutility/xstream_util.hpp"
#include "xvm/manager/xcontract_address_map.h"
#include "xconfig/xconfig_register.h"
#include "xstake/xstake_algorithm.h"
#include "xchain_upgrade/xchain_upgrade_center.h"

using namespace top::base;

namespace top { namespace store {

#define CHECK_UNIT_HEIGHT_SHOULD_BE_ZERO() \
do {\
    if (m_account->get_chain_height() != 0) {\
        return xaccount_unit_height_should_zero;\
    }\
}while(0)

#define CHECK_PROPERTY_MAX_NUM() \
do {\
    auto& config_register = top::config::xconfig_register_t::get_instance(); \
    uint32_t custom_property_max_number =XGET_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_max_number); \
    if ((uint32_t)m_account->get_property_hash_map().size() >= custom_property_max_number) {\
        return xaccount_property_number_exceed_max;\
    }\
}while(0)

#define CHECK_PROPERTY_NAME_LEN_MAX_NUM(key) \
do {\
    auto& config_register = top::config::xconfig_register_t::get_instance(); \
    uint32_t custom_property_name_max_len = XGET_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len); \
    if ((key).length() >= custom_property_name_max_len) {\
        return xaccount_property_name_length_exceed_max;\
    }\
}while(0)

#define CHECK_CREATE_NATIVE_PROPERTY(name) \
do {\
    if (xnative_property_name_t::is_native_property(name)) {\
        xerror("xstore_user_contract_should_not_do_native_property");\
        return xstore_user_contract_should_not_do_native_property;\
    }\
}while(0)

#define CHECK_PROPERTY_NAME_NATIVE_RIGHT(name, native_flag) \
do {\
    if (!native_flag && xnative_property_name_t::is_native_property(name)) {\
        xerror("xstore_user_contract_should_not_do_native_property");\
        return xstore_user_contract_should_not_do_native_property;\
    }\
}while(0)

#define STRING_PROP_READ_GET(prop) \
do {\
    int32_t error = 0;\
    (prop) = string_read_get(key, error);\
    if ((prop) == nullptr) {return error;}\
}while(0)

#define DEQUE_PROP_READ_GET(prop) \
do {\
    int32_t error = 0;\
    (prop) = deque_read_get(key, error);\
    if ((prop) == nullptr) {return error;}\
}while(0)

#define MAP_PROP_READ_GET(prop) \
do {\
    int32_t error = 0;\
    (prop) = map_read_get(key, error);\
    if ((prop) == nullptr) {return error;}\
}while(0)

xaccount_context_t::xaccount_context_t(const std::string& address,
                                       xstore_face_t* store)
: m_address(address)
, m_store(store) {
    m_account = m_store->clone_account(address);
    if (nullptr == m_account) {
        m_account = new xblockchain2_t(m_address);
    } else {
        m_native_property = m_account->get_native_property();
        m_native_property.clear_dirty();
    }
    m_accountcmd = std::make_shared<xaccount_cmd>(m_account, store);
    xinfo("create context, address:%s height:%d", address.c_str(), m_account->get_chain_height());
}

xaccount_context_t::xaccount_context_t(data::xblockchain2_t* blockchain, xstore_face_t* store)
: m_address(blockchain->get_account()), m_store(store) {
    m_account = blockchain;
    blockchain->add_ref();
    m_native_property = m_account->get_native_property();
    m_native_property.clear_dirty();
    m_accountcmd = std::make_shared<xaccount_cmd>(m_account, store);
    xinfo("create context, address:%s height:%d", blockchain->get_account().c_str(), m_account->get_chain_height());
}

xaccount_context_t::~xaccount_context_t() {
    if (m_account != nullptr) {
        m_account->release_ref();
    }
}

int32_t xaccount_context_t::create_user_account(const std::string& address) {
    assert(address == m_address);
    xinfo("xaccount_context_t::create_user_account address:%s", address.c_str());
    CHECK_UNIT_HEIGHT_SHOULD_BE_ZERO();
    set_lock_token_sum(XGET_CONFIG(min_account_deposit));
    xinfo("tmp create account lock sum %d, for authority", XGET_CONFIG(min_account_deposit));
    // just for test debug
    m_balance_change = ASSET_TOP(100000000);
    return xstore_success;
}

int32_t xaccount_context_t::exist_sub_or_contract_account() {
    int32_t size = 0;
    if (!m_native_property.native_deque_size(XPORPERTY_SUB_ACCOUNT_KEY, size) && size > 0) {
        return xaccount_property_sub_account_exist;
    }

    if (!m_native_property.native_deque_size(XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY, size) && size > 0) {
        return xaccount_property_contract_sub_account_exist;
    }
    return xstore_success;
}

int32_t xaccount_context_t::sub_contract_sub_account_check(const std::string& value) {
    if (m_native_property.native_deque_exist(XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY, value)) {
        return xaccount_property_sub_account_exist;
    }
    int32_t size;
    if (!m_native_property.native_deque_size(XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY, size) && size >= MAX_NORMAL_CONTRACT_ACCOUNT) {
        return xaccount_property_sub_account_overflow;
    }
    return xstore_success;
}
int32_t xaccount_context_t::set_contract_sub_account(const std::string& value) {
    return m_native_property.native_deque_push_back(XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::set_contract_parent_account(const uint64_t amount, const std::string& value) {
    std::string _value;
    if (!m_native_property.native_string_get(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, _value)) {
        return xaccount_property_parent_account_exist;
    }
    uint64_t balance = amount > XGET_CONFIG(min_account_deposit) ? (amount - XGET_CONFIG(min_account_deposit)) : 0;
    set_lock_token_sum(XGET_CONFIG(min_account_deposit));
    top_token_transfer_in(balance);
    return m_native_property.native_string_set(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::get_parent_account(std::string &value){
    if (!m_native_property.native_string_get(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, value)) {
        return xaccount_property_parent_account_exist;
    }
    return xaccount_property_parent_account_not_exist;
}

int32_t xaccount_context_t::account_alias_name_set(const std::string& name) {
    uint64_t lock_sum = get_lock_token_sum();
    if (!can_work()) {
        xwarn("xaccount_context_t::lock_sum %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }
    if (m_account->get_chain_height() == 0) {
        xwarn("xaccount_context_t::create_user_account unit height not zero");
        return xaccount_unit_height_should_not_zero;
    }
    return m_native_property.native_string_set(data::XPROPERTY_ALIAS_NAME, name);
}

int32_t xaccount_context_t::token_transfer_out(const data::xproperty_asset& asset, uint64_t gas_fee, uint64_t service_fee) {
    if (asset.is_top_token()) {
        return top_token_transfer_out(asset.m_amount, gas_fee, service_fee);
    } else {
        return other_token_transfer_out(asset, gas_fee, service_fee);
    }
}

int32_t xaccount_context_t::other_token_transfer_out(const data::xproperty_asset& asset, uint64_t gas_fee, uint64_t service_fee) {
    std::string token_value;
    int32_t ret = m_native_property.native_map_get(XPORPERTY_CONTRACT_NATIVE_TOKEN_KEY, asset.m_token_name, token_value);
    if (!ret && std::stoull(token_value) >= asset.m_amount) {
        uint64_t remain = std::stoull(token_value) - asset.m_amount;
        ret = m_native_property.native_map_set(XPORPERTY_CONTRACT_NATIVE_TOKEN_KEY, asset.m_token_name, std::to_string(remain));
        if (!ret && (gas_fee > 0 || service_fee > 0)) {
            return top_token_transfer_out(0, gas_fee, service_fee);
        }
    } else {
        return xaccount_balance_not_enough;
    }
    return ret;
}

int32_t xaccount_context_t::top_token_transfer_out(uint64_t amount, uint64_t gas_fee, uint64_t service_fee) {
    xinfo("xaccount_context_t::transfer_out amount:%d, gas_fee: %d, service_fee: %d", amount, gas_fee, service_fee);
    if(m_account->address() == sys_contract_zec_reward_addr) {
        xinfo("xaccount_context_t::transfer_out account address is %s", sys_contract_zec_reward_addr);
        return xstore_success;
    }
    uint64_t lock_sum = get_lock_token_sum();
    if (amount && !can_work()) {
        xwarn("xaccount_context_t::lock_sum %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }
    uint64_t balance = m_account->balance();
    uint64_t deduct_fee  = add(amount, gas_fee, service_fee);
    if ( static_cast<int64_t>(balance + m_balance_change) < static_cast<int64_t>(deduct_fee)) {
        xwarn("xaccount_context_t::transfer_out balance(%ld) change(%ld)less than deduct_fee(%ld)", balance, m_balance_change, deduct_fee);
        return xaccount_balance_not_enough;
    }
    m_balance_change -= deduct_fee;
    return xstore_success;
}

int32_t xaccount_context_t::token_transfer_in(const data::xproperty_asset& asset) {
    if (asset.is_top_token()) {
        return top_token_transfer_in(asset.m_amount);
    } else {
        return other_token_transfer_in(asset);
    }
}

int32_t xaccount_context_t::other_token_transfer_in(const data::xproperty_asset& asset) {
    if (m_account->balance() == 0 && get_lock_token_sum() == 0) {
        if (!data::is_sys_contract_address(common::xaccount_address_t{ m_address })) {
            return xaccount_account_not_exist;
        }
    }
    uint64_t balance = asset.m_amount;
    std::string token_value;
    int32_t ret = m_native_property.native_map_get(XPORPERTY_CONTRACT_NATIVE_TOKEN_KEY, asset.m_token_name, token_value);
    if (!ret) {
        balance = add(static_cast<uint64_t>(std::stoull(token_value)), asset.m_amount);
    }
    return m_native_property.native_map_set(XPORPERTY_CONTRACT_NATIVE_TOKEN_KEY, asset.m_token_name, std::to_string(balance));
}

int32_t xaccount_context_t::top_token_transfer_in(uint64_t amount) {
    xinfo("xaccount_context_t::transfer_in amount:%d", amount);
    if (!amount) {
        xdbg("no amount transfer in");
        return xstore_success;
    }

    if (data::is_sys_contract_address(common::xaccount_address_t{ m_address })) {
        m_balance_change += amount;

        return xstore_success;
    }

    const uint64_t safe_amount = 1;
    uint64_t lock_amount = 0;
    uint64_t lock_sum = get_lock_token_sum();
    if (lock_sum == 0/*new account*/) {
        if (amount < safe_amount) {
            return xaccount_transfer_less_than_safe_amount;
        }
        lock_amount = ((amount > XGET_CONFIG(min_account_deposit)) ? XGET_CONFIG(min_account_deposit) : amount);
        set_lock_token_sum(lock_amount);
    } else if (lock_sum < XGET_CONFIG(min_account_deposit)){
        uint64_t need_lock = XGET_CONFIG(min_account_deposit) - lock_sum;
        lock_amount = ((amount > need_lock) ? need_lock : amount);
        set_lock_token_sum(lock_amount +  lock_sum);
    }

    m_balance_change += (amount - lock_amount);

    return xstore_success;
}

int32_t xaccount_context_t::account_set_keys(const std::string &key, const std::string &value) {
    xinfo("xaccount_context_t::account_set_keys, [%s:%s]", key.c_str(), value.c_str());
    uint64_t lock_sum = get_lock_token_sum();
    if (!can_work()) {
        xwarn("xaccount_context_t::lock_sum %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }
    if (key != XPROPERTY_ACCOUNT_VOTE_KEY
        && key != XPROPERTY_ACCOUNT_TRANSFER_KEY
        && key != XPROPERTY_ACCOUNT_DATA_KEY
        && key != XPROPERTY_ACCOUNT_CONSENSUS_KEY) {
        return xaccount_set_keys_key_illegal;
    }
    // TODO (ernest) check value = 65
    return m_native_property.native_map_set(data::XPROPERTY_ACCOUNT_KEYS, key, value);
}

uint64_t xaccount_context_t::get_lock_token_sum() {
    int32_t ret = 0;
    std::string v;
    ret = m_native_property.native_string_get(XPROPERTY_LOCK_TOKEN_SUM_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_lock_token_sum(uint64_t sum) {
    m_native_property.native_string_set(XPROPERTY_LOCK_TOKEN_SUM_KEY, std::to_string(sum));
    xdbg("tgas_disk account: %s, lock_token_sum: %d", m_address.c_str(), sum);
    return 0;
}

// how many tgas you can get from pledging 1TOP
uint32_t xaccount_context_t::get_token_price() const {
    return m_account->get_token_price(m_sys_total_lock_tgas_token);
}

int32_t xaccount_context_t::set_pledge_token_tgas(uint64_t num) {
    if(num <= 0){
        return xtransaction_non_positive_pledge_token;
    }
    if(m_account->balance() < num){
        return xtransaction_not_enough_token;
    }

    auto sum = num + m_account->tgas_balance();
    xdbg("tgas_disk sum: %ull, num: %ull, old_pledge_tgas: %ull", sum, num, m_account->tgas_balance());

    m_pledge_balance_change.tgas += num;
    m_account->set_tgas_balance(sum);
    return 0;
}

int32_t xaccount_context_t::redeem_pledge_token_tgas(uint64_t num) {
    uint64_t pledge_token = m_account->tgas_balance();
    if(num > pledge_token){
        return xtransaction_not_enough_pledge_token_tgas;
    }
    m_pledge_balance_change.tgas -= num;
    m_account->set_tgas_balance(pledge_token - num);
    return 0;
}

uint64_t xaccount_context_t::get_used_tgas(){
    int32_t ret = 0;
    std::string v;
    ret = m_native_property.native_string_get(XPROPERTY_USED_TGAS_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_used_tgas(uint64_t num){
    m_native_property.native_string_set(XPROPERTY_USED_TGAS_KEY, std::to_string(num));
    m_native_property.native_string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(m_timer_height));
    return 0;
}

int32_t xaccount_context_t::incr_used_tgas(uint64_t num){
    auto used_tgas = calc_decayed_tgas();
    m_native_property.native_string_set(XPROPERTY_USED_TGAS_KEY, std::to_string(num + used_tgas));
    m_native_property.native_string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(m_timer_height));
    return 0;
}

uint64_t xaccount_context_t::calc_decayed_tgas(){
    uint32_t last_hour = get_last_tx_hour();
    uint64_t used_tgas{0};
    uint32_t decay_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
    if(m_timer_height - last_hour < decay_time){
        used_tgas = (decay_time - (m_timer_height - last_hour)) * get_used_tgas() / decay_time;
    }
    return used_tgas;
}

uint64_t xaccount_context_t::get_tgas_limit(){
    int32_t ret = 0;
    std::string v;
    ret = m_native_property.native_string_get(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::check_used_tgas(uint64_t &cur_tgas_usage, uint64_t deposit, uint64_t& deposit_usage){
    uint32_t last_hour = get_last_tx_hour();
    xdbg("tgas_disk last_hour: %d, m_timer_height: %d, no decay used_tgas: %d, used_tgas: %d, pledge_token: %d, token_price: %u, total_tgas: %d, tgas_usage: %d, deposit: %d",
          last_hour, m_timer_height, get_used_tgas(), calc_decayed_tgas(), m_account->tgas_balance(), get_token_price(), get_total_tgas(), cur_tgas_usage, deposit);

    auto available_tgas = get_available_tgas();
    xdbg("tgas_disk account: %s, total tgas usage adding this tx : %d", m_address.c_str(), cur_tgas_usage);
    if(cur_tgas_usage > (available_tgas + deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio))){
        xdbg("tgas_disk xtransaction_not_enough_pledge_token_tgas");
        deposit_usage = deposit;
        cur_tgas_usage = available_tgas;
        return xtransaction_not_enough_pledge_token_tgas;
    } else if(cur_tgas_usage > available_tgas){
        deposit_usage = (cur_tgas_usage - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        cur_tgas_usage = available_tgas;
        xdbg("tgas_disk tx deposit_usage: %d", deposit_usage);
    }
    return 0;
}

int32_t xaccount_context_t::update_tgas_sender(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage, bool is_contract){
    auto ret = check_used_tgas(tgas_usage, deposit, deposit_usage);
    // if(ret == 0 && !is_contract){
    if(ret == 0){
        // deposit -= deposit_usage;
        incr_used_tgas(tgas_usage);
    } else {
        // for contract tx, the used deposit will be recalculated in unfreeze tx
        // deposit_usage = 0;
    }
    return ret;
}

int32_t xaccount_context_t::update_tgas_contract_recv(uint64_t sender_used_deposit, const uint32_t deposit, uint64_t& deposit_usage, uint64_t& send_frozen_tgas, uint64_t deal_used_tgas){
    uint64_t available_tgas = get_available_tgas();
    xdbg("tgas_disk contract tx tgas calc, sender_used_deposit: %d, deposit: %d, deposit_usage: %d send_frozen_tgas: %d, available_tgas: %d, tgas_limit: %d",
                                           sender_used_deposit, deposit, deposit_usage, send_frozen_tgas, available_tgas, m_tgas_limit);
    if(send_frozen_tgas >= deal_used_tgas){
        send_frozen_tgas -= deal_used_tgas;
    } else {
        deal_used_tgas -= send_frozen_tgas;
        uint64_t min_tgas = std::min(available_tgas, std::min(send_frozen_tgas, m_tgas_limit));
        auto old_send_frozen_tgas = send_frozen_tgas;
        send_frozen_tgas = 0;
        if(deal_used_tgas <= min_tgas){
            uint64_t used_tgas = calc_decayed_tgas();
            used_tgas += deal_used_tgas;
            m_cur_used_tgas = deal_used_tgas;
            xdbg("tgas_disk contract tx tgas calc min_tgas enough, used_tgas: %d, deal_used_tgas: %d, min_tgas: %d", used_tgas, deal_used_tgas, min_tgas);
            return set_used_tgas(used_tgas);
        } else {
            deal_used_tgas -= min_tgas;
            m_cur_used_tgas = min_tgas;

            uint64_t deposit_tgas = (deposit - sender_used_deposit) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
            xassert(deposit_tgas >= old_send_frozen_tgas);
            deposit_tgas -= old_send_frozen_tgas;
            if(deal_used_tgas > deposit_tgas){
                xdbg("tgas_disk contract tx tgas calc not enough tgas, deal_used_tgas: %d, deposit: %d", deal_used_tgas, deposit);
                return xtransaction_contract_not_enough_tgas;
            } else {
                deposit_usage = deal_used_tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
                xdbg("tgas_disk contract tx tgas calc deposit_usage: %d", deposit_usage);
                uint64_t used_tgas = calc_decayed_tgas();
                used_tgas += min_tgas;
                xdbg("tgas_disk contract tx tgas calc min_tgas + deposit enough, used_tgas: %d, deal_used_tgas: %d, min_tgas: %d", used_tgas, deal_used_tgas, min_tgas);
                return set_used_tgas(used_tgas);
            }
        }
    }

    return 0;
}

uint64_t xaccount_context_t::get_total_tgas() const {
    return m_account->get_total_tgas(get_token_price());
}

uint64_t xaccount_context_t::get_available_tgas() const {
    return m_account->get_available_tgas(m_timer_height, get_token_price());
}

int32_t xaccount_context_t::calc_resource(uint64_t& tgas, uint32_t deposit, uint32_t& used_deposit){
    uint64_t used_tgas = calc_decayed_tgas();
    auto available_tgas = get_available_tgas();
    int32_t ret{0};
    xdbg("tgas_disk used_tgas: %llu, available_tgas: %llu, tgas: %llu, deposit: %u, used_deposit: %u", used_tgas, available_tgas, tgas, deposit, used_deposit);
    if(tgas > (available_tgas + (deposit - used_deposit) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio))){
        xdbg("tgas_disk xtransaction_not_enough_pledge_token_tgas");
        set_used_tgas(available_tgas + used_tgas);
        tgas = available_tgas - used_tgas;
        used_deposit = deposit;
        ret = xtransaction_not_enough_pledge_token_tgas;
    } else if(tgas > available_tgas){
        used_deposit += (tgas - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        set_used_tgas(available_tgas + used_tgas);
    } else {
        set_used_tgas(used_tgas + tgas);
    }

    xdbg("tgas_disk deposit: %u, used_deposit: %u", deposit, used_deposit);

    m_balance_change -= used_deposit;
    return ret;
}

uint64_t xaccount_context_t::get_last_tx_hour(){
    int32_t ret = 0;
    std::string v;
    ret = m_native_property.native_string_get(XPROPERTY_LAST_TX_HOUR_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_last_tx_hour(uint64_t num){
    m_native_property.native_string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(num));
    return 0;
}

uint64_t xaccount_context_t::get_pledge_token_disk(){
    return m_account->disk_balance();
}

int32_t xaccount_context_t::set_pledge_token_disk(uint64_t num){
    if(m_account->balance() < num){
        return xtransaction_pledge_too_much_token;
    }
    m_pledge_balance_change.disk += num;
    num += get_pledge_token_disk();
    m_account->set_disk_balance(num);
    return 0;
}

int32_t xaccount_context_t::redeem_pledge_token_disk(uint64_t num){
    auto pledge_token = get_pledge_token_disk();
    if(pledge_token < num){
        return xtransaction_not_enough_pledge_token_disk;
    }
    m_pledge_balance_change.disk -= num;
    m_account->set_disk_balance(pledge_token - num);
    return 0;
}

uint64_t xaccount_context_t::get_used_disk(){
    int32_t ret = 0;
    std::string v;
    ret = m_native_property.native_string_get(XPROPERTY_USED_DISK_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_used_disk(uint64_t num){
    m_native_property.native_string_set(XPROPERTY_USED_DISK_KEY, std::to_string(num));
    return 0;
}

int32_t xaccount_context_t::update_disk(uint64_t disk_usage){
#if 0 // disk is 0 at present
    uint64_t used_disk = get_used_disk();
    uint64_t pledge_disk = get_pledge_token_disk() / config::config_register.get<xdisk_price_per_byte_configuration_t>();
    uint64_t total_disk = pledge_disk + config::config_register.get<xfree_disk_configuration_t>();
    xdbg("tgas_disk disk_usage: %d, used_disk: %d, pledge_disk: %d, total_disk: %d", disk_usage, used_disk, pledge_disk, total_disk);
    disk_usage += used_disk;
    if(disk_usage > total_disk){
        return xtransaction_not_enough_pledge_token_disk;
    }

    auto ret = set_used_disk(disk_usage);
    return ret;
#else
    return 0;
#endif
}

bool xaccount_context_t::can_work() {
    if (data::is_sys_contract_address(common::xaccount_address_t{ m_address })) {
        return true;
    }
    uint64_t lock_sum = get_lock_token_sum();
    if (lock_sum < XGET_CONFIG(min_account_deposit)) {
        return false;
    }
    return true;
}

int32_t xaccount_context_t::lock_token(const uint256_t &tran_hash, uint64_t amount, const std::string &tran_params) {
    uint64_t balance = m_account->balance();
    uint64_t lock_sum = get_lock_token_sum();
    if (!can_work()) {
        xwarn("xaccount_context_t::lock_token, lock_sum %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }
    assert((balance + m_balance_change) >= 0);
    if ((balance + m_balance_change) < amount) {
        xwarn("xaccount_context_t::lock_token balance(%d) change(%d)less than amount(%d)", balance, m_balance_change, amount);
        return xaccount_lock_less_min_deposit;
    }
    int32_t ret = 0;
    std::string v;
    std::string hash_str((char *)tran_hash.data(), tran_hash.size());
    ret = m_native_property.native_map_get(XPROPERTY_LOCK_TOKEN_KEY, hash_str, v);
    if (0 == ret) {
        xwarn("xaccount_context_t::lock_token failed, same tx hash %s", hash_str.c_str());
        return xaccount_property_lock_token_key_same;
    }
    uint64_t clock_height = m_timer_height;
    m_balance_change -= amount;
    base::xstream_t stream(base::xcontext_t::instance());
    stream << clock_height;
    stream << tran_params;
    xinfo("xaccount_context_t::lock_token, account: %s, set lock token %d, clock_height %d, locked token sum %d, tx hash %s",
           m_address.c_str(), amount, clock_height, lock_sum + amount, to_hex_str(hash_str).c_str());
    m_native_property.native_map_set(XPROPERTY_LOCK_TOKEN_KEY, hash_str, std::string((char *)stream.data(), stream.size()));

    set_lock_token_sum(lock_sum + amount);

    return xstore_success;
}

int32_t xaccount_context_t::unlock_token(const uint256_t &tran_hash, const std::string &lock_hash_str, const std::vector<std::string> trans_signs) {

    std::string v;
    uint64_t lock_sum = get_lock_token_sum();
    if (!can_work()) {
        xwarn("xaccount_context_t::unlock_token, lock_sum %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }
    int32_t ret = m_native_property.native_map_get(XPROPERTY_LOCK_TOKEN_KEY, lock_hash_str, v);
    if (0 != ret) {
        xinfo("xaccount_context_t::unlock_token failed, tx_hash %s not exist", to_hex_str(lock_hash_str).c_str());
        return xaccount_property_unlock_token_key_not_exist;
    }

    base::xstream_t stream(base::xcontext_t::instance(),
                           (uint8_t *)v.c_str(),
                           (uint32_t)v.size());
    uint64_t now_clock = m_timer_height;
    uint64_t clock_timer;
    std::string raw_input;
    stream >> clock_timer;
    stream >> raw_input;

    data::xaction_lock_account_token lock_action;
    ret = lock_action.parse_param(raw_input);
    assert(0 == ret);
    if (lock_action.m_unlock_type == xaction_lock_account_token::UT_time) {
        uint64_t dure = xstring_utl::touint64(lock_action.m_unlock_values.at(0));
        xwarn("xaccount_context_t::unlock_token duration: %s, %d", lock_action.m_unlock_values.at(0).c_str(), dure);
        if ((now_clock - clock_timer) * 10 < dure) {
            xwarn("xaccount_context_t::unlock_token %d, but time not reach, lock clock %llu, now clock %llu", lock_action.m_amount, clock_timer, now_clock);
            return xaccount_property_unlock_token_time_not_reach;
        }
    }
    else {
        std::vector<utl::xecpubkey_t> pubkeys;
        std::vector<utl::xecdsasig_t> signs;
        for (auto &i : lock_action.m_unlock_values) {
            pubkeys.push_back(utl::xecpubkey_t((uint8_t *)i.c_str()));
        }
        for (auto &i : trans_signs) {
            signs.push_back(utl::xecdsasig_t((uint8_t *)i.c_str()));
        }
        // bool ilegal = consensus::xsign_common::check_rate_sign(1.0, tran_hash, pubkeys, signs);
        bool ilegal = true;  // TODO(jimmy)
        if (!ilegal) {
            xwarn("xaccount_context_t::unlock_token %d, but signs not illegal", lock_action.m_amount);
            return xaccount_property_unlock_token_sign_illegal;
        }
    }
    assert(lock_sum >= lock_action.m_amount);
    xinfo("xaccount_context_t::unlock_token account: %s, amount %d, locked token sum %d, tx hash: %s, lock hash: %s",
           m_address.c_str(), lock_action.m_amount, lock_sum - lock_action.m_amount, to_hex_str(tran_hash).c_str(), to_hex_str(lock_hash_str).c_str());

    ret = m_native_property.native_map_erase(XPROPERTY_LOCK_TOKEN_KEY, lock_hash_str);
    if (0 != ret) {
        xerror("xaccount_context_t::unlock_token tx erase failed, tx_hash %s not exist", to_hex_str(lock_hash_str).c_str());
        return xaccount_property_unlock_token_key_not_exist;
    }

    set_lock_token_sum(lock_sum - lock_action.m_amount);

    m_balance_change += lock_action.m_amount;

    return xstore_success;
}

int32_t xaccount_context_t::unlock_all_token(){
    std::string v;
    uint64_t lock_sum = get_lock_token_sum();
    if (!can_work()) {
        xwarn("xaccount_context_t::unlock_all_token %d less than min_deposit %d", lock_sum, XGET_CONFIG(min_account_deposit));
        return xaccount_balance_not_enough;
    }

    auto lock_txs_ptr = m_native_property.map_get(XPROPERTY_LOCK_TOKEN_KEY);
    if(lock_txs_ptr == nullptr){
        xdbg("m_native_property.map_get(XPROPERTY_LOCK_TOKEN_KEY) is nullptr");
        return 0;
    }
    auto lock_txs = lock_txs_ptr->get_map();
    for(auto tx : lock_txs){
        int32_t ret = m_native_property.native_map_get(XPROPERTY_LOCK_TOKEN_KEY, tx.first, v);
        xdbg("xaccount_context_t::unlock_all_token, first: %s, second: %s, ret=%d", to_hex_str(tx.first).c_str(), tx.second.c_str(), ret);

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)v.c_str(), (uint32_t)v.size());
        uint64_t now_clock = m_timer_height;
        uint64_t clock_timer;
        std::string raw_input;
        stream >> clock_timer;
        stream >> raw_input;

        data::xaction_lock_account_token lock_action;
        ret = lock_action.parse_param(raw_input);
        assert(0 == ret);
        if (lock_action.m_unlock_type == xaction_lock_account_token::UT_time) {
            uint64_t dure = xstring_utl::touint64(lock_action.m_unlock_values.at(0));
            xwarn("xaccount_context_t::unlock_all_token duration: %s, %d", lock_action.m_unlock_values.at(0).c_str(), dure);
            if ((now_clock - clock_timer) * 10 < dure) {
                xwarn("xaccount_context_t::unlock_all_token %d, but time not reach, lock clock %llu, now clock %llu", lock_action.m_amount, clock_timer, now_clock);
                continue;
            }
        } else {
            continue;
        }
        assert(lock_sum >= lock_action.m_amount);
        xinfo("xaccount_context_t::unlock_all_token account: %s, amount: %d, locked token sum %d, unlock amount: %d, lock clock %llu, now clock %llu, tx hash: %s",
               m_address.c_str(), lock_action.m_amount, lock_sum, lock_action.m_amount, clock_timer, now_clock, to_hex_str(tx.first).c_str());

        ret = m_native_property.native_map_erase(XPROPERTY_LOCK_TOKEN_KEY, tx.first);
        if (0 != ret) {
            xerror("xaccount_context_t::unlock_all_token tx erase failed, tx_hash %s not exist", to_hex_str(tx.first).c_str());
            return xaccount_property_unlock_token_key_not_exist;
        }

        set_lock_token_sum(lock_sum - lock_action.m_amount);

        m_balance_change += lock_action.m_amount;
    }

    return xstore_success;
}

int32_t xaccount_context_t::authorize_key(uint32_t author_type, const std::string &params) {
    //TODO author_type maybe not limit in transfer
    (void)author_type;
    m_native_property.native_map_set(XPROPERTY_AUTHORIZE_KEY, XPROPERTY_ACCOUNT_TRANSFER_KEY, params);
    return xstore_success;
}

void xaccount_context_t::deserilize_vote(const std::string& str, uint64_t& vote_num, uint16_t& duration, uint64_t& lock_time){
    base::xstream_t stream{xcontext_t::instance(), (uint8_t*)str.data(), static_cast<uint32_t>(str.size())};
    stream >> vote_num;
    stream >> duration;
    stream >> lock_time;
}

std::string xaccount_context_t::serilize_vote(uint64_t vote_num, uint16_t duration, uint64_t lock_time){
    base::xstream_t stream{xcontext_t::instance()};
    stream << vote_num;
    stream << duration;
    stream << lock_time;
    return std::string((char*)stream.data(), stream.size());
}

// only merge expire lock record
int32_t xaccount_context_t::merge_pledge_vote_property(){
    auto dp = m_native_property.deque_get(XPROPERTY_PLEDGE_VOTE_KEY);
    if(dp == nullptr){
        return 0;
    }
    auto ds = dp.get()->get_deque();
    uint64_t vote_num{0};
    uint16_t duration{0};
    uint64_t lock_time{0};
    uint64_t vote_sum{0};
    uint64_t expire_token{0};
    for(auto str: ds){
        deserilize_vote(str, vote_num, duration, lock_time);
        // if expire
#ifdef DEBUG
        if(m_timer_height - lock_time >= duration / 60){
#else
        if(m_timer_height - lock_time >= duration * 24 * 60 * 6){
#endif
            m_native_property.native_deque_erase(XPROPERTY_PLEDGE_VOTE_KEY, str);
            vote_sum += vote_num;
            if(0 != duration){ // if not calculated in XPROPERTY_EXPIRE_VOTE_TOKEN_KEY
                expire_token += (TOP_UNIT * vote_num / config::get_top_vote_rate(duration));
            }
        }
        xdbg("pledge_redeem_vote expire %s, %d, %d, %d, %d, %d", str.c_str(), vote_num, duration, lock_time, m_timer_height, vote_sum);
    }
    if(vote_sum > 0){
        m_native_property.native_deque_push_back(XPROPERTY_PLEDGE_VOTE_KEY, serilize_vote(vote_sum, 0, 0));
    }
    if(expire_token > 0){
        std::string val;
        m_native_property.native_string_get(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, val);
        expire_token += xstring_utl::touint64(val);
        xdbg("pledge_redeem_vote expire old: %llu, new: %llu", xstring_utl::touint64(val), expire_token);
        m_native_property.native_string_set(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, xstring_utl::tostring(expire_token));
    }
    return 0;
}

int32_t xaccount_context_t::insert_pledge_vote_property(xaction_pledge_token_vote& action){
    int32_t size{0};
    auto ret = m_native_property.native_deque_size(XPROPERTY_PLEDGE_VOTE_KEY, size);
    const int32_t max_property_deque_size{128};
    if(ret == 0 && size >= max_property_deque_size){
        xdbg("pledge_redeem_vote XPROPERTY_PLEDGE_VOTE_KEY size overflow %d", size);
        return xtransaction_pledge_redeem_vote_err;
    }

    auto dp = m_native_property.deque_get(XPROPERTY_PLEDGE_VOTE_KEY);
    std::deque<std::string> ds;
    if(dp != nullptr){
        ds = dp.get()->get_deque();
    }
    uint64_t vote_num{0};
    uint16_t duration{0};
    uint64_t lock_time{0};
    for(auto str: ds){
        deserilize_vote(str, vote_num, duration, lock_time);
        xdbg("pledge_redeem_vote %s, %d, %d, %d, %d, action: duration %d, num %d",
              str.c_str(), vote_num, duration, lock_time, m_timer_height, action.m_lock_duration, action.m_vote_num);
#ifdef DEBUG
        if(action.m_lock_duration == duration && (m_timer_height - lock_time) < 10){
#else
        // merge vote with same duration in 24 hours
        if(action.m_lock_duration == duration && (m_timer_height - lock_time) < 24 * 60 * 6){
#endif
            m_native_property.native_deque_erase(XPROPERTY_PLEDGE_VOTE_KEY, str);
            action.m_vote_num += vote_num;
            break;
        }
    }
    return m_native_property.native_deque_push_back(XPROPERTY_PLEDGE_VOTE_KEY,
    serilize_vote(action.m_vote_num, action.m_lock_duration, lock_time == 0 ? m_timer_height : lock_time));
}

int32_t xaccount_context_t::update_pledge_vote_property(xaction_pledge_token_vote& action){
    merge_pledge_vote_property();
    return insert_pledge_vote_property(action);
}

int32_t xaccount_context_t::redeem_pledge_vote_property(uint64_t num){
    auto dp = m_native_property.deque_get(XPROPERTY_PLEDGE_VOTE_KEY);
    std::deque<std::string> ds;
    if(dp != nullptr){
        ds = dp.get()->get_deque();
    }
    uint64_t vote_num{0};
    uint16_t duration{0};
    uint64_t lock_time{0};
    for(auto str: ds){
        deserilize_vote(str, vote_num, duration, lock_time);
        if(0 == duration){
            if(num > vote_num){
                xdbg("pledge_redeem_vote, redeem_num: %llu, expire_num: %llu, duration: %u, lock_time: %llu", num, vote_num, duration, lock_time);
                return xtransaction_pledge_redeem_vote_err;
            }
            m_native_property.native_deque_erase(XPROPERTY_PLEDGE_VOTE_KEY, str);
            std::string val;
            m_native_property.native_string_get(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, val);
            uint64_t expire_token = xstring_utl::touint64(val);
            int64_t balance_change = num * expire_token / vote_num;
            m_native_property.native_string_set(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, xstring_utl::tostring(expire_token - balance_change));
            vote_num -= num;
            if(vote_num > 0){ // if not all vote been redeemed
                m_native_property.native_deque_push_back(XPROPERTY_PLEDGE_VOTE_KEY, serilize_vote(vote_num, 0, 0));
            }
            set_unvote_num_change(-num);
            set_balance_change(balance_change);
            set_vote_balance_change(-balance_change);
            return 0;
        }
    }
    xdbg("pledge_redeem_vote no expire vote");
    return xtransaction_pledge_redeem_vote_err;
}

data::xproperty_asset xaccount_context_t::get_source_transfer_in() {
    data::xproperty_asset asset(0);
    if (m_balance_change >= 0) {
        asset.m_amount = m_balance_change;
    }
    return asset;
}

int32_t xaccount_context_t::set_parent_account(const uint64_t amount, const std::string& value) {
    std::string _value;
    if (!m_native_property.native_string_get(XPORPERTY_PARENT_ACCOUNT_KEY, _value)) {
        return xaccount_property_parent_account_exist;
    }
    uint64_t balance = amount > XGET_CONFIG(min_account_deposit) ? (amount - XGET_CONFIG(min_account_deposit)) : 0;
    set_lock_token_sum(XGET_CONFIG(min_account_deposit));
    top_token_transfer_in(balance);
    return m_native_property.native_string_set(XPORPERTY_PARENT_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::set_sub_account(const std::string& value) {
    return m_native_property.native_deque_push_back(XPORPERTY_SUB_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::sub_account_check(const std::string& value) {
    if (m_native_property.native_deque_exist(XPORPERTY_SUB_ACCOUNT_KEY, value)) {
        return xaccount_property_sub_account_exist;
    }
    int32_t size;
    if (!m_native_property.native_deque_size(XPORPERTY_SUB_ACCOUNT_KEY, size) && size >= MAX_SUB_ACCOUNT) {
        return xaccount_property_sub_account_overflow;
    }
    return xstore_success;
}

int32_t xaccount_context_t::remove_sub_account(const std::string& value) {
    return m_native_property.native_deque_erase(XPORPERTY_SUB_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::remove_contract_sub_account(const std::string& value) {
    return m_native_property.native_deque_erase(XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY, value);
}

int32_t xaccount_context_t::set_contract_code(const std::string &code) {
    // check account type
    CHECK_UNIT_HEIGHT_SHOULD_BE_ZERO();
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t application_contract_code_max_len = XGET_ONCHAIN_GOVERNANCE_PARAMETER(application_contract_code_max_len);
    if (code.size() == 0 || code.size() > application_contract_code_max_len) {
        return xaccount_property_contract_code_size_invalid;
    }

    // contract code is a native property, but store with user property
    std::string key = data::XPROPERTY_CONTRACT_CODE;
    auto ret = m_accountcmd->string_create(key);
    if (ret != xstore_success) {
        return ret;
    }
    return m_accountcmd->string_set(key, code);
}

int32_t xaccount_context_t::get_contract_code(std::string &code) {
    // check account type
    const std::string key = data::XPROPERTY_CONTRACT_CODE;
    return m_accountcmd->string_get(key, code);
}

xstring_ptr_t xaccount_context_t::string_read_get(const std::string & prop_name, int32_t & error_code) {
    xstring_ptr_t prop;
    if (!xnative_property_name_t::is_native_property(prop_name)) {
        prop = m_accountcmd->string_get(prop_name);
    } else {
        prop = m_native_property.string_get(prop_name);
    }
    if (prop == nullptr) {
        error_code = xaccount_property_not_create;
    }
    return prop;
}

xstrdeque_ptr_t xaccount_context_t::deque_read_get(const std::string& prop_name, int32_t & error_code) {
    xstrdeque_ptr_t prop;
    if (!xnative_property_name_t::is_native_property(prop_name)) {
        prop = m_accountcmd->deque_get(prop_name);
    } else {
        prop = m_native_property.deque_get(prop_name);
    }
    if (prop == nullptr) {
        xdbg("prop_name:%s is empty", prop_name.c_str());
        error_code = xaccount_property_not_create;
    }
    return prop;
}

xstrmap_ptr_t xaccount_context_t::map_read_get(const std::string& prop_name, int32_t & error_code) {
    xstrmap_ptr_t prop;
    if (!xnative_property_name_t::is_native_property(prop_name)) {
        prop = m_accountcmd->map_get(prop_name);
    } else {
        prop = m_native_property.map_get(prop_name);
    }
    if (prop == nullptr) {
        error_code = xaccount_property_not_create;
    }
    return prop;
}

int32_t xaccount_context_t::check_create_property(const std::string& key) {
    CHECK_CREATE_NATIVE_PROPERTY(key);
    //CHECK_UNIT_HEIGHT_SHOULD_BE_ZERO();
    CHECK_PROPERTY_NAME_LEN_MAX_NUM(key);
    CHECK_PROPERTY_MAX_NUM();
    return xstore_success;
}

int32_t xaccount_context_t::string_create(const std::string& key) {
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    return m_accountcmd->string_create(key);
}
int32_t xaccount_context_t::string_set(const std::string& key, const std::string& value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->string_set(key, value);
    } else {
        return m_native_property.native_string_set(key, value);
    }
}

int32_t xaccount_context_t::string_get(const std::string& key, std::string& value, const std::string& addr) {
    if(addr.empty()) {
        xstring_ptr_t prop;
        STRING_PROP_READ_GET(prop);
        prop->get(value);
        return xsuccess;
    } else {
        return m_store->string_get(addr, key, value);
    }
}
int32_t xaccount_context_t::string_empty(const std::string& key, bool& empty) {
    xstring_ptr_t prop;
    STRING_PROP_READ_GET(prop);
    empty = prop->empty();
    return xstore_success;
}
int32_t xaccount_context_t::string_size(const std::string& key, int32_t& size) {
    xstring_ptr_t prop;
    STRING_PROP_READ_GET(prop);
    size = prop->size();
    return xstore_success;
}
int32_t xaccount_context_t::list_create(const std::string& key) {
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    return m_accountcmd->list_create(key);
}
int32_t xaccount_context_t::list_push_back(const std::string& key, const std::string& value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->list_push_back(key, value);
    } else {
        return m_native_property.native_deque_push_back(key, value);
    }
}
int32_t xaccount_context_t::list_push_front(const std::string& key, const std::string& value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->list_push_front(key, value);
    } else {
        return m_native_property.native_deque_push_front(key, value);
    }
}
int32_t xaccount_context_t::list_pop_back(const std::string& key, std::string& value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->list_pop_back(key, value);
    } else {
        return m_native_property.native_deque_pop_back(key, value);
    }
}
int32_t xaccount_context_t::list_pop_front(const std::string& key, std::string& value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->list_pop_front(key, value);
    } else {
        return m_native_property.native_deque_pop_front(key, value);
    }
}
int32_t xaccount_context_t::list_clear(const std::string &key, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->list_clear(key);
    } else {
        return m_native_property.native_deque_clear(key);
    }
}
int32_t xaccount_context_t::list_get_back(const std::string& key, std::string & value) {
    xstrdeque_ptr_t prop;
    DEQUE_PROP_READ_GET(prop);
    bool ret = prop->get_back(value);
    return (ret == true) ? xstore_success : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_get_front(const std::string& key, std::string & value) {
    xstrdeque_ptr_t prop;
    DEQUE_PROP_READ_GET(prop);
    bool ret = prop->get_front(value);
    return (ret == true) ? xstore_success : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_get(const std::string& key, const uint32_t index, std::string & value, const std::string& addr) {
    if(addr.empty()) {
        xstrdeque_ptr_t prop;
        DEQUE_PROP_READ_GET(prop);
        bool ret = prop->get(index, value);
        return (ret == true) ? xstore_success : xaccount_property_operate_fail;
    } else {
        return m_store->list_get(addr, key, index, value);
    }
}
int32_t xaccount_context_t::list_empty(const std::string& key, bool& empty) {
    xstrdeque_ptr_t prop;
    DEQUE_PROP_READ_GET(prop);
    empty = prop->empty();
    return xstore_success;
}
int32_t xaccount_context_t::list_size(const std::string& key, int32_t& size, const std::string& addr) {
    if(addr.empty()) {
        xstrdeque_ptr_t prop;
        DEQUE_PROP_READ_GET(prop);
        size = prop->size();
        return xstore_success;
    } else {
        return m_store->list_size(addr, key, size);
    }
}
int32_t xaccount_context_t::list_get_range(const std::string &key, int32_t start, int32_t stop, std::vector<std::string> &values) {
    xstrdeque_ptr_t prop;
    DEQUE_PROP_READ_GET(prop);
    int32_t size = prop->size();
    if (stop > size) {
        stop = size;
    }
    for (int32_t i=start; i < stop; i++) {
        std::string value;
        auto ret = prop->get(i, value);
        assert(ret);
        values.push_back(value);
    }
    return xstore_success;
}
int32_t xaccount_context_t::list_get_all(const std::string &key, std::vector<std::string> &values, const std::string& addr) {
    if(addr.empty()) {
        xstrdeque_ptr_t prop;
        DEQUE_PROP_READ_GET(prop);
        int32_t size = prop->size();
        for (int32_t i=0; i < size; i++) {
            std::string value;
            auto ret = prop->get(i, value);
            assert(ret);
            values.push_back(value);
        }
    } else {
        return m_store->list_get_all(addr, key, values);
    }
    return xstore_success;
}
int32_t xaccount_context_t::list_copy_get(const std::string &key, std::deque<std::string> & deque) {
    xstrdeque_ptr_t prop;
    DEQUE_PROP_READ_GET(prop);
    deque = prop->get_deque();
    return xstore_success;
}

int32_t xaccount_context_t::map_create(const std::string& key) {
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    return m_accountcmd->map_create(key);
}
int32_t xaccount_context_t::map_get(const std::string & key, const std::string & field, std::string & value, const std::string& addr) {
    if(addr.empty()) {
        xstrmap_ptr_t prop;
        MAP_PROP_READ_GET(prop);
        bool ret = prop->get(field, value);
        return (ret == true) ? xstore_success : xaccount_property_map_field_not_create;
    } else {
        return m_store->map_get(addr, key, field, value);
    }
}
int32_t xaccount_context_t::map_set(const std::string & key, const std::string & field, const std::string & value, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->map_set(key, field, value);
    } else {
        return m_native_property.native_map_set(key, field, value);
    }
}
int32_t xaccount_context_t::map_remove(const std::string & key, const std::string & field, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->map_remove(key, field);
    } else {
        return m_native_property.native_map_erase(key, field);
    }
}
int32_t xaccount_context_t::map_clear(const std::string & key, bool native) {
    CHECK_PROPERTY_NAME_NATIVE_RIGHT(key, native);
    if (!xnative_property_name_t::is_native_property(key)) {
        return m_accountcmd->map_clear(key);
    } else {
        return m_native_property.native_map_clear(key);
    }
}
int32_t xaccount_context_t::map_empty(const std::string & key, bool& empty) {
    xstrmap_ptr_t prop;
    MAP_PROP_READ_GET(prop);
    empty = prop->empty();
    return xstore_success;
}
int32_t xaccount_context_t::map_size(const std::string & key, int32_t& size, const std::string& addr) {
    if(addr.empty()) {
        xstrmap_ptr_t prop;
        MAP_PROP_READ_GET(prop);
        size = prop->size();
        return xstore_success;
    } else {
        return m_store->map_size(addr, key, size);
    }
}
int32_t xaccount_context_t::map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr) {
    if(addr.empty()) {
        xstrmap_ptr_t prop;
        MAP_PROP_READ_GET(prop);
        map = prop->get_map();
        return xstore_success;
    } else {
        return m_store->map_copy_get(addr, key, map);
    }
}

int32_t xaccount_context_t::get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr) {
    if (addr.empty()) {
        return m_store->get_map_property(get_address(), height, key, value);
    }

    return m_store->get_map_property(addr, height, key, value);
}

int32_t xaccount_context_t::map_property_exist(const std::string& key) {
    xstrmap_ptr_t prop;
    MAP_PROP_READ_GET(prop);

    return 0;
}

int32_t xaccount_context_t::do_prop_set(xproperty_op_code_t cmd, const std::string & key, const std::string & op_para1, const std::string & op_para2) {
    data::xproperty_instruction_t instruction(cmd, op_para1, op_para2);
    return m_accountcmd->do_instruction(key, instruction);
}

int32_t xaccount_context_t::do_prop_set(xproperty_op_code_t cmd, const std::string & key, const std::string & op_para1) {
    return do_prop_set(cmd, key, op_para1, {});
}

int32_t xaccount_context_t::do_prop_set(xproperty_op_code_t cmd, const std::string & key) {
    return do_prop_set(cmd, key, {}, {});
}

bool xaccount_context_t::get_transaction_result(xtransaction_result_t& result) const {
    result.m_balance_change         = m_balance_change;
    result.m_pledge_balance_change  = m_pledge_balance_change;
    result.m_lock_balance_change    = m_lock_balance_change;
    //result.m_lock_balance_change    = m_lock_balance_change;
    result.m_lock_tgas_change       = m_lock_tgas_change;
    result.m_unvote_num_change      = m_unvote_num_change;
    result.m_props                  = m_accountcmd->get_property_hash();
    result.m_contract_txs           = m_contract_txs;
    result.m_native_property        = m_native_property;
    result.m_prop_log               = m_accountcmd->get_property_log();
    return true;
}

void xaccount_context_t::set_context_para(uint64_t clock, const std::string & random_seed, uint64_t timestamp, uint64_t sys_total_lock_tgas_token) {
    m_timer_height = clock;
    m_random_seed = random_seed;
    m_timestamp = timestamp;
    xassert(!random_seed.empty());
    m_sys_total_lock_tgas_token = sys_total_lock_tgas_token;
}

void xaccount_context_t::set_context_pare_current_table(const std::string & table_addr, uint64_t table_committed_height) {
    m_current_table_addr = table_addr;
    m_current_table_commit_height = table_committed_height;
}

bool xaccount_context_t::add_transaction(const xcons_transaction_ptr_t& trans) {
    m_currect_transaction = trans;
    return true;
}

void xaccount_context_t::set_source_pay_info(const data::xaction_asset_out& source_pay_info) {
    m_source_pay_info = source_pay_info;
}

const data::xaction_asset_out& xaccount_context_t::get_source_pay_info() {
    return m_source_pay_info;
}

int32_t xaccount_context_t::create_transfer_tx(const std::string & receiver, uint64_t amount) {
    uint32_t deposit = 0;
    xassert(data::is_contract_address(common::xaccount_address_t{ m_address }));
    if (data::is_user_contract_address(common::xaccount_address_t{ m_address })) {
        deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
        xwarn("xaccount_context_t::create_transfer_tx fail to create user contract transaction from:%s,to:%s,amount:%" PRIu64,
                m_address.c_str(), receiver.c_str(), amount);
        return xstore_user_contract_can_not_initiate_transaction;
    }
    xtransaction_ptr_t tx = m_account->make_transfer_tx(receiver, amount, m_timestamp, 0, deposit);
    xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
    uint32_t contract_call_contracts_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
    if (m_contract_txs.size() >= contract_call_contracts_num) {
        xwarn("xaccount_context_t::create_transfer_tx contract transaction exceeds max from:%s,to:%s,amount:%" PRIu64,
                m_address.c_str(), receiver.c_str(), amount);
        return xaccount_contract_number_exceed_max;
    }
    m_contract_txs.push_back(constx);
    xdbg("xaccount_context_t::create_transfer_tx tx:%s,from:%s,to:%s,amount:%ld,nonce:%ld,deposit:%d",
        tx->get_digest_hex_str().c_str(), m_address.c_str(), receiver.c_str(), amount, tx->get_tx_nonce(), deposit);
    return xstore_success;
}

int32_t xaccount_context_t::generate_tx(const std::string& target_addr, const std::string& func_name, const std::string& func_param) {
    if (data::is_user_contract_address(common::xaccount_address_t{ m_address })) {
        xwarn("xaccount_context_t::generate_tx from:%s,to:%s,func_name:%s",
                m_address.c_str(), target_addr.c_str(), func_name.c_str());
        return xstore_user_contract_can_not_initiate_transaction;
    }

    xtransaction_ptr_t tx = m_account->make_run_contract_tx(target_addr, func_name, func_param, 0, m_timestamp, 0, 0);
#if 0  // TODO(jimmy) not check now
    auto const & fork_config = chain_upgrade::xchain_fork_config_center_t::chain_fork_config();
    if (chain_upgrade::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, get_timer_height())) {
        bool ret = tx->transaction_len_check();
        if (!ret) {
            return xtransaction_param_invalid;
        }
    }
#endif
    xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
    uint32_t contract_call_contracts_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
    if (m_contract_txs.size() >= contract_call_contracts_num) {
        xwarn("xaccount_context_t::generate_tx contract transaction exceeds max from:%s,to:%s,func_name:%s",
                m_address.c_str(), target_addr.c_str(), func_name.c_str());
        return xaccount_contract_number_exceed_max;
    }
    m_contract_txs.push_back(constx);
    xdbg("xaccount_context_t::generate_tx tx:%s,from:%s,to:%s,func_name:%s,nonce:%ld,lasthash:%ld,func_param:%ld",
        tx->get_digest_hex_str().c_str(), m_address.c_str(), target_addr.c_str(), func_name.c_str(), tx->get_tx_nonce(), tx->get_last_hash(), base::xhash64_t::digest(func_param));
    return xstore_success;
}

// vote to candidate
int32_t xaccount_context_t::vote_out(const std::string& addr_to,
    const std::string& lock_hash, uint64_t vote_amount, uint64_t expiration) {

    uint32_t ret = store::xaccount_property_not_exist;
    std::string value;
    xproperty_vote v;

    if (get_vote_info(lock_hash, value) == 0) {
        base::xstream_t stream(base::xcontext_t::instance(),
            (uint8_t*)value.c_str(), (uint32_t)value.size());
        v.serialize_from(stream);
        uint64_t now_clock = m_timer_height;
        if (v.m_expiration != expiration) {
            // expiration time is different
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY, value);
            ret = store::xaccount_property_not_exist;
        }
        else if (now_clock > v.m_expiration) {
            // vote is expired
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY, value);
            ret = store::xaccount_property_not_exist;
        }
        else if (v.m_available < vote_amount) {
            // available vote is not enough
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY, value);
            ret = store::xaccount_property_not_exist;
        }
        else {
            v.m_available -= vote_amount;

            base::xstream_t stream_in(base::xcontext_t::instance());
            v.serialize_to(stream_in);
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY,
                std::string((char*)stream_in.data(), stream_in.size()));

            xproperty_vote_out v_out;
            if (get_vote_out_info(addr_to, lock_hash, value) == 0) {
                base::xstream_t stream(base::xcontext_t::instance(),
                    (uint8_t*)value.c_str(), (uint32_t)value.size());
                v_out.serialize_from(stream);
                v_out.m_amount += vote_amount;
            }
            else {
                v_out.m_address = addr_to;
                v_out.m_lock_hash = v.m_lock_hash;
                v_out.m_amount = vote_amount;
                v_out.m_expiration = v.m_expiration;
            }

            base::xstream_t stream_out(base::xcontext_t::instance());
            v_out.serialize_to(stream_out);
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY,
                std::string((char*)stream_out.data(), stream_out.size()));
            ret = store::xstore_success;
        }
    }
    return ret;
}

// get vote from candiate
int32_t xaccount_context_t::vote_in(const std::string& addr_to, const std::string& lock_hash, uint64_t amount) {
    int32_t ret = store::xaccount_property_not_exist;
    std::string value;
    xproperty_vote_out v_out;

    if (get_vote_out_info(addr_to, lock_hash, value) == 0) {
        base::xstream_t stream(base::xcontext_t::instance(),
            (uint8_t*)value.c_str(), (uint32_t)value.size());
        v_out.serialize_from(stream);
        if (amount > v_out.m_amount) {
            ret = -1; // vote amount is not enough
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, value);
        }
        else {
            v_out.m_amount -= amount;

            std::string vote_value;
            xproperty_vote v;

            if (get_vote_info(lock_hash, vote_value) == 0) {
                base::xstream_t stream(base::xcontext_t::instance(),
                    (uint8_t*)vote_value.c_str(), (uint32_t)vote_value.size());
                v.serialize_from(stream);

                v.m_available += amount;

                base::xstream_t stream_in(base::xcontext_t::instance());
                v.serialize_to(stream_in);

                m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY,
                    std::string((char*)stream_in.data(), stream_in.size()));
                ret = store::xstore_success;
            }
        }

        if (v_out.m_amount != 0) {
            base::xstream_t stream_out(base::xcontext_t::instance());
            v_out.serialize_to(stream_out);

            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY,
                std::string((char*)stream_out.data(), stream_out.size()));
        }
    }
    return ret;
}

int32_t xaccount_context_t::get_vote_info(const std::string& lock_hash, std::string& value) {

    int32_t ret = store::xaccount_property_not_exist;
    int32_t size{ 0 };
    xproperty_vote v;
    std::string value_str;
    if (m_native_property.native_deque_size(data::XPROPERTY_CONTRACT_VOTE_KEY, size) == xsuccess) {
        for (int32_t i = 0; i < size; ++i) {
            m_native_property.native_deque_pop_front(data::XPROPERTY_CONTRACT_VOTE_KEY, value_str);
            base::xstream_t stream(base::xcontext_t::instance(),
                (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            v.serialize_from(stream);
            if (lock_hash == v.m_lock_hash) {
                value = std::move(value_str);
                ret = 0; // find
                break;
            }
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_KEY, value_str);
        }
    }
    return ret;
}

int32_t xaccount_context_t::get_vote_out_info(const std::string& addr_to,
    const std::string& lock_hash, std::string& value) {

    int32_t ret = store::xaccount_property_not_exist;
    int32_t size{ 0 };
    xproperty_vote_out v_out;
    std::string value_str;
    if (m_native_property.native_deque_size(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, size) == xsuccess) {
        for (int32_t i = 0; i < size; ++i) {
            m_native_property.native_deque_pop_front(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, value_str);
            base::xstream_t stream(base::xcontext_t::instance(),
                (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            v_out.serialize_from(stream);
            if (v_out.m_address == addr_to &&
                v_out.m_lock_hash == lock_hash) {
                value = std::move(value_str);
                ret = 0; // find
                break;
            }
            m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, value_str);
        }
    }
    return ret;
}

int32_t xaccount_context_t::clear_vote_out_info(const std::string& lock_hash) {

    int32_t ret = store::xaccount_property_not_exist;
    int32_t size{ 0 };
    xproperty_vote_out v_out;
    std::string value_str;
    if (m_native_property.native_deque_size(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, size) == xsuccess) {
        for (int32_t i = 0; i < size; ++i) {
            m_native_property.native_deque_pop_front(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, value_str);
            base::xstream_t stream(base::xcontext_t::instance(),
                (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            v_out.serialize_from(stream);
            if (v_out.m_lock_hash == lock_hash) {
                ret = 0; // find
            }
            else {
                m_native_property.native_deque_push_back(data::XPROPERTY_CONTRACT_VOTE_OUT_KEY, value_str);
            }
        }
    }

    return ret;
}

data::xblock_t*
xaccount_context_t::get_block_by_height(const std::string & owner, uint64_t height) const {
    // TODO(jimmy)
    base::xvaccount_t _vaddr(owner);
    base::xauto_ptr<base::xvblock_t> _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddr, height, base::enum_xvblock_flag_committed, true);
    if (_block != nullptr) {
        _block->add_ref();
        return dynamic_cast<data::xblock_t*>(_block.get());
    }
    return nullptr;
}

uint64_t
xaccount_context_t::get_blockchain_height(const std::string & owner) const {
    uint64_t height;
    if (owner == m_address) {
        height = m_account->get_chain_height();
    } else if (owner == m_current_table_addr) {
        height = m_current_table_commit_height;
    } else {
        height = m_store->get_blockchain_height(owner);
    }
    xdbg("xaccount_context_t::get_blockchain_height owner=%s,height=%" PRIu64 "", owner.c_str(), height);
    return height;
}

}  // namespace store
}  // namespace top
