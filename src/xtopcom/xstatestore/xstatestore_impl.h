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

NS_BEG2(top, statestore)

class xstatestore_timer_t;
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
    virtual bool execute_table_block(base::xvblock_t * block) override;
    virtual void update_node_type(common::xnode_type_t combined_node_type) override;
    void try_update_tables_execute_height();

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
    bool get_mpt(base::xvblock_t * block, xhash256_t & root_hash, std::shared_ptr<state_mpt::xtop_state_mpt> & mpt);
    void set_latest_executed_info(const base::xvaccount_t & target_account, uint64_t height,const std::string & blockhash);
    uint64_t get_latest_executed_block_height(const base::xvaccount_t & target_account);
    bool execute_block_recurse(base::xvblock_t * block, const xhash256_t root_hash, uint64_t min_height);
    bool set_and_commit_mpt(base::xvblock_t * block, const xhash256_t root_hash, std::shared_ptr<state_mpt::xtop_state_mpt> pre_mpt, bool & mpt_committed);
    uint64_t try_update_execute_height(const base::xvaccount_t & target_account, uint64_t max_count);
    void push_try_execute_table(std::string table_addr);
    void pop_try_execute_tables(std::set<std::string> & try_execute_tables);
    common::xnode_type_t get_node_type() const;
    bool is_archive_node() const;

private:
    std::map<std::string, xstatestore_table_ptr_t> m_table_statestore;
    xstatestore_timer_t * m_timer;
    uint32_t m_bus_listen_id;
    common::xnode_type_t m_combined_node_type{common::xnode_type_t::invalid};
    std::set<std::string> m_try_execute_tables;
    mutable std::mutex m_mutex;
    bool m_started{false};
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
