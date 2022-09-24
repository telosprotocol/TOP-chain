// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xbase/xlru_cache.h"
#include "xcommon/xaccount_address.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xstatestore_exec.h"
#include "xstatestore/xtablestate_ext.h"
#include "xstatestore/xstatestore_access.h"

NS_BEG2(top, statestore)

class xstatestore_table_t {
public:
    xstatestore_table_t(common::xaccount_address_t const&  table_addr);

public:
    common::xaccount_address_t const &  get_table_address() const {return m_table_addr;}

    xtablestate_ext_ptr_t   get_tablestate_ext_from_block(base::xvblock_t* target_block) const;
    bool                    get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const;
    void                    on_table_block_committed(base::xvblock_t* block) const;
    xtablestate_ext_ptr_t   do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const;

    data::xunitstate_ptr_t  get_unit_state_from_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& index) const;
    data::xunitstate_ptr_t  get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * unit_block) const;
    data::xunitstate_ptr_t  get_unit_state_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block) const;

    data::xtablestate_ptr_t get_latest_connectted_table_state() const;

    uint64_t                get_latest_executed_block_height() const;
    void                    raise_execute_height(const xstate_sync_info_t & sync_info);

private:
    
    void    init_cache();
private:
    common::xaccount_address_t  m_table_addr;
    xstatestore_cache_t         m_table_cache;
    xstatestore_executor_t      m_table_executor;
    xstatestore_base_t          m_store_base;
};
using xstatestore_table_ptr_t = std::shared_ptr<xstatestore_table_t>;

NS_END2
