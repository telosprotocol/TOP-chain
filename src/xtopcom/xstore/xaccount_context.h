// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvproperty.h"
#include "xvledger/xvstate.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xtop_log.h"
#include "xdata/xaction_parse.h"
#include "xdata/xproperty.h"
#include "xdata/xlightunit.h"
#include "xdata/xunit_bstate.h"
#include "xcommon/common.h"
#include "xstatectx/xstatectx_face.h"

namespace top { namespace store {

using data::xtransaction_result_t;

const uint16_t MIN_VOTE_LOCK_DAYS = 30;
const uint16_t MAX_VOTE_LOCK_DAYS = 570;
const uint64_t AMPLIFY_FACTOR = 1e6;
const uint64_t MAX_TOP_VOTE_RATE = 2 * AMPLIFY_FACTOR;
const uint64_t EXP_BASE = 104 * 1e4;

class xaccount_context_t {
 public:
    xaccount_context_t(const data::xunitstate_ptr_t & unitstate, const statectx::xstatectx_face_ptr_t & statectx, uint64_t tx_nonce);
    xaccount_context_t(const data::xunitstate_ptr_t & unitstate);

    virtual ~xaccount_context_t();

    const data::xunitstate_ptr_t & get_blockchain() const {
        return m_account;
    }
    std::string get_address() const {return m_account->account_address().to_string();}
    bool    get_transaction_result(xtransaction_result_t& result);
    size_t  get_op_records_size() const;
    const std::vector<data::xcons_transaction_ptr_t> & get_create_txs() const {
        return m_contract_txs;
    }


    bool add_transaction(const data::xcons_transaction_ptr_t & trans);
    void    set_context_para(uint64_t clock, const std::string & random_seed, uint64_t timestamp, uint64_t sys_total_lock_tgas_token);
    void    set_context_pare_current_table(const std::string & table_addr, uint64_t table_committed_height);
    const std::string & get_random_seed() const {return m_random_seed;}
    uint64_t get_timer_height() const {return m_timer_height;}
    uint64_t get_chain_height() const {return m_account->height();}

    // property APIs
    int32_t create_user_account(const std::string& address);
    int32_t token_transfer_out(const data::xproperty_asset& asset, evm_common::u256 amount256 = 0, uint64_t gas_fee = 0, uint64_t service_fee = 0);
    int32_t top_token_transfer_out(uint64_t amount, uint64_t gas_fee = 0, uint64_t service_fee = 0);
    int32_t token_transfer_in(const data::xproperty_asset& asset, evm_common::u256 amount256);
    int32_t top_token_transfer_in(uint64_t amount);

    // check legal in transaction parse check, here, check balance and execute
    int32_t lock_token(const uint256_t &tran_hash, uint64_t amount, const std::string &tran_params);

    int32_t unlock_token(const uint256_t &tran_hash, const std::string &lock_hash_str, const std::vector<std::string> signs);
    int32_t unlock_all_token();

    int32_t  check_used_tgas(uint64_t &num, uint64_t deposit, uint64_t& deposit_usage);
    uint64_t get_used_tgas();
    int32_t  set_used_tgas(uint64_t);
    int32_t  incr_used_tgas(uint64_t);
    int32_t  update_tgas_sender(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage, bool is_contract);
    int32_t  update_tgas_sender(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage);
    int32_t  update_tgas_contract_recv(uint64_t tgas_usage, const uint32_t deposit, uint64_t& deposit_usage, uint64_t& send_frozen_tgas, uint64_t deal_used_tgas);
    uint64_t get_total_tgas() const ;
    uint64_t get_available_tgas() const ;
    uint64_t get_total_gas_burn() const { return m_total_gas_burn;}

    int32_t  calc_resource(uint64_t& tgas, uint32_t deposit, uint32_t& used_deposit);

    uint64_t get_last_tx_hour();
    int32_t  set_last_tx_hour(uint64_t);

    int32_t  update_disk(uint64_t);

    int32_t other_balance_to_available_balance(const std::string & property_name, base::vtoken_t token);
    int32_t available_balance_to_other_balance(const std::string & property_name, base::vtoken_t token);

    int32_t update_pledge_vote_property(data::xaction_pledge_token_vote & action);
    static std::string serilize_vote_map_field(uint16_t duration, uint64_t lock_time);
    static std::string serilize_vote_map_value(uint64_t vote_num);
    static void deserilize_vote_map_field(const std::string& str, uint16_t& duration, uint64_t& lock_time);
    static void deserilize_vote_map_value(const std::string& str, uint64_t& vote_num);
    int32_t merge_pledge_vote_property();
    int32_t insert_pledge_vote_property(data::xaction_pledge_token_vote & action);
    int32_t redeem_pledge_vote_property(uint64_t num);
    static uint64_t get_top_by_vote(uint64_t vote_num, uint16_t duration);

    void deserilize_vote(const std::string& str, uint64_t& vote_num, uint16_t& duration, uint64_t& lock_time);
    std::string serilize_vote(uint64_t vote_num, uint16_t duration, uint64_t lock_time);

    xstring_ptr_t string_read_get(const std::string& prop_name, int32_t & error_code);
    xstrdeque_ptr_t deque_read_get(const std::string& prop_name, int32_t & error_code);
    xstrmap_ptr_t map_read_get(const std::string& prop_name, int32_t & error_code);

