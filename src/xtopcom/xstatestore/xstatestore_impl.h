// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xmbus/xmessage_bus.h"
#include "xstatestore/xstatestore_face.h"
#include "xstatestore/xstatestore_table.h"
#include "xstate_mpt/xstate_mpt.h"

NS_BEG2(top, statestore)

// the statestore interface
class xstatestore_impl_t : public xstatestore_face_t {
 public:
   xstatestore_impl_t();
   virtual ~xstatestore_impl_t() {}

 public:
    virtual bool                    start(const xobject_ptr_t<base::xiothread_t> & iothread) override;
    // query accountindex
    virtual bool                    get_accountindex_from_latest_connected_table(common::xaccount_address_t const & account_address, base::xaccount_index_t & index) const override;
    virtual bool                    get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const override;

    // query unitstate
    virtual data::xunitstate_ptr_t  get_unit_latest_connectted_change_state(common::xaccount_address_t const & account_address) const override;
    virtual data::xunitstate_ptr_t  get_unit_latest_connectted_state(common::xaccount_address_t const & account_address) const override;
    virtual data::xunitstate_ptr_t  get_unit_committed_changed_state(common::xaccount_address_t const & account_address, uint64_t max_height) const override;
    virtual data::xunitstate_ptr_t  get_unit_committed_state(common::xaccount_address_t const & account_address, uint64_t height) const override;
    virtual data::xunitstate_ptr_t  get_unit_state_by_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& index) const override;  // TODO(jimmy)
    virtual data::xunitstate_ptr_t  get_unit_state_by_unit_block(base::xvblock_t * target_block) const override;
    virtual data::xunitstate_ptr_t  get_unit_state_by_table(common::xaccount_address_t const & account_address, const std::string& table_height) const override;
    virtual uint64_t                get_blockchain_height(common::xaccount_address_t const & account_address) const override;
    virtual int32_t                 map_get(common::xaccount_address_t const & account_address, const std::string &key, const std::string &field, std::string &value) const override;
    virtual int32_t                 string_get(common::xaccount_address_t const & account_address, const std::string& key, std::string& value) const override;
    virtual int32_t                 map_copy_get(common::xaccount_address_t const & account_address, const std::string &key, std::map<std::string, std::string> &map) const override;
    virtual int32_t                 get_map_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) const override;
    virtual int32_t                 get_string_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::string &value) const override;

    // query tablestate
    virtual data::xtablestate_ptr_t     get_table_state_by_block(base::xvblock_t * target_block) const override;
    virtual data::xtablestate_ptr_t     get_table_connectted_state(common::xaccount_address_t const & table_address) const override;
    virtual bool get_receiptid_state_and_prove(common::xaccount_address_t const & table_address,
                                              base::xvblock_t * latest_commit_block,
                                              base::xvproperty_prove_ptr_t & property_prove_ptr,
                                              data::xtablestate_ptr_t & tablestate_ptr) const override;
    virtual bool excute_table_block(base::xvblock_t * block, evm_common::xh256_t & root_hash) override;

 private:
    static base::xauto_ptr<base::xvblock_t> get_latest_connectted_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account);
    static base::xauto_ptr<base::xvblock_t> get_committed_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account, uint64_t max_height);   
    void    init_all_tablestate();    
    data::xunitstate_ptr_t       get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * target_block) const;
    base::xvblockstore_t*        get_blockstore() const;
    mbus::xmessage_bus_t *       get_mbus() const;
    xstatestore_table_ptr_t      get_table_statestore_from_unit_addr(common::xaccount_address_t const & account_address) const;
    xstatestore_table_ptr_t      get_table_statestore_from_table_addr(common::xaccount_address_t const & table_address) const;
    data::xtablestate_ptr_t      get_table_state_by_block_internal(common::xaccount_address_t const& table_address, base::xvblock_t * target_block) const;
    void    on_block_to_db_event(mbus::xevent_ptr_t e);

 private:
    std::map<std::string, xstatestore_table_ptr_t> m_table_statestore;
    base::xxtimer_t * m_timer;
    uint32_t m_bus_listen_id;
    bool m_started{false};
};

NS_END2
