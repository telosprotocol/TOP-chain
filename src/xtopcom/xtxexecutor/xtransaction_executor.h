// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xdata/xtransaction.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xaccount_context.h"

NS_BEG2(top, txexecutor)

using store::xaccount_context_t;
using store::xtransaction_result_t;
using data::xcons_transaction_ptr_t;

struct xbatch_txs_result_t {
    xtransaction_result_t                   succ_txs_result;
    std::vector<xcons_transaction_ptr_t>    m_exec_succ_txs;
    std::vector<xcons_transaction_ptr_t>    m_exec_fail_txs;
};

class xtransaction_executor {
 public:
    static int32_t exec_batch_txs(xaccount_context_t * account_context, const std::vector<xcons_transaction_ptr_t> & txs,
        xbatch_txs_result_t & txs_result);

 private:
    static int32_t exec_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx);
    static int32_t exec_one_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx);
};

NS_END2
