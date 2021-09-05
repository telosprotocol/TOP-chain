// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "xtxexecutor/xunit_service_error.h"

#include "xdata/xaction_parse.h"
#include "xdata/xdata_defines.h"
#include "xdata/xchain_param.h"
#include "xdata/xlightunit.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xaccount_context.h"
#include "xvm/xvm_service.h"
#include "xbasic/xmodule_type.h"
#include "xbasic/xutility.h"
#include "xcrypto/xckey.h"
#include "xbase/xmem.h"
#include "xstore/xstore_error.h"
#include "xdata/xgenesis_data.h"
#include "xverifier/xtx_verifier.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xtxexecutor/xtransaction_context.h"

NS_BEG2(top, txexecutor)

using store::xaccount_context_t;
using data::xtransaction_result_t;
using data::xcons_transaction_ptr_t;

class xtransaction_prepare_t{
 public:
    xtransaction_prepare_t(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : m_account_ctx(account_ctx), m_trans(trans) {
        assert(trans != nullptr);
    }

    int32_t check();
    int32_t parse();
    //int32_t exec();
    std::string get_err_msg(const int32_t type);

 private:
    int32_t source_action_exec();
    int32_t target_action_exec();
    int32_t source_confirm_action_exec();

    std::shared_ptr<xtransaction_face_t> m_trans_obj{};
    xaccount_context_t* m_account_ctx;
    data::xcons_transaction_ptr_t m_trans;
};




NS_END2