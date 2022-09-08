// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvpropertyprove.h"
#include "xcommon/xaccount_address.h"
#include "xstate_mpt/xstate_mpt.h"

NS_BEG2(top, statestore)

// TODO(jimmy) cache latest commit tablestate
// the statestore interface
class xstatestore_face_t {
 public:
    virtual bool                    start(const xobject_ptr_t<base::xiothread_t> & iothread) = 0;
    // query accountindex
    virtual bool                    get_accountindex_from_latest_connected_table(common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const = 0;
    virtual bool                    get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const = 0;

    // query unitstate
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
    // virtual bool execute_one_table_block(base::xvblock_t * block, std::shared_ptr<state_mpt::xtop_state_mpt> mpt) = 0;
    virtual uint64_t try_update_execute_height(const base::xvaccount_t & target_account, uint64_t max_count) = 0;
    virtual bool execute_table_block(base::xvblock_t * block) = 0;
    // virtual bool execute_table_block(base::xvblock_t * block, evm_common::xh256_t & root_hash) = 0;
};

class xstatestore_hub_t {
public:
    static xstatestore_face_t* instance();
};

NS_END2
