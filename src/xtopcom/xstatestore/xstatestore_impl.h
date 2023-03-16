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
#include "xstatestore/xstatestore_resource.h"

NS_BEG2(top, statestore)

class xstatestore_timer_t;
// the statestore interface
class xstatestore_impl_t : public xstatestore_face_t {
 public:
   xstatestore_impl_t();
   virtual ~xstatestore_impl_t() {}

 public:
    virtual bool                    start(const xobject_ptr_t<base::xiothread_t> & iothread, const xobject_ptr_t<base::xiothread_t> & iothread_for_prune) override;
    // query accountindex
    virtual bool                    get_accountindex_from_latest_connected_table(common::xaccount_address_t const & table_address, common::xaccount_address_t const & account_address, base::xaccount_index_t & index) const override;
    virtual bool                    get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const override;
    virtual bool                    get_accountindex(const std::string& table_height, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const override;
    virtual bool                        get_accountindex(xblock_number_t number, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const override;
    virtual data::xaccountstate_ptr_t   get_accountstate(xblock_number_t number, common::xaccount_address_t const & account_address) const override;
    virtual std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> get_all_accountindex(base::xvblock_t * table_block, std::error_code & ec) const override;

    // query unitstate
    virtual data::xunitstate_ptr_t      get_unitstate(xblock_number_t number, common::xaccount_address_t const & account_address) const override;
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
    virtual xtablestate_ext_ptr_t       get_tablestate_ext_from_block(base::xvblock_t * target_block) const override;
    virtual data::xtablestate_ptr_t     get_table_state_by_block(base::xvblock_t * target_block) const override;
    virtual data::xtablestate_ptr_t     get_table_connectted_state(common::xaccount_address_t const & table_address) const override;
    virtual bool get_receiptid_state_and_prove(common::xaccount_address_t const & table_address,
                                              base::xvblock_t * latest_commit_block,
                                              base::xvproperty_prove_ptr_t & property_prove_ptr,
                                              data::xtablestate_ptr_t & tablestate_ptr) const override;
    virtual void on_table_block_committed(base::xvblock_t* block) const override;
    virtual uint64_t get_latest_executed_block_height(common::xtable_address_t const & table_address) const override;
    virtual uint64_t get_need_sync_state_block_height(common::xtable_address_t const & table_address) const override;
    virtual xtablestate_ext_ptr_t do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::map<std::string, base::xaccount_index_t> const& account_index_map, std::error_code & ec) const override;

    // void prune();

 private:
    base::xauto_ptr<base::xvblock_t> get_latest_connectted_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account) const;
    static base::xauto_ptr<base::xvblock_t> get_committed_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account, uint64_t max_height);   
    void    init_all_tablestate();    
    data::xunitstate_ptr_t       get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * target_block) const;
    base::xvblockstore_t*        get_blockstore() const;
    mbus::xmessage_bus_t *       get_mbus() const;
    xstatestore_table_ptr_t      get_table_statestore_from_unit_addr(common::xaccount_address_t const & account_address) const;
    xstatestore_table_ptr_t      get_table_statestore_from_table_addr(std::string const & table_addr) const;
    void                         on_block_to_db_event(mbus::xevent_ptr_t e);
    void                         on_state_sync_event(mbus::xevent_ptr_t e);
    void                         on_state_sync_result(mbus::xevent_state_sync_ptr_t state_sync_event);
    common::xnode_type_t         get_node_type() const;
    bool                         is_archive_node() const;
    void                         on_table_block_committed(const mbus::xevent_ptr_t & event) const;
    xtablestate_ext_ptr_t        get_tablestate_ext_from_block_inner(base::xvblock_t* target_block, bool bstate_must) const;

private:
    std::map<std::string, xstatestore_table_ptr_t> m_table_statestore;
    xstatestore_base_t  m_store_base;
    xstatestore_timer_t * m_timer;
    uint32_t m_store_block_listen_id;
    uint32_t m_state_sync_listen_id;
    bool m_started{false};
    xobject_ptr_t<statestore_prune_dispatcher_t> m_prune_dispather{nullptr};
    std::shared_ptr<xstatestore_resources_t> m_para;
};

class xstatestore_timer_t : public top::base::xxtimer_t {
public:
    xstatestore_timer_t(base::xcontext_t & _context, int32_t timer_thread_id, xstatestore_impl_t * statestore)
      : base::xxtimer_t(_context, timer_thread_id), m_statestore(statestore) {
    }

protected:
    ~xstatestore_timer_t() override {
    }

protected:
    bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) override;

private:
    observer_ptr<xstatestore_impl_t> m_statestore;
};

NS_END2
