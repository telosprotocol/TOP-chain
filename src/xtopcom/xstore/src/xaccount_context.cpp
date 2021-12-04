// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <cinttypes>

#include "xstore/xstore_error.h"
#include "xstore/xaccount_context.h"

#include "xbase/xlog.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvpropertyrules.h"
#include "xcrypto/xckey.h"
#include "xdata/xaction_parse.h"
#include "xdata/xproperty.h"
#include "xdata/xdata_defines.h"
#include "xdata/xchain_param.h"
#include "xdata/xtransaction_maker.hpp"
#include "xbasic/xutility.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtx_factory.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xutility/xstream_util.hpp"
#include "xvm/manager/xcontract_address_map.h"
#include "xstake/xstake_algorithm.h"
#include "xchain_fork/xchain_upgrade_center.h"

using namespace top::base;

namespace top { namespace store {

#define CHECK_BSTATE_NULL_RETURN(bstate, funcname) \
do {\
    if (nullptr == bstate) {\
        std::string str_funcname = std::string(funcname);\
        xwarn("%s,fail-load bstate.address=%s", str_funcname.c_str(), get_address().c_str());\
        return xaccount_account_not_exist;\
    }\
}while(0)
#define CHECK_PROPERTY_NULL_RETURN(propobj, funcname, propname) \
do {\
    if (nullptr == bstate) {\
        std::string str_funcname = std::string(funcname);\
        xwarn("%s,fail-property not exist.address=%s,propname=%s", str_funcname.c_str(), get_address().c_str(), propname.c_str());\
        return xaccount_property_not_create;\
    }\
}while(0)

#define CHECK_FIND_PROPERTY(bstate, propname) \
do {\
    if (false == bstate->find_property(propname)) {\
        xwarn("xaccount_context_t,fail-find property.address=%s,propname=%s", get_address().c_str(), propname.c_str());\
        return xaccount_property_not_create;\
    }\
}while(0)

xaccount_context_t::xaccount_context_t(const xaccount_ptr_t & unitstate) {
    m_account = unitstate;

    m_latest_exec_sendtx_nonce = m_account->get_latest_send_trans_number();
    m_latest_exec_sendtx_hash = m_account->account_send_trans_hash();
    m_latest_create_sendtx_nonce = m_latest_exec_sendtx_nonce;
    m_latest_create_sendtx_hash = m_latest_exec_sendtx_hash;
    m_canvas = make_object_ptr<base::xvcanvas_t>();
    xinfo("create context, address:%s,height:%ld,uri=%s",
        unitstate->get_account().c_str(), unitstate->get_block_height(), m_account->get_bstate()->get_execute_uri().c_str());
}

// TODO(jimmy) this constructor api will be deleted later
xaccount_context_t::xaccount_context_t(const xaccount_ptr_t & unitstate, xstore_face_t* store) {
    m_account = unitstate;

    m_latest_exec_sendtx_nonce = m_account->get_latest_send_trans_number();
    m_latest_exec_sendtx_hash = m_account->account_send_trans_hash();
    m_latest_create_sendtx_nonce = m_latest_exec_sendtx_nonce;
    m_latest_create_sendtx_hash = m_latest_exec_sendtx_hash;
    m_canvas = make_object_ptr<base::xvcanvas_t>();
    xinfo("create context, address:%s,height:%ld,uri=%s",
        unitstate->get_account().c_str(), unitstate->get_block_height(), m_account->get_bstate()->get_execute_uri().c_str());
}

xaccount_context_t::~xaccount_context_t() {
}

int32_t xaccount_context_t::create_user_account(const std::string& address) {
    assert(address == get_address());
    xinfo("xaccount_context_t::create_user_account address:%s", address.c_str());

    auto old_token = token_balance(XPROPERTY_BALANCE_AVAILABLE);
    if (old_token != 0) {
        xerror("xaccount_context_t::create_user_account fail-token not zero");
        return -1;
    }

    // just for test debug
    base::vtoken_t add_token = (base::vtoken_t)(ASSET_TOP(100000000));
    return token_deposit(XPROPERTY_BALANCE_AVAILABLE, add_token);
}

int32_t xaccount_context_t::token_transfer_out(const data::xproperty_asset& asset, uint64_t gas_fee, uint64_t service_fee) {
    if (asset.is_top_token()) {
        return top_token_transfer_out(asset.m_amount, gas_fee, service_fee);
    } else {
        xassert(false);
        return -1;
    }
}

int32_t xaccount_context_t::top_token_transfer_out(uint64_t amount, uint64_t gas_fee, uint64_t service_fee) {
    if (amount == 0 && gas_fee == 0 && service_fee == 0) {
        xerror("xaccount_context_t::top_token_transfer_out fail-invalid para.amount=%ld,gas_fee=%ld,service_fee=%ld", amount, gas_fee, service_fee);
        return -1;
    }

    if(get_address() == sys_contract_zec_reward_addr) {
        return xstore_success;
    }
    int32_t ret = xsuccess;
    if (amount > 0) {
        ret = token_withdraw(XPROPERTY_BALANCE_AVAILABLE, (base::vtoken_t)amount);
        if (ret != xsuccess) {
            return ret;
        }
    }
    if (gas_fee > 0) {
        ret = token_withdraw(XPROPERTY_BALANCE_AVAILABLE, (base::vtoken_t)gas_fee);
        if (ret != xsuccess) {
            return ret;
        }
    }
    if (service_fee > 0) {
        ret = token_withdraw(XPROPERTY_BALANCE_AVAILABLE, (base::vtoken_t)service_fee);
        if (ret != xsuccess) {
            return ret;
        }
    }
    return xstore_success;
}

int32_t xaccount_context_t::token_transfer_in(const data::xproperty_asset& asset) {
    if (asset.is_top_token()) {
        return top_token_transfer_in(asset.m_amount);
    } else {
        xassert(false);
        return -1;
    }
}

int32_t xaccount_context_t::top_token_transfer_in(uint64_t amount) {
    xdbg("xaccount_context_t::top_token_transfer_in amount=%ld", amount);
    if (amount == 0) {
        return xstore_success;
    }
    return token_deposit(XPROPERTY_BALANCE_AVAILABLE, (base::vtoken_t)amount);
}

// how many tgas you can get from pledging 1TOP
uint32_t xaccount_context_t::get_token_price() const {
    return m_account->get_token_price(m_sys_total_lock_tgas_token);
}

uint64_t xaccount_context_t::get_used_tgas(){
    int32_t ret = 0;
    std::string v;
    ret = string_get(XPROPERTY_USED_TGAS_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_used_tgas(uint64_t num){
    string_set(XPROPERTY_USED_TGAS_KEY, std::to_string(num));
    string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(m_timer_height));
    return 0;
}
// set actual used tgas
int32_t xaccount_context_t::incr_used_tgas(uint64_t num){
    auto used_tgas = calc_decayed_tgas();
    string_set(XPROPERTY_USED_TGAS_KEY, std::to_string(num + used_tgas));
    string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(m_timer_height));
    return 0;
}
// get actual used tgas
uint64_t xaccount_context_t::calc_decayed_tgas(){
    uint32_t last_hour = get_last_tx_hour();
    uint64_t used_tgas{0};
    uint32_t decay_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
    if(m_timer_height - last_hour < decay_time){
        used_tgas = (decay_time - (m_timer_height - last_hour)) * get_used_tgas() / decay_time;
    }
    return used_tgas;
}

int32_t xaccount_context_t::check_used_tgas(uint64_t &cur_tgas_usage, uint64_t deposit, uint64_t& deposit_usage){
    uint32_t last_hour = get_last_tx_hour();
    xdbg("tgas_disk last_hour: %d, m_timer_height: %d, no decay used_tgas: %d, used_tgas: %d, pledge_token: %d, token_price: %u, total_tgas: %d, tgas_usage: %d, deposit: %d",
          last_hour, m_timer_height, get_used_tgas(), calc_decayed_tgas(), m_account->tgas_balance(), get_token_price(), get_total_tgas(), cur_tgas_usage, deposit);

    auto available_tgas = get_available_tgas();
    xdbg("tgas_disk account: %s, total tgas usage adding this tx : %d", get_address().c_str(), cur_tgas_usage);
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

int32_t xaccount_context_t::update_tgas_sender(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage) {
    auto ret = check_used_tgas(tgas_usage, deposit, deposit_usage);
    if(ret == 0) {
        incr_used_tgas(tgas_usage);
    }
    available_balance_to_other_balance(XPROPERTY_BALANCE_BURN, base::vtoken_t(deposit_usage));
    xdbg("xaccount_context_t::update_tgas_sender tgas_usage: %llu, deposit: %u, deposit_usage: %llu", tgas_usage, deposit, deposit_usage);
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

    if (used_deposit > 0) {
        xdbg("xaccount_context_t::calc_resource balance withdraw used_deposit=%u", used_deposit);
        ret = available_balance_to_other_balance(XPROPERTY_BALANCE_BURN, base::vtoken_t(used_deposit));
    }
    return ret;
}

uint64_t xaccount_context_t::get_last_tx_hour(){
    int32_t ret = 0;
    std::string v;
    ret = string_get(XPROPERTY_LAST_TX_HOUR_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

int32_t xaccount_context_t::set_last_tx_hour(uint64_t num){
    string_set(XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(num));
    return 0;
}

int32_t xaccount_context_t::update_disk(uint64_t disk_usage){
    // disk is 0 at present
    return 0;
}

int32_t xaccount_context_t::lock_token(const uint256_t &tran_hash, uint64_t amount, const std::string &tran_params) {
    xdbg_info("xaccount_context_t::lock_token enter.amount=%ld", amount);

    int32_t ret = xsuccess;
    // firstly, withdraw amount from available balance
    ret = available_balance_to_other_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(amount));
    if (xsuccess != ret) {
        return ret;
    }
    // thirdly, save lock token record by txhash  TODO(jimmy) should limit record number
    std::string v;
    std::string hash_str((char *)tran_hash.data(), tran_hash.size());
    ret = map_get(XPROPERTY_LOCK_TOKEN_KEY, hash_str, v);
    if (xsuccess == ret) {
        xerror("xaccount_context_t::lock_token failed, same tx hash %s", hash_str.c_str());
        return xaccount_property_lock_token_key_same;
    }
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_timer_height;
    stream << tran_params;
    std::string record_str = std::string((char *)stream.data(), stream.size());
    ret = map_set(XPROPERTY_LOCK_TOKEN_KEY, hash_str, record_str);
    if (xsuccess != ret) {
        xerror("xaccount_context_t::lock_token set failed tx hash %s", hash_str.c_str());
        return xaccount_property_lock_token_key_same;
    }

    return xstore_success;
}

int32_t xaccount_context_t::unlock_token(const uint256_t &tran_hash, const std::string &lock_hash_str, const std::vector<std::string> trans_signs) {
    // 1. find lock token record by txhash and remove this record
    std::string v;
    int32_t ret = map_get(XPROPERTY_LOCK_TOKEN_KEY, lock_hash_str, v);
    if (xsuccess != ret) {
        xerror("xaccount_context_t::unlock_token fail-find lock record, tx_hash=%s", base::xstring_utl::to_hex(lock_hash_str).c_str());
        return xaccount_property_unlock_token_key_not_exist;
    }
    // 2. check if unlock condition is met
    base::xstream_t stream(base::xcontext_t::instance(),
                           (uint8_t *)v.c_str(),
                           (uint32_t)v.size());
    uint64_t clock_timer;
    std::string raw_input;
    stream >> clock_timer;
    stream >> raw_input;

    uint64_t now_clock = m_timer_height;
    data::xaction_lock_account_token lock_action;
    ret = lock_action.parse_param(raw_input);
    assert(0 == ret);
    if (lock_action.m_unlock_type == xaction_lock_account_token::UT_time) {
        uint64_t dure = xstring_utl::touint64(lock_action.m_unlock_values.at(0));
        if ( (now_clock < clock_timer) || ((now_clock - clock_timer) * 10 < dure) ) {
            xwarn("xaccount_context_t::unlock_token %d, but time not reach, lock clock %llu, now clock %llu", lock_action.m_amount, clock_timer, now_clock);
            return xaccount_property_unlock_token_time_not_reach;
        }
    }
    else {
        xerror("xaccount_context_t::unlock_token fail-not support type.type=%d", lock_action.m_unlock_type);
        return xaccount_property_unlock_token_time_not_reach;
    }

    // 3. remove token lock record
    ret = map_remove(XPROPERTY_LOCK_TOKEN_KEY, lock_hash_str);
    if (xsuccess != ret) {
        xerror("xaccount_context_t::unlock_token tx erase failed, tx_hash %s not exist", base::xstring_utl::to_hex(lock_hash_str).c_str());
        return xaccount_property_unlock_token_key_not_exist;
    }

    // 4. withdraw lock balance and deposit available balance
    ret = other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(lock_action.m_amount));
    if (xsuccess != ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xaccount_context_t::unlock_all_token(){
    std::string v;

    std::map<std::string, std::string> lock_txs;
    map_copy_get(XPROPERTY_LOCK_TOKEN_KEY, lock_txs);
    for(auto tx : lock_txs){
        int32_t ret = map_get(XPROPERTY_LOCK_TOKEN_KEY, tx.first, v);
        xdbg("xaccount_context_t::unlock_all_token, first: %s, second: %s, ret=%d", base::xstring_utl::to_hex(tx.first).c_str(), tx.second.c_str(), ret);

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
            xassert(false);  // not support
            continue;
        }

        xinfo("xaccount_context_t::unlock_all_token account: %s, amount: %d, unlock amount: %d, lock clock %llu, now clock %llu, tx hash: %s",
               get_address().c_str(), lock_action.m_amount, lock_action.m_amount, clock_timer, now_clock, base::xstring_utl::to_hex(tx.first).c_str());

        ret = map_remove(XPROPERTY_LOCK_TOKEN_KEY, tx.first);
        if (0 != ret) {
            xerror("xaccount_context_t::unlock_all_token tx erase failed, tx_hash %s not exist", base::xstring_utl::to_hex(tx.first).c_str());
            return xaccount_property_unlock_token_key_not_exist;
        }

        ret = other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(lock_action.m_amount));
        if (xsuccess != ret) {
            return ret;
        }
    }

    return xstore_success;
}

void xaccount_context_t::deserilize_vote_map_field(const std::string& str, uint16_t& duration, uint64_t& lock_time){
    base::xstream_t stream{xcontext_t::instance(), (uint8_t*)str.data(), static_cast<uint32_t>(str.size())};
    stream >> duration;
    stream >> lock_time;
}
void xaccount_context_t::deserilize_vote_map_value(const std::string& str, uint64_t& vote_num){
    base::xstream_t stream{xcontext_t::instance(), (uint8_t*)str.data(), static_cast<uint32_t>(str.size())};
    stream >> vote_num;
}
std::string xaccount_context_t::serilize_vote_map_field(uint16_t duration, uint64_t lock_time){
    base::xstream_t stream{xcontext_t::instance()};
    stream << duration;
    stream << lock_time;
    return std::string((char*)stream.data(), stream.size());
}
std::string xaccount_context_t::serilize_vote_map_value(uint64_t vote_num){
    base::xstream_t stream{xcontext_t::instance()};
    stream << vote_num;
    return std::string((char*)stream.data(), stream.size());
}

// only merge expire lock record
int32_t xaccount_context_t::merge_pledge_vote_property(){
    std::map<std::string, std::string> pledge_votes;
    map_copy_get(XPROPERTY_PLEDGE_VOTE_KEY, pledge_votes);
    if (pledge_votes.empty()) {
        // do nothing
        return 0;
    }

    uint64_t vote_sum{0};
    uint64_t expire_token{0};
    for (auto & v : pledge_votes) {
        uint64_t vote_num{0};
        uint16_t duration{0};
        uint64_t lock_time{0};
        deserilize_vote_map_field(v.first, duration, lock_time);
        deserilize_vote_map_value(v.second, vote_num);

#ifdef DEBUG
        if(m_timer_height - lock_time >= duration / 60){
#else
        if(m_timer_height - lock_time >= duration * 24 * 60 * 6){
#endif
            map_remove(XPROPERTY_PLEDGE_VOTE_KEY, v.first);
            vote_sum += vote_num;
            if(0 != duration){ // if not calculated in XPROPERTY_EXPIRE_VOTE_TOKEN_KEY
                expire_token += get_top_by_vote(vote_num, duration);
                xdbg("xaccount_context_t::merge_pledge_vote_property expire. vote_num=%d,duration=%d,lock_time=%d,clock=%ld,vote_sum=%ld", vote_num, duration, lock_time, m_timer_height, vote_sum);
            }
        }
    }

    if(vote_sum > 0){
        map_set(XPROPERTY_PLEDGE_VOTE_KEY, serilize_vote_map_field(0, 0), serilize_vote_map_value(vote_sum));
    }
    if(expire_token > 0){
        std::string val;
        string_get(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, val);
        expire_token += xstring_utl::touint64(val);
        xdbg("xaccount_context_t::merge_pledge_vote_property expire old: %llu, new: %llu", xstring_utl::touint64(val), expire_token);
        string_set(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, xstring_utl::tostring(expire_token));
    }
    return 0;
}

int32_t xaccount_context_t::insert_pledge_vote_property(xaction_pledge_token_vote& action){
    std::map<std::string, std::string> pledge_votes;
    map_copy_get(XPROPERTY_PLEDGE_VOTE_KEY, pledge_votes);
    const size_t max_property_deque_size{128};
    if (pledge_votes.size() >= max_property_deque_size) {
        xwarn("xaccount_context_t::insert_pledge_vote_property XPROPERTY_PLEDGE_VOTE_KEY size overflow %zu", pledge_votes.size());
        return xtransaction_pledge_redeem_vote_err;
    }

    for (auto & v : pledge_votes) {
        uint64_t vote_num{0};
        uint16_t duration{0};
        uint64_t lock_time{0};
        deserilize_vote_map_field(v.first, duration, lock_time);
        deserilize_vote_map_value(v.second, vote_num);
#ifdef DEBUG
        if(action.m_lock_duration == duration && (m_timer_height - lock_time) < 10){
#else
        // merge vote with same duration in 24 hours
        if(action.m_lock_duration == duration && (m_timer_height - lock_time) < 24 * 60 * 6){
#endif
            xdbg("xaccount_context_t::insert_pledge_vote_property merge old record.clock=%ld,vote_num=%ld,duration=%d,lock_time=%ld,action.m_vote_num=%ld",
                    m_timer_height, vote_num, duration, lock_time, action.m_vote_num);
            action.m_vote_num += vote_num;
            map_set(XPROPERTY_PLEDGE_VOTE_KEY, v.first, serilize_vote_map_value(action.m_vote_num));
            return xsuccess;
        }
    }
    xdbg("xaccount_context_t::insert_pledge_vote_property add new record.vote_num=%ld,duration=%d,lock_time=%ld",
            action.m_vote_num, action.m_lock_duration, m_timer_height);
    map_set(XPROPERTY_PLEDGE_VOTE_KEY, serilize_vote_map_field(action.m_lock_duration, m_timer_height), serilize_vote_map_value(action.m_vote_num));
    return xsuccess;
}

int32_t xaccount_context_t::update_pledge_vote_property(xaction_pledge_token_vote& action){
    merge_pledge_vote_property();
    return insert_pledge_vote_property(action);
}

int32_t xaccount_context_t::redeem_pledge_vote_property(uint64_t num){
    if (num == 0) {
        return 0;
    }

    int32_t ret = xsuccess;

    std::map<std::string, std::string> pledge_votes;
    map_copy_get(XPROPERTY_PLEDGE_VOTE_KEY, pledge_votes);

    for (auto & v : pledge_votes) {
        uint64_t vote_num{0};
        uint16_t duration{0};
        uint64_t lock_time{0};
        deserilize_vote_map_field(v.first, duration, lock_time);
        deserilize_vote_map_value(v.second, vote_num);

        if(0 == duration){
            if(num > vote_num){
                xwarn("xaccount_context_t::redeem_pledge_vote_property, redeem_num=%llu, expire_num: %llu, duration: %u, lock_time: %llu", num, vote_num, duration, lock_time);
                return xtransaction_pledge_redeem_vote_err;
            }

            // calc balance change
            std::string val;
            string_get(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, val);
            uint64_t expire_token = xstring_utl::touint64(val);
            int64_t balance_change = num * expire_token / vote_num;  // TODO(jimmy) overflow fault
            string_set(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, xstring_utl::tostring(expire_token - balance_change));
            vote_num -= num;

            // update field
            map_set(XPROPERTY_PLEDGE_VOTE_KEY, serilize_vote_map_field(0, 0), serilize_vote_map_value(vote_num));

            // update unvote num, balance, vote pledge balance
            ret = uint64_sub(XPROPERTY_UNVOTE_NUM, num);
            if (xsuccess != ret) {
                return ret;
            }
            ret = other_balance_to_available_balance(XPROPERTY_BALANCE_PLEDGE_VOTE, base::vtoken_t(balance_change));
            if (xsuccess != ret) {
                return ret;
            }
            return 0;
        }
    }
    xwarn("pledge_redeem_vote no expire vote");
    return xtransaction_pledge_redeem_vote_err;
}

// calculate the top num needed to get specific votes
uint64_t xaccount_context_t::get_top_by_vote(uint64_t vote_num, uint16_t duration) {
    auto factor = MAX_TOP_VOTE_RATE;
    if (duration < MAX_VOTE_LOCK_DAYS) {
        uint64_t af = AMPLIFY_FACTOR;
        auto power = duration / MIN_VOTE_LOCK_DAYS - 1;
        for (auto i = 0; i < power; ++i) {
            af *= EXP_BASE;
            af /= AMPLIFY_FACTOR;
        }
        factor = af;
    }
    auto original_num = (top::xstake::uint128_t)vote_num * TOP_UNIT * AMPLIFY_FACTOR / factor;
    uint64_t top_num = 0;
    if (original_num > UINT64_MAX) {
        top_num = UINT64_MAX;
    } else {
        top_num = original_num;
    }
    xdbg("get_top_by_vote factor: %llu, top_num: %llu, vote_num: %llu, duration: %d", factor, top_num, vote_num, duration);
    return top_num;
}

int32_t xaccount_context_t::other_balance_to_available_balance(const std::string & property_name, base::vtoken_t token) {
    int32_t ret;
    ret = token_withdraw(property_name, token);
    if (xsuccess != ret) {
        xwarn("xaccount_context_t::other_balance_to_available_balance fail-withdraw balance, amount=%ld", token);
        return ret;
    }
    ret = token_deposit(XPROPERTY_BALANCE_AVAILABLE, token);
    if (xsuccess != ret) {
        xwarn("xaccount_context_t::other_balance_to_available_balance fail-deposit balance, amount=%ld", token);
        return ret;
    }
    xdbg("xaccount_context_t::other_balance_to_available_balance property=%s,amount=%ld", property_name.c_str(), token);
    return xsuccess;
}
int32_t xaccount_context_t::available_balance_to_other_balance(const std::string & property_name, base::vtoken_t token) {
    int32_t ret;
    ret = token_withdraw(XPROPERTY_BALANCE_AVAILABLE, token);
    if (xsuccess != ret) {
        xwarn("xaccount_context_t::available_balance_to_other_balance fail-withdraw balance, amount=%ld", token);
        return ret;
    }
    ret = token_deposit(property_name, token);
    if (xsuccess != ret) {
        xwarn("xaccount_context_t::available_balance_to_other_balance fail-deposit balance, amount=%ld", token);
        return ret;
    }
    xdbg("xaccount_context_t::available_balance_to_other_balance property=%s,amount=%ld", property_name.c_str(), token);
    return xsuccess;
}



// ================== tx related info apis ======================
void xaccount_context_t::set_tx_info_unconfirm_tx_num(uint32_t num) {
    std::string value = base::xstring_utl::tostring(num);
    int32_t ret = map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, value);
    xassert(ret == xsuccess);
}
void xaccount_context_t::set_tx_info_latest_sendtx_num(uint64_t num) {
    std::string value = base::xstring_utl::tostring(num);
    int32_t ret = map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, value);
    xassert(ret == xsuccess);
}
void xaccount_context_t::set_tx_info_latest_sendtx_hash(const std::string & hash) {
    int32_t ret = map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, hash);
    xassert(ret == xsuccess);
}
void xaccount_context_t::set_tx_info_recvtx_num(uint64_t num) {
    std::string value = base::xstring_utl::tostring(num);
    int32_t ret = map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_RECVTX_NUM, value);
    xassert(ret == xsuccess);
}

// ================== basic operation apis ======================

const xobject_ptr_t<base::xvbstate_t> & xaccount_context_t::get_bstate() const {
    return m_account->get_bstate();
}

xobject_ptr_t<base::xvbstate_t> xaccount_context_t::load_bstate(const std::string & other_addr) {
    if (other_addr.empty()) {
        return m_account->get_bstate();
    }

    // if not assign height, then get latest connect block and state
    base::xvaccount_t _vaddr(other_addr);
    base::xauto_ptr<base::xvbstate_t> _bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_latest_connectted_block_state(_vaddr, metrics::statestore_access_from_store_bstate);
    if (_bstate == nullptr) {
        xerror("xaccount_context_t::load_bstate,fail-get latest connectted state.account=%s", other_addr.c_str());
        return nullptr;
    }
    xdbg("xaccount_context_t::load_bstate,succ-get latest connectted state.account=%s,height=%ld", other_addr.c_str(), _bstate->get_block_height());
    return _bstate;
}
xobject_ptr_t<base::xvbstate_t> xaccount_context_t::load_bstate(const std::string& other_addr, uint64_t height) {
    std::string query_addr = other_addr.empty() ? get_address() : other_addr;
    base::xvaccount_t _vaddr(query_addr);
    base::xauto_ptr<base::xvbstate_t> _bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_committed_block_state(_vaddr, height, metrics::statestore_access_from_store_bstate);
    if (_bstate == nullptr) {
        xwarn("xaccount_context_t::load_bstate,fail-get target state fail.account=%s,height=%ld", query_addr.c_str(), height);
        return nullptr;
    }
    xdbg("xaccount_context_t::load_bstate,succ-get latest committed state.account=%s,height=%ld", query_addr.c_str(),height);
    return _bstate;
}

base::xauto_ptr<base::xstringvar_t> xaccount_context_t::load_string_for_write(base::xvbstate_t* bstate, const std::string & key) {
    if (false == bstate->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return bstate->new_string_var(key, m_canvas.get());
        }
    }
    auto propobj = bstate->load_string_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xdequevar_t<std::string>> xaccount_context_t::load_deque_for_write(base::xvbstate_t* bstate, const std::string & key) {
    if (false == bstate->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return bstate->new_string_deque_var(key, m_canvas.get());
        }
    }
    auto propobj = bstate->load_string_deque_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xmapvar_t<std::string>> xaccount_context_t::load_map_for_write(base::xvbstate_t* bstate, const std::string & key) {
    if (false == bstate->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return bstate->new_string_map_var(key, m_canvas.get());
        }
    }
    auto propobj = bstate->load_string_map_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    return nullptr;
}
base::xauto_ptr<base::xtokenvar_t> xaccount_context_t::load_token_for_write(base::xvbstate_t* bstate, const std::string & key) {
    if (false == bstate->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return bstate->new_token_var(key, m_canvas.get());
        }
    }
    auto propobj = bstate->load_token_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    xassert(false);
    return nullptr;
}
base::xauto_ptr<base::xvintvar_t<uint64_t>> xaccount_context_t::load_uin64_for_write(base::xvbstate_t* bstate, const std::string & key) {
    if (false == bstate->find_property(key)) {
        if (base::xvpropertyrules_t::is_valid_native_property(key)) {
            return bstate->new_uint64_var(key, m_canvas.get());
        }
    }
    auto propobj = bstate->load_uint64_var(key);
    if (nullptr != propobj) {
        return propobj;
    }
    xassert(false);
    return nullptr;
}

