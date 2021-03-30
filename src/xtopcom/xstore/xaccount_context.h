// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "xbase/xdata.h"
#include "xbasic/xobject_ptr.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xlogic_time.h"
#include "xdata/xaction_parse.h"
#include "xdata/xnative_property.h"
#include "xdata/xproperty.h"
#include "xdata/xlightunit.h"
#include "xstore/xaccount_cmd.h"
#include "xstore/xstore.h"

namespace top { namespace store {

using data::xtransaction_result_t;

#define HASH_POINTERS_KEY   "_haskpt_key_"

class xaccount_context_t {
 public:
    xaccount_context_t(const std::string& address, xstore_face_t* store);
    virtual ~xaccount_context_t();

    data::xblockchain2_t* get_blockchain() const {return m_account; }
    std::string const & get_address() const noexcept {return m_address;}
    xstore_face_t* get_store() const noexcept {return m_store;}
    bool get_transaction_result(xtransaction_result_t& result) const;
    xproperty_log_ptr_t get_property_log() const {return m_accountcmd->get_property_log();}

    bool    add_transaction(const xcons_transaction_ptr_t& trans);
    void    set_context_para(uint64_t clock, const std::string & random_seed, uint64_t timestamp, uint64_t sys_total_lock_tgas_token);
    const std::string & get_random_seed() const {return m_random_seed;}
    uint64_t get_timer_height() const {return m_timer_height;}
    uint64_t get_chain_height() const {return m_account->get_chain_height();}

    // property APIs
    int32_t create_user_account(const std::string& address);
    int32_t account_alias_name_set(const std::string& name);
    int32_t token_transfer_out(const data::xproperty_asset& asset, uint64_t gas_fee = 0, uint64_t service_fee = 0);
    int32_t other_token_transfer_out(const data::xproperty_asset& asset, uint64_t gas_fee = 0, uint64_t service_fee = 0);
    int32_t top_token_transfer_out(uint64_t amount, uint64_t gas_fee = 0, uint64_t service_fee = 0);
    int32_t token_transfer_in(const data::xproperty_asset& asset);
    int32_t other_token_transfer_in(const data::xproperty_asset& asset);
    int32_t top_token_transfer_in(uint64_t amount);

    int32_t account_set_keys(const std::string &key, const std::string &value);

    // check legal in transaction parse check, here, check balance and execute
    int32_t lock_token(const uint256_t &tran_hash, uint64_t amount, const std::string &tran_params);

    int32_t unlock_token(const uint256_t &tran_hash, const std::string &lock_hash_str, const std::vector<std::string> signs);
    int32_t unlock_all_token();
    int32_t authorize_key(uint32_t author_type, const std::string &params);

    uint64_t get_lock_token_sum();
    int32_t  set_lock_token_sum(uint64_t);

    int32_t  set_pledge_token_tgas(uint64_t);
    int32_t  redeem_pledge_token_tgas(uint64_t);

    int32_t  check_used_tgas(uint64_t &num, uint64_t deposit, uint64_t& deposit_usage);
    uint64_t get_used_tgas();
    int32_t  set_used_tgas(uint64_t);
    int32_t  incr_used_tgas(uint64_t);
    int32_t  update_tgas_sender(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage, bool is_contract);
    int32_t  update_tgas_contract_recv(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage, uint64_t& send_frozen_tgas, uint64_t deal_used_tgas);
    uint64_t get_total_tgas() const ;
    uint64_t get_available_tgas() const ;

    int32_t  calc_resource(uint64_t& tgas, uint32_t deposit, uint32_t& used_deposit);

    uint64_t get_last_tx_hour();
    int32_t  set_last_tx_hour(uint64_t);

    uint64_t get_pledge_token_disk();
    int32_t  set_pledge_token_disk(uint64_t);
    int32_t  redeem_pledge_token_disk(uint64_t);

    uint64_t get_used_disk();
    int32_t  set_used_disk(uint64_t);
    int32_t  update_disk(uint64_t);

