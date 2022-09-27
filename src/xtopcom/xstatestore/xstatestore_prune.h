// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaccount_address.h"
#include "xdata/xtable_bstate.h"
#include "xstatestore/xstatestore_base.h"

#include <string>

NS_BEG2(top, statestore)

class xstatestore_prune_t {
public:
    xstatestore_prune_t(common::xaccount_address_t const & table_addr);

public:
    void on_table_mpt_and_unitstate_executed(uint64_t table_block_height);
    void prune(uint64_t latest_executed_height);

private:
    void init();
    uint64_t prune_by_height_section(uint64_t from_height, uint64_t to_height);
    void prune_imp(const std::shared_ptr<state_mpt::xtop_state_mpt> & mpt, base::xvblock_t* next_block);

private:
    mutable std::mutex m_prune_lock;
    common::xaccount_address_t m_table_addr;
    uint64_t m_pruned_height{0};
    xstatestore_base_t m_statestore_base;
};

NS_END2
