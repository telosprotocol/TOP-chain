// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xdata/xtable_bstate.h"

NS_BEG2(top, statectx)

class xstatectx_base_t {
 public:
    xstatectx_base_t(const data::xtablestate_ptr_t & prev_table_state,const data::xtablestate_ptr_t & commit_table_state, uint64_t clock);

 public:
    static xobject_ptr_t<base::xvbstate_t> create_proposal_bstate(base::xvblock_t* prev_block, base::xvbstate_t* prev_bstate, uint64_t clock);
 public:
    xobject_ptr_t<base::xvbstate_t> load_inner_table_unit_state(const base::xvaccount_t & addr) const;
    xobject_ptr_t<base::xvbstate_t> load_different_table_unit_state(const base::xvaccount_t & addr) const;
    data::xblock_ptr_t              load_inner_table_unit_block(const base::xvaccount_t & addr) const;
    xobject_ptr_t<base::xvbstate_t> load_proposal_block_state(const base::xvaccount_t & addr, base::xvblock_t* prev_block) const;
 private:
    base::xvblockstore_t*       get_blockstore() const;
    base::xvblkstatestore_t*    get_xblkstatestore() const;
    void                        sync_unit_block(const base::xvaccount_t & _vaddr, uint64_t end_height) const;

 private:
    data::xtablestate_ptr_t m_table_state{nullptr};
    data::xtablestate_ptr_t m_commit_table_state{nullptr};
    uint64_t    m_clock{0};
};


NS_END2