    uint64_t get_tgas_limit();
    void     set_tgas_limit(uint64_t tgas_limit) { m_tgas_limit = tgas_limit;}

    int64_t get_balance_change() {return m_balance_change;}
    void    set_balance_change(int64_t balance_change) {m_balance_change += balance_change;}

    int64_t get_tgas_balance_change() {return m_pledge_balance_change.tgas;}
    void    set_tgas_balance_change(int64_t tgas_balance_change) {m_pledge_balance_change.tgas += tgas_balance_change;}

    int64_t get_disk_balance_change() {return m_pledge_balance_change.disk;}
    void    set_disk_balance_change(int64_t disk_balance_change) {m_pledge_balance_change.disk += disk_balance_change;}

    int64_t get_vote_balance_change() {return m_pledge_balance_change.vote;}
    void    set_vote_balance_change(int64_t vote_balance_change) {m_pledge_balance_change.vote += vote_balance_change;}

    int64_t get_lock_balance_change() {return m_lock_balance_change;}
    void    set_lock_balance_change(int64_t change) {m_lock_balance_change += change;}

    //int64_t get_lock_balance_change() {return m_lock_balance_change;}
    //void    set_lock_balance_change(int64_t change) {m_lock_balance_change += change;}

    int64_t get_lock_tgas_change() {return m_lock_tgas_change;}
    void    set_lock_tgas_change(int64_t lock_tgas_change) {m_lock_tgas_change += lock_tgas_change;}

    int64_t get_unvote_num_change() {return m_unvote_num_change;}
    void    set_unvote_num_change(int64_t unvote_num_change) {m_unvote_num_change += unvote_num_change;}

    int32_t update_pledge_vote_property(xaction_pledge_token_vote& action);
    int32_t merge_pledge_vote_property();
    int32_t insert_pledge_vote_property(xaction_pledge_token_vote& action);
    int32_t redeem_pledge_vote_property(uint64_t num);

    void deserilize_vote(const std::string& str, uint64_t& vote_num, uint16_t& duration, uint64_t& lock_time);
    std::string serilize_vote(uint64_t vote_num, uint16_t duration, uint64_t lock_time);
    data::xproperty_asset get_source_transfer_in();

    int32_t do_prop_set(xproperty_op_code_t cmd, const std::string & key);
    int32_t do_prop_set(xproperty_op_code_t cmd, const std::string & key, const std::string & op_para1);
    int32_t do_prop_set(xproperty_op_code_t cmd, const std::string & key, const std::string & op_para1, const std::string & op_para2);

    int32_t set_contract_code(const std::string &code);
    int32_t get_contract_code(std::string &code);

    xstring_ptr_t string_read_get(const std::string& prop_name, int32_t & error_code);
    xstrdeque_ptr_t deque_read_get(const std::string& prop_name, int32_t & error_code);
    xstrmap_ptr_t map_read_get(const std::string& prop_name, int32_t & error_code);

    // common data operation APIs
    int32_t string_create(const std::string& key);
    int32_t string_set(const std::string& key, const std::string& value, bool native = false);
    int32_t string_get(const std::string& key, std::string& value, const std::string& addr="");
    int32_t string_empty(const std::string& key, bool& empty);
    int32_t string_size(const std::string& key, int32_t& size);

    int32_t list_create(const std::string& key);
    int32_t list_push_back(const std::string& key, const std::string& value, bool native = false);
    int32_t list_push_front(const std::string& key, const std::string& value, bool native = false);
    int32_t list_pop_back(const std::string& key, std::string& value, bool native = false);
    int32_t list_pop_front(const std::string& key, std::string& value, bool native = false);
    int32_t list_clear(const std::string &key, bool native = false);
    int32_t list_get_back(const std::string& key, std::string & value);
    int32_t list_get_front(const std::string& key, std::string & value);
    int32_t list_get(const std::string& key, const uint32_t index, std::string & value, const std::string& addr="");
    int32_t list_empty(const std::string& key, bool& empty);
    int32_t list_size(const std::string& key, int32_t& size, const std::string& addr="");
    int32_t list_get_range(const std::string &key, int32_t start, int32_t stop, std::vector<std::string> &values);
    int32_t list_get_all(const std::string &key, std::vector<std::string> &values, const std::string& addr = "");
    int32_t list_copy_get(const std::string &prop_name, std::deque<std::string> & deque);

