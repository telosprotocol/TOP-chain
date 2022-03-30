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
#include "xdata/xunit_bstate.h"

NS_BEG2(top, statectx)

class xstatectx_para_t {  // TODO(jimmy)
 public:
    xstatectx_para_t(const xstatectx_para_t & r)
    : m_clock(r.m_clock), m_random_seed(r.m_random_seed), m_total_lock_tgas_token(r.m_total_lock_tgas_token) {
    }
    xstatectx_para_t(uint64_t clock, const std::string & random_seed, uint64_t tgas_lock)
    : m_clock(clock), m_random_seed(random_seed), m_total_lock_tgas_token(tgas_lock) {
    }

 public:
    uint64_t        get_timestamp() const {return (uint64_t)(m_clock * 10) + base::TOP_BEGIN_GMTIME;}

 public:
    uint64_t        m_clock{0};
    std::string     m_random_seed;
    uint64_t        m_total_lock_tgas_token{0};
};

// the table world state context
class xstatectx_face_t {
 public:
    virtual const data::xtablestate_ptr_t &     get_table_state() const = 0;
    virtual data::xunitstate_ptr_t  load_unit_state(const base::xvaccount_t & addr) = 0;
    virtual const xstatectx_para_t & get_ctx_para() const = 0;
    virtual bool                    do_rollback() = 0;
    virtual size_t                  do_snapshot() = 0;
    virtual const std::string &     get_table_address() const = 0;
    virtual bool                    is_state_dirty() const = 0;
};
using xstatectx_face_ptr_t = std::shared_ptr<xstatectx_face_t>;

NS_END2
