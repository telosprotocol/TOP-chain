// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xcommon/xaccount_address.h"
#include "xstatestore/xstatestore_face.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xtablestate_ext.h"
#include "xstatestore/xstatestore_access.h"

NS_BEG2(top, statestore)

class xexecute_listener_face_t {
public:
    virtual void on_executed(uint64_t height) = 0;
};

class xstatestore_executor_t {
public:
    static constexpr uint32_t               execute_update_limit{32};
    static constexpr uint32_t               execute_unit_limit_demand{100};  // execute unit for unitstate on demand to fullunit
    static std::mutex   m_global_execute_lock;

public:
    xstatestore_executor_t(common::xaccount_address_t const& table_addr, xexecute_listener_face_t * execute_listener);
    void    init();

public:
    xtablestate_ext_ptr_t   execute_and_get_tablestate_ext(base::xvblock_t* target_block, std::error_code & ec) const;
    xtablestate_ext_ptr_t   get_latest_executed_tablestate_ext() const;
    xtablestate_ext_ptr_t   do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const;
    void                    on_table_block_committed(base::xvblock_t* block) const;
    void                    raise_execute_height(const xstate_sync_info_t & sync_info);

    void    execute_and_get_accountindex(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const;
    void    execute_and_get_tablestate(base::xvblock_t* block, data::xtablestate_ptr_t &tablestate, std::error_code & ec) const;

    void    build_unitstate_by_accountindex(common::xaccount_address_t const& unit_addr, base::xaccount_index_t const& account_index, data::xunitstate_ptr_t &unitstate, std::error_code & ec) const;
    void    build_unitstate_by_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t* unit, data::xunitstate_ptr_t &unitstate, std::error_code & ec) const;  

    uint64_t get_latest_executed_block_height() const;
    uint64_t get_need_sync_state_block_height() const;

protected:
    uint64_t update_execute_from_execute_height(uint64_t old_execute_height) const;
    void    set_latest_executed_info(uint64_t height,const std::string & blockhash) const;
    void    set_need_sync_state_block_height(uint64_t height) const;
    void    update_latest_executed_info(base::xvblock_t* block) const;
    void    recover_execute_height(uint64_t old_executed_height);
    bool    need_store_unitstate() const;
    xtablestate_ext_ptr_t write_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const;

    data::xunitstate_ptr_t make_state_from_current_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t * current_block, std::error_code & ec) const;
    data::xunitstate_ptr_t make_state_from_prev_state_and_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t * current_block, data::xunitstate_ptr_t const& prev_bstate, std::error_code & ec) const;
    data::xunitstate_ptr_t execute_unit_recursive(common::xaccount_address_t const& unit_addr, base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const;

    xtablestate_ext_ptr_t  make_state_from_current_table(base::xvblock_t* current_block, std::error_code & ec) const;
    xtablestate_ext_ptr_t  make_state_from_prev_state_and_table(base::xvblock_t* current_block, xtablestate_ext_ptr_t const& prev_state, std::error_code & ec) const;
    xtablestate_ext_ptr_t  execute_block_recursive(base::xvblock_t* current_block, uint32_t & limit, std::error_code & ec) const;
    xtablestate_ext_ptr_t execute_and_get_tablestate_ext_unlock(base::xvblock_t* block, std::error_code & ec) const;

protected:
    mutable std::mutex          m_execute_lock;  // protect the whole execution
    mutable std::mutex          m_execute_height_lock;
    mutable uint64_t            m_executed_height{0};
    mutable uint64_t            m_need_all_state_sync_height{0};
    common::xaccount_address_t  m_table_addr;
    xstatestore_base_t          m_statestore_base;
    xstatestore_accessor_t      m_state_accessor;
    xexecute_listener_face_t *  m_execute_listener{nullptr};
};

NS_END2
