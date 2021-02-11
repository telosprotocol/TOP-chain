// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <chrono>
#include <cstdint>
#include "xbasic/xns_macro.h"
#include "xdata/xtransaction.h"
#include "xvm_define.h"
#include "xvm_service.h"
#include "xcontract_helper.h"

NS_BEG2(top, xvm)
using top::data::xtransaction_t;
using top::data::xaction_t;

class xtransaction_context;

class xvm_context {
public:
    xvm_context(xvm_service& vm_service, const data::xtransaction_ptr_t& trx, xaccount_context_t* account_context, xtransaction_trace_ptr trace_ptr);
    void exec();
    void publish_code();
public:
    xvm_service&                m_vm_service;
    const xaction_t&            m_current_action;
    common::xaccount_address_t  m_parent_account;
    common::xaccount_address_t  m_contract_account;
    const std::string           m_exec_account;
    shared_ptr<xcontract_helper> m_contract_helper;
    xtransaction_trace_ptr      m_trace_ptr;

private:
    std::string get_parent_address();
};

NS_END2
