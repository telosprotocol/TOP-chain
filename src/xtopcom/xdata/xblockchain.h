// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbase/xobject_ptr.h"
#include "xdata/xaccount_mstate.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, data)

class xblockchain2_t : public xbase_dataobj_t<xblockchain2_t, xdata_type_blockchain> {
 public:
    enum {
        enum_blockchain_ext_type_uncnfirmed_accounts = 1,
        enum_blockchain_ext_type_binlog              = 2,
    };
 public:
    xblockchain2_t(uint32_t chainid, const std::string & account, base::enum_xvblock_level level);
    // default is main chain id or get chain id from account
    xblockchain2_t(const std::string & account, base::enum_xvblock_level level);
    xblockchain2_t(const std::string & account);
    xblockchain2_t() = default;
 protected:
     ~xblockchain2_t() override = default;
 private:
    xblockchain2_t(const xblockchain2_t &);
    xblockchain2_t & operator = (const xblockchain2_t &);
 public:
    virtual int32_t do_write(base::xstream_t & stream) override;
    virtual int32_t do_read(base::xstream_t & stream) override;

 public:  // update blockchain by block
    bool        update_last_block_state(const xblock_t* block);
    bool        update_state_by_genesis_block(const xblock_t* block);
    bool        update_state_by_next_height_block(const xblock_t* block);
    bool        update_state_by_full_block(const xblock_t* block);
    bool        apply_block(const xblock_t* block);  // TODO(jimmy) should move to statestore future
    xobject_ptr_t<xblockchain2_t>   clone_state();

 public:  // api for basic blockchain
    const std::string & get_account()const {return m_account;}
    base::enum_xvblock_level    get_block_level() const {return m_block_level;}
    uint64_t            get_chain_height()const {return m_last_state_block_height;}
    uint64_t            get_account_create_time() const {return m_account_state.get_account_create_time();}
    uint64_t            get_last_height()const {return m_last_state_block_height;}
    const std::string & get_last_block_hash()const {return m_last_state_block_hash;}
    std::string         to_basic_string() const;

 public:  // set api for account context, use for save temp change
    inline void         set_balance(uint64_t balance) {m_account_state.set_balance(balance);}
    inline void         set_burn_balance(uint64_t token) {m_account_state.set_burn_balance(token);}
    inline void         set_tgas_balance(uint64_t tgas_balance) {m_account_state.set_pledge_tgas_balance(tgas_balance);}
    inline void         set_disk_balance(uint64_t disk_balance) {m_account_state.set_pledge_disk_balance(disk_balance);}
    inline void         set_vote_balance(uint64_t vote_balance) {m_account_state.set_pledge_vote_balance(vote_balance);}
    inline void         set_lock_tgas(uint64_t lock_tgas) {m_account_state.set_lock_tgas(lock_tgas);}
    inline void         set_unvote_num(uint64_t unvote_num) {m_account_state.set_unvote_number(unvote_num);}
    inline void         set_account_send_trans_number(uint64_t number) {m_account_state.set_latest_send_trans_number(number);}
    inline void         set_account_send_trans_hash(const uint256_t & hash) {m_account_state.set_latest_send_trans_hash(hash);}


    xtransaction_ptr_t  make_transfer_tx(const std::string & to, uint64_t amount, uint64_t firestamp, uint16_t duration, uint32_t deposit, const std::string& token_name = "TOP");
    xtransaction_ptr_t  make_run_contract_tx(const std::string & to, const std::string& func_name, const std::string& func_param, uint64_t amount,
                        uint64_t firestamp, uint16_t duration, uint32_t deposit);

