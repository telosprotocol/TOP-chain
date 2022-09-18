// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xcommon/xaccount_address.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xtablestate_ext.h"

NS_BEG2(top, statestore)

class xstatestore_executor_t {
public:
    static constexpr uint32_t               execute_demand_limit{10};
    static constexpr uint32_t               execute_update_limit{32};

public:
    xstatestore_executor_t(common::xaccount_address_t const& table_addr);

public:
    void    execute_and_get_tablestate_ext(base::xvblock_t* target_block, xtablestate_ext_ptr_t & tablestate_ext, std::error_code & ec) const;
    void    execute_and_get_accountindex(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const;
    void    execute_and_get_tablestate(base::xvblock_t* block, data::xtablestate_ptr_t &tablestate, std::error_code & ec) const;
    void    on_table_block_committed(base::xvblock_t* block) const;

protected:
    void    update_execute_from_execute_height() const;
    void    try_execute_block_on_demand(base::xvblock_t* block, std::error_code & ec) const;
    void    get_accountindex_from_block(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const;
    void    execute_block_recursive(base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const;
    void    execute_block_with_prev(base::xvblock_t* block, xobject_ptr_t<base::xvblock_t> const& prev_block, std::error_code & ec) const;
    void    set_latest_executed_info(uint64_t height,const std::string & blockhash) const;
    uint64_t    get_latest_executed_block_height() const;

private:
    mutable std::mutex          m_execute_lock;
    mutable uint64_t            m_executed_height{0};
    common::xaccount_address_t  m_table_addr;
    xstatestore_base_t          m_statestore_base;
};

NS_END2
