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
    xunitstate_ctx_t(data::xunitstate_ptr_t const& unitstate, base::xaccount_index_t const& accoutindex);
    xunitstate_ctx_t(data::xaccountstate_ptr_t const& accountstate) 
    : m_accountstate(accountstate) {}    

 public:
    // common::xaccount_address_t const&   get_address() const {return m_cur_unitstate->account_address();}
    const data::xunitstate_ptr_t &      get_unitstate() const {return m_accountstate->get_unitstate();}
    std::string const&                  get_unit_hash() const{return m_accountstate->get_unit_hash();}
    data::xaccountstate_ptr_t const&    get_accoutstate() const {return m_accountstate;}

 private: 
    data::xaccountstate_ptr_t           m_accountstate{nullptr};
};
using xunitstate_ctx_ptr_t = std::shared_ptr<xunitstate_ctx_t>;

NS_END2
