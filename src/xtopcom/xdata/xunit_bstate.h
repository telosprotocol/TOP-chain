// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xcommon/xchain_uuid.h"
#include "xdata/xblock.h"
#include "xdata/xbstate_ctx.h"
#include "xdata/xproperty.h"
#include "xdata/xsystem_contract/xallowance.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstatistic/xstatistic.h"
#include "xvledger/xvstate.h"

#include <string>

NS_BEG2(top, data)

enum class xtop_allowance_update_op {
    invalid = 0,
    increase,
    decrease,
};
using xallowance_update_op_t = xtop_allowance_update_op;

class xunit_bstate_t : public xbstate_ctx_t, public xstatistic::xstatistic_obj_face_t {
 public:
    xunit_bstate_t(base::xvbstate_t* bstate, bool readonly = true);
    
    ~xunit_bstate_t();
 private:
    xunit_bstate_t();
    xunit_bstate_t(const xunit_bstate_t &);
    xunit_bstate_t & operator = (const xunit_bstate_t &);

 public:  // for unit account
    bool                is_empty_state() const;
    inline uint64_t     balance()const {return token_get(XPROPERTY_BALANCE_AVAILABLE);}
    inline uint64_t     burn_balance()const {return token_get(XPROPERTY_BALANCE_BURN);}
    inline uint64_t     tgas_balance() const {return token_get(XPROPERTY_BALANCE_PLEDGE_TGAS);}
    inline uint64_t     disk_balance() const {return 0;}  // TODO(jimmy) not support
    inline uint64_t     vote_balance() const {return token_get(XPROPERTY_BALANCE_PLEDGE_VOTE);}
    inline uint64_t     lock_balance() const {return token_get(XPROPERTY_BALANCE_LOCK);}
    inline uint64_t     lock_tgas() const {return uint64_property_get(XPROPERTY_LOCK_TGAS);}
    inline uint64_t     unvote_num() const {return uint64_property_get(XPROPERTY_UNVOTE_NUM);}
    uint64_t            get_account_create_time() const;
    uint64_t            get_free_tgas() const ;
    uint64_t            get_total_tgas(uint32_t token_price) const ;
    uint64_t            get_last_tx_hour() const ;
    uint64_t            get_used_tgas() const ;
    uint64_t            calc_decayed_tgas(uint64_t timer_height) const ;
    static uint32_t     get_token_price(uint64_t onchain_total_gas_deposit) ;
    uint64_t            get_available_tgas(uint64_t timer_height, uint32_t token_price) const ;
    std::string         get_code() const;
    std::string         get_storage(const std::string& index_str) const;
    uint64_t            available_tgas(uint64_t timer_height, uint64_t onchain_total_gas_deposit) const;

    int32_t balance(uint64_t new_balance);
    int32_t burn_balance(uint64_t new_burn_balance);
    int32_t tgas_balance(uint64_t new_tgas_balance);
    int32_t vote_balance(uint64_t new_vote_balance);
    int32_t lock_balance(uint64_t new_lock_balance);

 public: // Set APIs
    int32_t     set_account_create_time(uint64_t clock);
    int32_t     set_tx_info_latest_sendtx_num(uint64_t num);
    int32_t     set_tx_info_latest_sendtx_hash(const std::string & hash);

    void transfer(common::xtoken_id_t const token_id, observer_ptr<xunit_bstate_t> const & recver_state, evm_common::u256 const & value, std::error_code & ec);

    evm_common::u256 allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, std::error_code & ec) const;
    std::map<common::xtoken_id_t, data::system_contract::xallowance_t> allowance(std::error_code & ec) const;

    void update_allowance(common::xtoken_id_t const token_id,
                          common::xaccount_address_t const & spender,
                          evm_common::u256 const & amount,
                          xallowance_update_op_t const op,
                          std::error_code & ec);
    void approve(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec);

    common::xaccount_address_t tep_token_owner(common::xchain_uuid_t chain_uuid) const;
    void tep_token_owner(common::xchain_uuid_t chain_uuid, common::xaccount_address_t const & new_owner, std::error_code & ec);
    common::xaccount_address_t tep_token_controller(common::xchain_uuid_t chain_uuid) const;
    void tep_token_controller(common::xchain_uuid_t chain_uuid, common::xaccount_address_t const & new_controller, std::error_code & ec);

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_unit_bstate;}

private:
    xbytes_t raw_allowance(common::xtoken_id_t const token_id, std::error_code & ec) const;
    xobject_ptr_t<base::xmapvar_t<std::string>> raw_allowance(std::error_code & ec) const;
    void raw_allowance(common::xtoken_id_t const token_id, xbytes_t const & raw_data, std::error_code & ec);
    evm_common::u256 allowance_impl(xbytes_t const & serialized_allowance_map, common::xaccount_address_t const & spender, std::error_code & ec) const;
    std::map<common::xtoken_id_t, data::system_contract::xallowance_t> allowance_impl(xobject_ptr_t<base::xmapvar_t<std::string>> const & raw_allowance_data,
                                                                                      std::error_code & ec) const;
    void set_allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec);
    void dec_allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> raw_owner(std::error_code & ec) const;
    xbytes_t raw_owner(common::xchain_uuid_t chain_uuid, std::error_code & ec) const;
    xobject_ptr_t<base::xmapvar_t<std::string>> raw_controller(std::error_code & ec) const;
    xbytes_t raw_controller(common::xchain_uuid_t chain_uuid, std::error_code & ec) const;

    virtual int32_t get_object_size_real() const override;

private:  // TODO(jimmy) this apis will be droped in future
    uint32_t            get_unconfirm_sendtx_num() const;
    int32_t             set_tx_info_recvtx_num(uint64_t num);
    uint64_t            account_recv_trans_number() const;
    uint256_t           account_send_trans_hash() const;
    uint64_t            account_send_trans_number() const;  
    uint64_t            get_latest_send_trans_number() const; // XTODO not to use tx infos from unitstate      
};

using xunitstate_ptr_t = std::shared_ptr<xunit_bstate_t>;

class xaccount_state_t {
public:
    xaccount_state_t(xunitstate_ptr_t const& unitstate, base::xaccount_index_t const& accoutindex);
    xunitstate_ptr_t const&     get_unitstate() const {return m_unitstate;}
    uint64_t                    get_tx_nonce() const {return m_accountindex.get_latest_tx_nonce();}
    std::string const&          get_unit_hash() const {return m_accountindex.get_latest_unit_hash();}
    base::xaccount_index_t const& get_accountindex() const {return m_accountindex;}
public:  // set in consensus
    void                increase_tx_nonce();
    void                set_tx_nonce(uint64_t txnonce);
    void                update_account_index(base::xaccount_index_t const& accoutindex);
    bool                do_rollback();
    size_t              do_snapshot();

private:
    xunitstate_ptr_t            m_unitstate{nullptr};
    base::xaccount_index_t      m_accountindex;
    uint64_t                    m_nonce_snapshot{0};    
};
using xaccountstate_ptr_t = std::shared_ptr<xaccount_state_t>;

NS_END2
