// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xstatectx/xunitstate_ctx.h"

NS_BEG2(top, statectx)

class xstatectx_para_t {
 public:
    xstatectx_para_t(const xstatectx_para_t & r)
    : m_clock(r.m_clock) {
    }
    xstatectx_para_t(uint64_t clock)
    : m_clock(clock) {
    }

 public:
    uint64_t        m_clock{0};
};

// the table world state context
class xstatectx_face_t {
 public:
    virtual const data::xtablestate_ptr_t &     get_table_state() const = 0;
    virtual data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) = 0;
    virtual data::xunitstate_ptr_t  load_unit_state(common::xaccount_address_t const& address) = 0;
    virtual data::xunitstate_ptr_t  load_commit_unit_state(common::xaccount_address_t const& address) {return nullptr;}
    virtual data::xunitstate_ptr_t  load_commit_unit_state(common::xaccount_address_t const& address, uint64_t height) {return nullptr;}
    virtual uint64_t                load_account_height(common::xaccount_address_t const& address) {return 0;}
    virtual bool                    do_rollback() = 0;
    virtual size_t                  do_snapshot() = 0;
    virtual void                    finish_execution() {}; // change statectx to unchanged status
    virtual void                    do_commit(base::xvblock_t* current_block) {return;}  // TODO(jimmy) do commit changed state to db
    virtual std::string             get_table_address() const = 0;
    virtual bool                    is_state_dirty() const = 0;
    virtual std::map<std::string, xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const = 0;    

    virtual ~xstatectx_face_t() = default;
};
using xstatectx_face_ptr_t = std::shared_ptr<xstatectx_face_t>;

NS_END2
