// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xdata/xblock.h"

NS_BEG2(top, contract_common)

struct xtop_contract_execution_param {
    common::xlogic_time_t clock{0};
    common::xlogic_time_t timestamp{0};
    common::xlogic_time_t timeofday{0};
    std::string random_seed;
    std::string table_account;
    uint64_t table_commit_height{0};
    uint64_t total_lock_tgas_token{0};

    xtop_contract_execution_param() = default;
    xtop_contract_execution_param(data::xblock_consensus_para_t const & cs_para) {
        clock = cs_para.get_clock();
        timestamp = cs_para.get_timestamp();
        timeofday = cs_para.get_gettimeofday_s();
        random_seed = cs_para.get_random_seed();
        table_account = cs_para.get_table_account();
        table_commit_height = cs_para.get_table_proposal_height() >= 3 ? cs_para.get_table_proposal_height() - 3 : 0;
        total_lock_tgas_token = cs_para.get_total_lock_tgas_token();
    }
};
using xcontract_execution_param_t = xtop_contract_execution_param;

NS_END2