 public:  // for unit account
    inline uint64_t     balance()const {return m_account_state.get_balance();}
    inline uint64_t     burn_balance()const {return m_account_state.get_burn_balance();}
    inline uint64_t     tgas_balance() const {return m_account_state.get_pledge_tgas_balance();}
    inline uint64_t     disk_balance() const {return m_account_state.get_pledge_disk_balance();}
    inline uint64_t     vote_balance() const {return m_account_state.get_pledge_vote_balance();}
    inline uint64_t     lock_balance() const {return m_account_state.get_lock_balance();}
    //inline uint64_t     lock_balance() const {return m_account_state.get_lock_balance();}
    inline uint64_t     lock_tgas() const {return m_account_state.get_lock_tgas();}
    inline uint64_t     unvote_num() const {return m_account_state.get_unvote_number();}
    inline uint16_t     get_unconfirm_sendtx_num() const {return m_account_state.get_unconfirm_sendtx_num();}
    inline uint64_t     get_last_full_unit_height() const {return m_last_full_block_height;}
    inline const std::string & get_last_full_unit_hash() const {return m_last_full_block_hash;}
    inline const uint256_t & account_send_trans_hash()const {return m_account_state.get_latest_send_trans_hash();}
    inline uint64_t     account_send_trans_number()const {return m_account_state.get_latest_send_trans_number();}
    inline uint64_t     account_recv_trans_number()const {return m_account_state.get_latest_recv_trans_number();}

    bool        get_property_hash(const std::string& key, std::string& hash) const {return m_account_state.get_property_hash(key, hash);}
    const       std::map<std::string, std::string> & get_property_hash_map() const {return m_account_state.get_propertys_hash();}
    size_t      get_property_hash_map_size() const {return m_account_state.get_propertys_hash().size();}
    const       xnative_property_t& get_native_property() const {return m_account_state.get_native_property();}
    const       xaccount_mstate2 & get_account_mstate() const {return m_account_state;}

    uint64_t get_free_tgas() const ;
    uint64_t get_total_tgas(uint32_t token_price) const ;
    uint64_t get_last_tx_hour() const ;
    uint64_t get_used_tgas() const ;
    uint64_t calc_decayed_tgas(uint64_t timer_height) const ;
    static uint32_t get_token_price(uint64_t onchain_total_pledge_token) ;
    uint64_t get_available_tgas(uint64_t timer_height, uint32_t token_price) const ;

 public: // for table account
    void set_unconfirmed_accounts(const std::set<std::string> & accounts);
    const std::set<std::string> get_unconfirmed_accounts() const;
    void            set_extend_data(uint16_t name, const std::string & value);
    std::string     get_extend_data(uint16_t name);

 public:
    bool    add_full_table(const xblock_t* block);
    bool    add_light_table(const xblock_t* block);

 private:
    bool        add_light_unit(const xblock_t* block);
    bool        add_full_unit(const xblock_t* block);
    bool        add_table(const xblock_t* block);
    void        update_block_height_hash_info(const xblock_t * block);
    void        update_account_create_time(const xblock_t * block);

 public:  // old apis
    const std::string & address() {return m_account;}

 public:  // property apis
    void        set_property(const std::string & prop, const xdataobj_ptr_t & obj);
    void        set_all_propertys(const std::map<std::string, xdataobj_ptr_t> & propobjs);
    xdataobj_ptr_t      find_property(const std::string & prop) const;
    const std::map<std::string, xdataobj_ptr_t> &   get_property_objs() const {return m_property_objs;}

 private:
    uint8_t                     m_version{0};
    std::string                 m_account;
    base::enum_xvblock_level    m_block_level;
    uint64_t                    m_last_state_block_height{0};
    std::string                 m_last_state_block_hash{};
    uint64_t                    m_last_full_block_height{0};
    std::string                 m_last_full_block_hash{};
    uint64_t                    m_property_confirm_height{0};
    xaccount_mstate2            m_account_state;
    std::map<uint16_t, std::string> m_ext;
    std::map<std::string, xdataobj_ptr_t>   m_property_objs;
};

using xaccount_t = xblockchain2_t;
using xblockchain2_ptr_t = xobject_ptr_t<xblockchain2_t>;
using xaccount_ptr_t = xobject_ptr_t<xblockchain2_t>;
using xblockchain_ptr_t = xobject_ptr_t<xblockchain2_t>;

NS_END2