int32_t xaccount_context_t::check_create_property(const std::string& key) {
    if (false == base::xvpropertyrules_t::is_valid_sys_contract_property(key)) {
        xerror("xaccount_context_t::check_create_property,property name not valid.key=%s", key.c_str());
        return xaccount_property_create_fail;
    }
    if (get_bstate()->find_property(key)) {
        xerror("xaccount_context_t::check_create_property fail-already exist.propname=%s", key.c_str());
        return xaccount_property_create_fail;
    }
    return xstore_success;
}

int32_t xaccount_context_t::string_create(const std::string& key) {
    xdbg("xaccount_context_t::string_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto & bstate = get_bstate();
    auto propobj = bstate->new_string_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::string_create", key);

    return xsuccess;
}
int32_t xaccount_context_t::string_set(const std::string& key, const std::string& value) {
    xdbg("xaccount_context_t::string_set,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = load_string_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::string_set", key);
    return propobj->reset(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}

int32_t xaccount_context_t::string_get(const std::string& key, std::string& value, const std::string& addr) {
    xobject_ptr_t<base::xvbstate_t> bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::string_get");

    CHECK_FIND_PROPERTY(bstate, key);

    auto propobj = bstate->load_string_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::string_get", key);

    value = propobj->query();
    return xsuccess;
}
int32_t xaccount_context_t::list_create(const std::string& key) {
    xdbg("xaccount_context_t::list_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto & bstate = get_bstate();
    auto propobj = bstate->new_string_deque_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_create", key);
    return xsuccess;
}
int32_t xaccount_context_t::list_push_back(const std::string& key, const std::string& value) {
    xdbg("xaccount_context_t::list_push_back,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = load_deque_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_push_back", key);
    return propobj->push_back(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_push_front(const std::string& key, const std::string& value) {
    xdbg("xaccount_context_t::list_push_front,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = load_deque_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_push_front", key);
    return propobj->push_front(value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_pop_back(const std::string& key, std::string& value) {
    xdbg("xaccount_context_t::list_pop_back,property_modify_enter.address=%s,height=%ld,propname=%s",
        get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_pop_back", key);

    if (propobj->query().size() == 0) {
        xwarn("xaccount_context_t::list_pop_back fail-property is empty.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query_back();
    return propobj->pop_back(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_pop_front(const std::string& key, std::string& value) {
    xdbg("xaccount_context_t::list_pop_front,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_pop_front", key);
    if (propobj->query().size() == 0) {
        xwarn("xaccount_context_t::list_pop_front fail-property is empty.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query_front();
    return propobj->pop_front(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::list_clear(const std::string &key) {
    xdbg("xaccount_context_t::list_clear,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_clear", key);
    propobj->clear(m_canvas.get());
    return xsuccess;
}

int32_t xaccount_context_t::list_get(const std::string& key, const uint32_t pos, std::string & value, const std::string& addr) {
    auto bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::list_get");

    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_get", key);

    if (pos >= propobj->query().size()) {
        xwarn("xaccount_context_t::list_get fail-query pos invalid.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_operate_fail;
    }
    value = propobj->query(pos);
    return xsuccess;
}

int32_t xaccount_context_t::list_size(const std::string& key, int32_t& size, const std::string& addr) {
    auto bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::list_size");

    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_size", key);

    size = (int32_t)(propobj->query().size());
    return xstore_success;
}

int32_t xaccount_context_t::list_get_all(const std::string &key, std::vector<std::string> &values, const std::string& addr) {
    // TODO(jimmy) not support std::vector query now
    xerror("xaccount_context_t::list_get_all not support");
    return -1;

    // auto bstate = load_bstate(addr);
    // CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::list_get_all");

    // auto propobj = bstate->load_string_deque_var(key);
    // CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_get_all", key);

    // values = propobj->query();
    // return xstore_success;
}
int32_t xaccount_context_t::list_copy_get(const std::string &key, std::deque<std::string> & deque) {
    auto & bstate = get_bstate();
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_deque_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::list_copy_get", key);
    deque = propobj->query();
    return xstore_success;
}

int32_t xaccount_context_t::map_create(const std::string& key) {
    xdbg("xaccount_context_t::map_create,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto ret = check_create_property(key);
    if (ret) {
        return ret;
    }
    auto & bstate = get_bstate();
    auto propobj = bstate->new_string_map_var(key, m_canvas.get());
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_create", key);
    return xsuccess;
}
int32_t xaccount_context_t::map_get(const std::string & key, const std::string & field, std::string & value, const std::string& addr) {
    auto bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::map_get");
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_get", key);

    if (false == propobj->find(field)) {
        xwarn("xaccount_context_t::map_get fail-field not find.addr=%s,propname=%s,field=%s", get_address().c_str(), key.c_str(), field.c_str());
        return xaccount_property_map_field_not_create;
    }
    value = propobj->query(field);
    return xsuccess;
}

int32_t xaccount_context_t::map_set(const std::string & key, const std::string & field, const std::string & value) {
    xdbg("xaccount_context_t::map_set,property_modify_enter.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
    auto & bstate = get_bstate();
    auto propobj = load_map_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_set", key);
    if (true == propobj->find(field)) {
        auto old_value = propobj->query(field);
        if (old_value == value) {
            // TODO(jimmy) some system contract will set same value
            xwarn("xaccount_context_t::map_set-warn set same value.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
            return xsuccess;
        }
    }
    return propobj->insert(field, value, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::map_remove(const std::string & key, const std::string & field) {
    xdbg("xaccount_context_t::map_remove,property_modify_enter.address=%s,height=%ld,propname=%s,field=%s", get_address().c_str(), get_chain_height(), key.c_str(), field.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_remove", key);
    if (false == propobj->find(field)) {
        xwarn("xaccount_context_t::map_remove fail-field not find.addr=%s,propname=%s", get_address().c_str(), key.c_str());
        return xaccount_property_map_field_not_create;
    }
    return propobj->erase(field, m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::map_clear(const std::string & key) {
    xdbg("xaccount_context_t::map_clear,property_modify_enter.address=%s,height=%ld,propname=%s", get_address().c_str(), get_chain_height(), key.c_str());
    auto & bstate = get_bstate();
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_clear", key);
    return propobj->clear(m_canvas.get()) == true ? xsuccess : xaccount_property_operate_fail;
}
int32_t xaccount_context_t::map_size(const std::string & key, int32_t& size, const std::string& addr) {
    auto bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::map_size");
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_size", key);

    size = (int32_t)(propobj->query().size());
    return xstore_success;
}
int32_t xaccount_context_t::map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr) {
    auto bstate = load_bstate(addr);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::map_copy_get");
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::map_copy_get", key);

    map = propobj->query();
    return xsuccess;
}

int32_t xaccount_context_t::get_string_property(const std::string& key, std::string& value, uint64_t height, const std::string& addr) {
    auto bstate = load_bstate(addr, height);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::get_string_property");
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::get_string_property", key);

    value = propobj->query();
    return xsuccess;
}

int32_t xaccount_context_t::get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr) {
    auto bstate = load_bstate(addr, height);
    CHECK_BSTATE_NULL_RETURN(bstate, "xaccount_context_t::get_map_property");
    CHECK_FIND_PROPERTY(bstate, key);
    auto propobj = bstate->load_string_map_var(key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::get_map_property", key);

    value = propobj->query();
    return xsuccess;
}

int32_t xaccount_context_t::map_property_exist(const std::string& key) {
    auto & bstate = get_bstate();
    if (!bstate->find_property(key)) {
        return xaccount_property_not_create;
    }
    return xsuccess;
}

uint64_t xaccount_context_t::token_balance(const std::string& key) {
    auto & bstate = get_bstate();
    if (!bstate->find_property(key)) {
        return 0;
    }
    auto propobj = bstate->load_token_var(key);
    base::vtoken_t balance = propobj->get_balance();
    if (balance < 0) {
        xerror("xaccount_context_t::token_balance fail-should not appear. balance=%ld", balance);
        return 0;
    }
    return (uint64_t)balance;
}

int32_t xaccount_context_t::token_withdraw(const std::string& key, base::vtoken_t sub_token) {
    xdbg("xaccount_context_t::token_withdraw,property_modify_enter.address=%s,height=%ld,propname=%s,token=%ld", get_address().c_str(), get_chain_height(), key.c_str(), sub_token);
    auto & bstate = get_bstate();
    auto propobj = load_token_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::token_withdraw", key);
    auto balance = propobj->get_balance();
    if (sub_token <= 0 || sub_token > balance) {
        xwarn("xaccount_context_t::token_withdraw fail-can't do withdraw. propname=%ld,balance=%ld,sub_token=%ld", key.c_str(), balance, sub_token);
        return xaccount_property_operate_fail;
    }

    auto left_token = propobj->withdraw(sub_token, m_canvas.get());
    xassert(left_token < balance);
    return xsuccess;
}

int32_t xaccount_context_t::token_deposit(const std::string& key, base::vtoken_t add_token) {
    xdbg("xaccount_context_t::token_deposit,property_modify_enter.address=%s,height=%ld,propname=%s,token=%ld", get_address().c_str(), get_chain_height(), key.c_str(), add_token);
    auto & bstate = get_bstate();
    auto propobj = load_token_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::token_withdraw", key);
    if (add_token <= 0) {
        xwarn("xaccount_context_t::token_withdraw fail-can't do deposit. add_token=%ld", add_token);
        return xaccount_property_operate_fail;
    }
    auto balance = propobj->get_balance();
    auto left_token = propobj->deposit(add_token, m_canvas.get());
    xassert(left_token > balance);
    return xsuccess;
}

int32_t xaccount_context_t::uint64_add(const std::string& key, uint64_t change) {
    if (change == 0) {
        return xsuccess;
    }
    xdbg("xaccount_context_t::uint64_add,property_modify_enter.address=%s,height=%ld,propname=%s,change=%ld", get_address().c_str(), get_chain_height(), key.c_str(), change);
    auto & bstate = get_bstate();
    auto propobj = load_uin64_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::uint64_add", key);
    uint64_t oldvalue = propobj->get();
    uint64_t newvalue = oldvalue + change;  // TODO(jimmy) overflow ?
    propobj->set(newvalue, m_canvas.get());
    xdbg("xaccount_context_t::uint64_add property=%s,old_value=%ld,new_value=%ld,change=%ld", key.c_str(), oldvalue, newvalue, change);
    return xsuccess;
}
int32_t xaccount_context_t::uint64_sub(const std::string& key, uint64_t change) {
    if (change == 0) {
        return xsuccess;
    }
    xdbg("xaccount_context_t::uint64_sub,property_modify_enter.address=%s,height=%ld,propname=%s,change=%ld", get_address().c_str(), get_chain_height(), key.c_str(), change);
    auto & bstate = get_bstate();
    auto propobj = load_uin64_for_write(bstate.get(), key);
    CHECK_PROPERTY_NULL_RETURN(propobj, "xaccount_context_t::uint64_sub", key);
    uint64_t oldvalue = propobj->get();
    if (oldvalue < change) {
        xwarn("xaccount_context_t::uint64_sub fail-invalid para.value=%ld,change=%ld", oldvalue, change);
        return xaccount_property_operate_fail;
    }
    uint64_t newvalue = oldvalue - change;
    propobj->set(newvalue, m_canvas.get());
    xdbg("xaccount_context_t::uint64_sub property=%s,old_value=%ld,new_value=%ld,change=%ld", key.c_str(), oldvalue, newvalue, change);
    return xsuccess;
}


bool xaccount_context_t::get_transaction_result(xtransaction_result_t& result) {
    std::string property_binlog;
    m_canvas->encode(property_binlog);

    std::string fullstate_bin;
    get_bstate()->take_snapshot(fullstate_bin);

    result.m_property_binlog    = property_binlog;
    result.m_full_state         = fullstate_bin;
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

size_t xaccount_context_t::get_op_records_size() const {
    return m_canvas.get()->get_op_records_size();
}

bool xaccount_context_t::add_transaction(const xcons_transaction_ptr_t& trans) {
    m_contract_txs.clear();
    if (trans->is_self_tx() || trans->is_send_tx()) {
        if (m_latest_exec_sendtx_nonce != trans->get_transaction()->get_last_nonce()) {
            xwarn("xaccount_context_t::add_transaction fail-sendtx nonce unmatch. account=%s,account_tx_nonce=%ld,tx=%s",
                get_address().c_str(), m_latest_exec_sendtx_nonce, trans->dump().c_str());
            return false;
        }
        m_latest_exec_sendtx_nonce = trans->get_transaction()->get_tx_nonce();
        m_latest_exec_sendtx_hash = trans->get_tx_hash_256();

        // try update latest create nonce
        update_latest_create_nonce_hash(trans);  // self tx may create contract tx
    }
    xdbg("xaccount_context_t::add_transaction account=%s,height=%ld,tx=%s", get_address().c_str(), get_chain_height(), trans->dump(true).c_str());
    m_currect_transaction = trans;
    return true;
}
bool xaccount_context_t::finish_exec_all_txs(const std::vector<xcons_transaction_ptr_t> & txs) {
    xassert(m_currect_transaction != nullptr);

    // update account create time propertys
    set_account_create_time();

    // update tx related propertys
    if (get_chain_height() > 0) { // no need set for genesis block
        uint64_t new_sendtx_nonce = m_account->get_latest_send_trans_number();
        uint256_t new_sendtx_hash = m_account->account_send_trans_hash();
        uint32_t recv_tx_num = 0;
        uint32_t confirm_tx_num = 0;
        uint32_t send_tx_num = 0;
        for (auto & tx : txs) {
            if (tx->is_self_tx() || tx->is_send_tx()) {
                if (new_sendtx_nonce != tx->get_transaction()->get_last_nonce()) {
                    xerror("xaccount_context_t::finish_exec_all_txs fail-match send nonce hash.last_nonce=%ld,tx_nonce=%ld",
                        new_sendtx_nonce, tx->get_transaction()->get_last_nonce());
                    return false;
                }
                xassert(new_sendtx_nonce == tx->get_transaction()->get_last_nonce());
                new_sendtx_nonce = tx->get_transaction()->get_tx_nonce();
                new_sendtx_hash = tx->get_tx_hash_256();
            }
            if (tx->is_send_tx()) {
                send_tx_num++;
            }
            if (tx->is_recv_tx()) {
                recv_tx_num++;
            }
            if (tx->is_confirm_tx()) {
                confirm_tx_num++;
            }
        }

        // set latest sendtx nonce and hash
        if (new_sendtx_nonce != m_account->get_latest_send_trans_number()) {
            set_tx_info_latest_sendtx_num(new_sendtx_nonce);
            std::string transaction_hash_str = std::string(reinterpret_cast<char*>(new_sendtx_hash.data()), new_sendtx_hash.size());
            set_tx_info_latest_sendtx_hash(transaction_hash_str);
        }

        // set recvtx total number
        if (recv_tx_num > 0) {
            uint64_t recv_num = m_account->account_recv_trans_number();
            set_tx_info_recvtx_num(recv_num + recv_tx_num);
        }

        // set unconfirm sendtx number
        uint32_t old_unconfirm_num = m_account->get_unconfirm_sendtx_num();
        if (old_unconfirm_num + send_tx_num < confirm_tx_num) {
            xerror("xaccount_context_t::finish_exec_all_txs fail-unconfirm num unmatch.old_unconfirm_num=%d,send_tx_num=%d,confirm_tx_num=%d",
                old_unconfirm_num, send_tx_num, confirm_tx_num);
            return false;
        }
        uint32_t new_unconfirm_new = old_unconfirm_num + send_tx_num - confirm_tx_num;
        if (new_unconfirm_new != old_unconfirm_num) {
            set_tx_info_unconfirm_tx_num(new_unconfirm_new);
        }
        xdbg_info("xaccount_context_t::finish_exec_all_txs,account=%s,height=%ld,recv_tx_num=%d,send_tx_num=%d,confirm_tx_num=%d,new_sendtx_nonce=%ld",
            get_address().c_str(), get_chain_height(), recv_tx_num, send_tx_num, confirm_tx_num, new_sendtx_nonce);
    }

    return true;
}

void xaccount_context_t::set_account_create_time() {
    if (false == get_bstate()->find_property(XPROPERTY_ACCOUNT_CREATE_TIME)) {
        auto propobj = get_bstate()->new_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME, m_canvas.get());
        uint64_t create_time = m_timer_height == 0 ? base::TOP_BEGIN_GMTIME : m_timer_height;
        propobj->set(create_time, m_canvas.get());
    }
}

void xaccount_context_t::set_source_pay_info(const data::xaction_asset_out& source_pay_info) {
    m_source_pay_info = source_pay_info;
}

const data::xaction_asset_out& xaccount_context_t::get_source_pay_info() {
    return m_source_pay_info;
}

void xaccount_context_t::get_latest_create_nonce_hash(uint64_t & nonce, uint256_t & hash) {
    nonce = m_latest_create_sendtx_nonce;
    hash = m_latest_create_sendtx_hash;
}

void xaccount_context_t::update_latest_create_nonce_hash(const xcons_transaction_ptr_t & tx) {
    xassert(tx->is_self_tx() || tx->is_send_tx());
    // maybe not need update, it's ok
    if (m_latest_create_sendtx_nonce == tx->get_tx_last_nonce()) {
        m_latest_create_sendtx_nonce = tx->get_tx_nonce();
        m_latest_create_sendtx_hash = tx->get_tx_hash_256();
    }
}

int32_t xaccount_context_t::create_transfer_tx(const std::string & receiver, uint64_t amount) {
    xassert(data::is_contract_address(common::xaccount_address_t{ get_address() }));
    if (data::is_user_contract_address(common::xaccount_address_t{ get_address() })) {
        xwarn("xaccount_context_t::create_transfer_tx fail to create user contract transaction from:%s,to:%s,amount:%" PRIu64,
                get_address().c_str(), receiver.c_str(), amount);
        return xstore_user_contract_can_not_initiate_transaction;
    }

    uint64_t latest_sendtx_nonce;
    uint256_t latest_sendtx_hash;
    get_latest_create_nonce_hash(latest_sendtx_nonce, latest_sendtx_hash);

    xtransaction_ptr_t tx = data::xtx_factory::create_contract_subtx_transfer(get_address(), receiver, latest_sendtx_nonce, latest_sendtx_hash, amount, m_timestamp);
    xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());

    uint32_t contract_call_contracts_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
    if (m_contract_txs.size() >= contract_call_contracts_num) {
        xwarn("xaccount_context_t::create_transfer_tx contract transaction exceeds max from:%s,to:%s,amount:%" PRIu64,
                get_address().c_str(), receiver.c_str(), amount);
        return xaccount_contract_number_exceed_max;
    }
    m_contract_txs.push_back(constx);
    update_latest_create_nonce_hash(constx);
    xdbg_info("xaccount_context_t::create_transfer_tx tx:%s,from:%s,to:%s,amount:%ld,nonce:%ld",
        tx->get_digest_hex_str().c_str(), get_address().c_str(), receiver.c_str(), amount, tx->get_tx_nonce());
    return xstore_success;
}

int32_t xaccount_context_t::generate_tx(const std::string& target_addr, const std::string& func_name, const std::string& func_param) {
    if (data::is_user_contract_address(common::xaccount_address_t{ get_address() })) {
        xwarn("xaccount_context_t::generate_tx from:%s,to:%s,func_name:%s",
                get_address().c_str(), target_addr.c_str(), func_name.c_str());
        return xstore_user_contract_can_not_initiate_transaction;
    }

    uint64_t latest_sendtx_nonce;
    uint256_t latest_sendtx_hash;
    get_latest_create_nonce_hash(latest_sendtx_nonce, latest_sendtx_hash);

    xtransaction_ptr_t tx = data::xtx_factory::create_contract_subtx_call_contract(get_address(), target_addr, latest_sendtx_nonce, latest_sendtx_hash, func_name, func_param, m_timestamp);
    xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
    uint32_t contract_call_contracts_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
    if (m_contract_txs.size() >= contract_call_contracts_num) {
        xwarn("xaccount_context_t::generate_tx contract transaction exceeds max from:%s,to:%s,func_name:%s",
                get_address().c_str(), target_addr.c_str(), func_name.c_str());
        return xaccount_contract_number_exceed_max;
    }
    m_contract_txs.push_back(constx);
    update_latest_create_nonce_hash(constx);
    xdbg_info("xaccount_context_t::generate_tx tx:%s,from:%s,to:%s,func_name:%s,nonce:%ld,lasthash:%ld,func_param:%ld",
        tx->get_digest_hex_str().c_str(), get_address().c_str(), target_addr.c_str(), func_name.c_str(), tx->get_tx_nonce(), tx->get_last_hash(), base::xhash64_t::digest(func_param));
    return xstore_success;
}

data::xblock_t*
xaccount_context_t::get_block_by_height(const std::string & owner, uint64_t height) const {
    // TODO(jimmy)
    base::xvaccount_t _vaddr(owner);
    XMETRICS_GAUGE(metrics::blockstore_access_from_account_context, 1);
    base::xauto_ptr<base::xvblock_t> _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddr, height, base::enum_xvblock_flag_committed, false);
    if (_block != nullptr) {
        // system contract only need input for full-table
        base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddr, _block.get());
        _block->add_ref();
        return dynamic_cast<data::xblock_t*>(_block.get());
    }
    return nullptr;
}

data::xblock_t*
xaccount_context_t::get_next_full_block(const std::string & owner, const uint64_t cur_full_height) const {
    base::xvaccount_t vaddr(owner);
    uint64_t next_full_height = cur_full_height + XGET_CONFIG(fulltable_interval_block_num);
    XMETRICS_GAUGE(metrics::blockstore_access_from_account_context, 1);
    base::xauto_ptr<base::xvblock_t> block = base::xvchain_t::instance().get_xblockstore()->load_block_object(vaddr, next_full_height, base::enum_xvblock_flag_committed, false);
    xdbg("xaccount_context_t::get_next_full_block owner=%s,cur_height=%llu,next_full_height=%llu", owner.c_str(), cur_full_height, next_full_height);

    if (block != nullptr && block->get_block_level() == base::enum_xvblock_level_table && block->get_block_class() == base::enum_xvblock_class_full) {
        base::xvchain_t::instance().get_xblockstore()->load_block_input(vaddr, block.get());
        block->add_ref();
        return dynamic_cast<data::xblock_t*>(block.get());
    }
    return nullptr;
}

uint64_t
xaccount_context_t::get_blockchain_height(const std::string & owner) const {
    // TODO(jimmy) should talk with contract

    uint64_t height;
    if (owner == get_address()) {
        // m_account is proposal state, should return prev height
        height = m_account->get_block_height() > 0 ? m_account->get_block_height() - 1 : 0;
    } else if (owner == m_current_table_addr) {
        height = m_current_table_commit_height;
    } else {
        base::xvaccount_t _vaddr(owner);
        height = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block_height(_vaddr);
    }
    xdbg("xaccount_context_t::get_blockchain_height owner=%s,height=%" PRIu64 "", owner.c_str(), height);
    return height;
}

}  // namespace store
}  // namespace top
