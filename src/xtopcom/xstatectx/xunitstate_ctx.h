// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xdata/xunit_bstate.h"

NS_BEG2(top, statectx)

// unit state context is a wrap unitstate 
class xunitstate_ctx_t {
 public:
    xunitstate_ctx_t(const data::xunitstate_ptr_t & unitstate);
    xunitstate_ctx_t(const data::xunitstate_ptr_t & unitstate, const data::xblock_ptr_t & prev_block);

 public:
    const std::string &                 get_address() const {return m_cur_unitstate->get_account();}
    const data::xunitstate_ptr_t &      get_unitstate() const {return m_cur_unitstate;}
    const data::xblock_ptr_t &          get_prev_block() const {return m_prev_block;} 

 private:
    data::xunitstate_ptr_t              m_cur_unitstate{nullptr};
    data::xblock_ptr_t                  m_prev_block{nullptr};
};
using xunitstate_ctx_ptr_t = std::shared_ptr<xunitstate_ctx_t>;

NS_END2
