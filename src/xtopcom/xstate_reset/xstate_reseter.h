// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xdata/xcons_transaction.h"
#include "xstatectx/xstatectx_face.h"
#include "xvledger/xvaccount.h"

NS_BEG2(top, state_reset)

class xstate_reseter {
public:
    // what we need:
    // 1. current table statectx ptr ✔
    // 2. current table id account ✔
    // 3. current timeclock block height (might not need, if we put fork logic outside..., better to be inside?) ✔
    // 4. origin_tx list reference ( we only need to check if txs contains constract's self on_timer. ) ✔
    // 5. fork_info contract properties. ( use some read properties means in .cpp ) ✔
    // 6. reset data ()
    xstate_reseter(statectx::xstatectx_face_ptr_t statectx_ptr, std::shared_ptr<std::vector<data::xcons_transaction_ptr_t>> txs_ptr, uint64_t current_time_block_height);

    xstate_reseter(const xstate_reseter &) = delete;
    xstate_reseter(xstate_reseter &&) = delete;
    ~xstate_reseter() = default;

    bool exec_reset();

private:
    bool check_tx_contains_contract();

private:
    bool m_need_fork{true};  // TODO set false for normal case. to make performance influences minimum.
    statectx::xstatectx_face_ptr_t m_statectx_ptr;
    std::shared_ptr<std::vector<data::xcons_transaction_ptr_t>> m_txs_ptr;
    base::xvaccount_t m_table_account;  // todo use xtable_address_t
    std::string m_corresponse_contract_address;
    uint64_t m_current_time_block_height;
};

NS_END2