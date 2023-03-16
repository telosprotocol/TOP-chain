// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xcommon/xfixed_hash.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvpropertyprove.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xcommon.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xstatestore/xtablestate_ext.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvpropertyprove.h"
#include "xstatectx/xunitstate_ctx.h"

NS_BEG2(top, statestore)

class xstate_sync_info_t {
public:
    xstate_sync_info_t() {
    }
    xstate_sync_info_t(uint64_t height, const evm_common::xh256_t & root_hash, const evm_common::xh256_t & table_state_hash, const std::string & blockhash)
      : m_height(height), m_root_hash(root_hash), m_table_state_hash(table_state_hash), m_blockhash(blockhash) {
    }
    uint64_t get_height() const {
        return m_height;
    }
    const evm_common::xh256_t & get_root_hash() const {
        return m_root_hash;
    }
    const evm_common::xh256_t & get_table_state_hash() const {
        return m_table_state_hash;
    }

    const std::string & get_blockhash() const {
        return m_blockhash;
    }

private:
    uint64_t m_height;
    evm_common::xh256_t m_root_hash;
    evm_common::xh256_t m_table_state_hash;
    std::string m_blockhash;
};

// the statestore interface
class xstatestore_face_t {
 public:
    xstatestore_face_t() = default;
    virtual ~xstatestore_face_t() {}    
 public:
    virtual bool                    start(const xobject_ptr_t<base::xiothread_t> & iothread, const xobject_ptr_t<base::xiothread_t> & iothread_for_prune) = 0;
    virtual xtablestate_ext_ptr_t   get_tablestate_ext_from_block(base::xvblock_t * target_block) const = 0;
    
    // query accountindex
    virtual bool                    get_accountindex_from_latest_connected_table(common::xaccount_address_t const & table_address, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const = 0;
    virtual bool                    get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const = 0;
    virtual bool                    get_accountindex(const std::string& table_height, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const = 0;
    virtual bool                        get_accountindex(xblock_number_t number, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const = 0;
    virtual data::xaccountstate_ptr_t   get_accountstate(xblock_number_t number, common::xaccount_address_t const & account_address) const = 0;
    virtual std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> get_all_accountindex(base::xvblock_t * table_block, std::error_code & ec) const = 0;

    // query unitstate
    virtual data::xunitstate_ptr_t      get_unitstate(xblock_number_t number, common::xaccount_address_t const & account_address) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_latest_connectted_change_state(common::xaccount_address_t const & account_address) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_latest_connectted_state(common::xaccount_address_t const & account_address) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_committed_changed_state(common::xaccount_address_t const & account_address, uint64_t height) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_committed_state(common::xaccount_address_t const & account_address, uint64_t height) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_state_by_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& index) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_state_by_unit_block(base::xvblock_t * target_block) const = 0;
    virtual data::xunitstate_ptr_t  get_unit_state_by_table(common::xaccount_address_t const & account_address, const std::string& table_height) const = 0;
    virtual uint64_t                get_blockchain_height(common::xaccount_address_t const & account_address) const = 0;
    virtual int32_t                 map_get(common::xaccount_address_t const & account_address, const std::string &key, const std::string &field, std::string &value) const = 0;
    virtual int32_t                 string_get(common::xaccount_address_t const & account_address, const std::string& key, std::string& value) const = 0;
    virtual int32_t                 map_copy_get(common::xaccount_address_t const & account_address, const std::string &key, std::map<std::string, std::string> &map) const = 0;
    virtual int32_t                 get_map_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) const = 0;
    virtual int32_t                 get_string_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::string &value) const = 0;

    // query tablestate
    virtual data::xtablestate_ptr_t     get_table_state_by_block(base::xvblock_t * target_block) const = 0;
    virtual data::xtablestate_ptr_t     get_table_connectted_state(common::xaccount_address_t const & table_address) const = 0;
    // TODO(jimmy)
    virtual bool get_receiptid_state_and_prove(common::xaccount_address_t const & table_address,
                                              base::xvblock_t * latest_commit_block,
                                              base::xvproperty_prove_ptr_t & property_prove_ptr,
                                              data::xtablestate_ptr_t & tablestate_ptr) const = 0;

    // block execute
    virtual void on_table_block_committed(base::xvblock_t* block) const = 0;
    virtual uint64_t get_latest_executed_block_height(common::xtable_address_t const & table_address) const = 0;
    virtual uint64_t get_need_sync_state_block_height(common::xtable_address_t const & table_address) const = 0;
    virtual xtablestate_ext_ptr_t do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::map<std::string, base::xaccount_index_t> const& account_index_map, std::error_code & ec) const = 0;
};

class xstatestore_hub_t {
public:
    static xstatestore_face_t* instance();
    static xstatestore_face_t* reset_instance();
private:
    static xstatestore_face_t * _static_statestore;
};

int32_t verify_standby_transaction(data::xtransaction_t const * trx);

NS_END2