    // common data operation APIs
    int32_t string_create(const std::string& key);
    int32_t string_set(const std::string& key, const std::string& value);
    int32_t string_get(const std::string& key, std::string& value, const std::string& addr="");
    int32_t get_string_property(const std::string& key, std::string& value, uint64_t height, const std::string& addr="");

    int32_t list_create(const std::string& key);
    int32_t list_push_back(const std::string& key, const std::string& value);
    int32_t list_push_front(const std::string& key, const std::string& value);
    int32_t list_pop_back(const std::string& key, std::string& value);
    int32_t list_pop_front(const std::string& key, std::string& value);
    int32_t list_clear(const std::string &key);
    int32_t list_get(const std::string& key, const uint32_t index, std::string & value, const std::string& addr="");
    int32_t list_size(const std::string& key, int32_t& size, const std::string& addr="");
    int32_t list_get_all(const std::string &key, std::vector<std::string> &values, const std::string& addr = "");
    int32_t list_copy_get(const std::string &prop_name, std::deque<std::string> & deque);

    int32_t map_create(const std::string& key);
    int32_t map_get(const std::string & key, const std::string & field, std::string & value, const std::string& addr="");
    int32_t map_set(const std::string & key, const std::string & field, const std::string & value);
    int32_t map_remove(const std::string & key, const std::string & field);
    int32_t map_clear(const std::string & key);
    int32_t map_size(const std::string & key, int32_t& size, const std::string& addr="");
    int32_t map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr="");
    int32_t get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr="");
    int32_t map_property_exist(const std::string& key);

    base::xauto_ptr<base::xtokenvar_t> load_token_for_write(base::xvbstate_t* bstate, const std::string & key);
    uint64_t    token_balance(const std::string& key);
    int32_t     token_deposit(const std::string& key, base::vtoken_t add_token);
    int32_t     token_withdraw(const std::string& key, base::vtoken_t sub_token);

    base::xauto_ptr<base::xvintvar_t<uint64_t>> load_uin64_for_write(base::xvbstate_t* bstate, const std::string & key);
    int32_t     uint64_add(const std::string& key, uint64_t change);
    int32_t     uint64_sub(const std::string& key, uint64_t change);

    int32_t create_transfer_tx(const std::string & receiver, uint64_t amount);
    int32_t generate_tx(const std::string& target_addr, const std::string& func_name, const std::string& func_param);
    int32_t check_create_property(const std::string& key);

    void set_source_pay_info(const data::xaction_asset_out& source_pay_info);
    const data::xaction_asset_out& get_source_pay_info();

    data::xblock_t*
    get_block_by_height(const std::string & owner, uint64_t height) const;
    data::xblock_t*
    get_next_full_block(const std::string & owner, const uint64_t cur_full_height) const;

    uint64_t get_blockchain_height(const std::string & owner) const;

    int64_t get_tgas_balance_change() const { return m_tgas_balance_change; }
    void add_tgas_balance_change(uint64_t amount) { m_tgas_balance_change += amount; }
    void sub_tgas_balance_change(uint64_t amount) { m_tgas_balance_change -= amount; }

 private:
    const xobject_ptr_t<base::xvbstate_t> & get_bstate() const;
    xobject_ptr_t<base::xvbstate_t>         load_bstate(const std::string & other_addr);
    xobject_ptr_t<base::xvbstate_t>         load_bstate(const std::string & other_addr, uint64_t height);
    base::xauto_ptr<base::xstringvar_t>     load_string_for_write(base::xvbstate_t* bstate, const std::string & key);
    base::xauto_ptr<base::xdequevar_t<std::string>> load_deque_for_write(base::xvbstate_t* bstate, const std::string & key);
    base::xauto_ptr<base::xmapvar_t<std::string>>   load_map_for_write(base::xvbstate_t* bstate, const std::string & key);

    void    get_latest_create_nonce_hash(uint64_t & nonce);
    void update_latest_create_nonce_hash(const data::xcons_transaction_ptr_t & tx);
    void    set_account_create_time();

 public:
    uint32_t get_token_price() const ;
    uint64_t calc_decayed_tgas();
    uint32_t m_cur_used_tgas{0};
    uint32_t m_cur_used_deposit{0};

 private:
    data::xunitstate_ptr_t m_account{nullptr};
    uint64_t            m_latest_exec_sendtx_nonce{0};  // for exec tx
    uint64_t            m_latest_create_sendtx_nonce{0};  // for contract create tx

    xobject_ptr_t<base::xvcanvas_t>     m_canvas{nullptr};
    statectx::xstatectx_face_ptr_t     m_statectx{nullptr};
    data::xcons_transaction_ptr_t m_currect_transaction{nullptr};
    std::vector<data::xcons_transaction_ptr_t> m_contract_txs;

    std::vector<data::xcons_transaction_ptr_t> m_succ_contract_txs;

    data::xaction_asset_out m_source_pay_info;
    uint64_t            m_timestamp{0};
    uint64_t            m_timer_height{0};
    uint64_t            m_tgas_limit{0};
    int64_t             m_tgas_balance_change{0};
    std::string         m_random_seed;
    uint64_t            m_sys_total_lock_tgas_token{0};

    std::string         m_current_table_addr;
    uint64_t            m_current_table_commit_height{0};
    uint64_t            m_total_gas_burn{0};

 public:
    common::xtop_logs_t const & logs() const noexcept;
    void add_log(common::xtop_log_t log_);
 private:
    common::xtop_logs_t logs_;
};

using xaccount_context_ptr_t = std::shared_ptr<xaccount_context_t>;

}  // namespace store
}  // namespace top
