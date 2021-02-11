// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xbasic/xns_macro.h"
#include "xbasic/xlru_cache.h"
#include "xvm_trace.h"
#include "xlua_engine.h"
#include "xvm_native_func.h"
#include "xstore/xaccount_context.h"
NS_BEG2(top, xvm)
using data::xtransaction_t;
using store::xaccount_context_t;
using store::xstore_face_t;
using basic::xlru_cache;

class xvm_service {
 public:
    xvm_service();
    //~xvm_service();
    xtransaction_trace_ptr deal_transaction(const data::xtransaction_ptr_t& trx, xaccount_context_t* account_context);
    native_handler* get_native_handler(string action_name);
 public:
    xlru_cache<common::xaccount_address_t, shared_ptr<xengine>> m_vm_cache;
    xvm_native_func                         m_native_func;
    //todo db
    //todo config
};
NS_END2