    int32_t map_create(const std::string& key);
    int32_t map_get(const std::string & key, const std::string & field, std::string & value, const std::string& addr="");
    int32_t map_set(const std::string & key, const std::string & field, const std::string & value, bool native = false);
    int32_t map_remove(const std::string & key, const std::string & field, bool native = false);
    int32_t map_clear(const std::string & key, bool native = false);
    int32_t map_empty(const std::string & key, bool& empty);
    int32_t map_size(const std::string & key, int32_t& size, const std::string& addr="");
    int32_t map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr="");
    int32_t get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr="");
    int32_t map_property_exist(const std::string& key);

    int32_t create_transfer_tx(const std::string & receiver, uint64_t amount);
    int32_t generate_tx(const std::string& target_addr, const std::string& func_name, const std::string& func_param);
    int32_t check_create_property(const std::string& key);
    int32_t set_parent_account(const uint64_t amount, const std::string& value);
    int32_t set_sub_account(const std::string& value);
    int32_t sub_account_check(const std::string& value);
    int32_t remove_sub_account(const std::string& value);
    int32_t remove_contract_sub_account(const std::string& value);

    int32_t set_contract_sub_account(const std::string& value);
    int32_t set_contract_parent_account(const uint64_t amount, const std::string& value);
    int32_t sub_contract_sub_account_check(const std::string& value);
    int32_t exist_sub_or_contract_account();

    int32_t get_parent_account(std::string &value);

    void set_source_pay_info(const data::xaction_asset_out& source_pay_info);
    const data::xaction_asset_out& get_source_pay_info();

    bool    can_work();

    int32_t vote_out(const std::string& addr_to, const std::string& lock_hash, uint64_t vote_amount, uint64_t expiration);
    int32_t vote_in(const std::string& address, const std::string& lock_hash_str, uint64_t amount);

    data::xblock_t*
    get_block_by_height(const std::string & owner, uint64_t height) const;

    uint64_t get_blockchain_height(const std::string & owner) const;

 private:
    int32_t get_vote_info(const std::string& lock_hash_str, std::string& value);
    int32_t get_vote_out_info(const std::string& addr_to, const std::string& lock_hash_str, std::string& value);
    int32_t clear_vote_out_info(const std::string& lock_hash);

 public:
    uint32_t get_token_price() const ;
    uint64_t calc_decayed_tgas();
    uint32_t m_cur_used_tgas{0};
    uint32_t m_cur_used_deposit{0};

 private:
    std::string         m_address;
    xstore_face_t*      m_store;
    xblockchain2_t*     m_account{nullptr};
    std::shared_ptr<xaccount_cmd> m_accountcmd;
    xcons_transaction_ptr_t  m_currect_transaction;
    std::vector<xcons_transaction_ptr_t> m_contract_txs;
    int64_t             m_balance_change{0};
    xpledge_balance_change m_pledge_balance_change;
    int64_t             m_lock_balance_change{0};
    //int64_t             m_lock_balance_change{0};
    int64_t             m_lock_tgas_change{0};
    int64_t             m_unvote_num_change{0};
    data::xnative_property_t  m_native_property;
    data::xaction_asset_out m_source_pay_info;
    uint64_t            m_timestamp{0};
    uint64_t            m_timer_height{0};
    uint64_t            m_tgas_limit{0};
    std::string         m_random_seed;
    uint64_t            m_sys_total_lock_tgas_token{0};
};

}  // namespace store
}  // namespace top
