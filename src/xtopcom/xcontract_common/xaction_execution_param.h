// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xdata/xblock.h"

#include <map>
#include <string>

NS_BEG2(top, contract_common)

struct xtop_action_execution_param {
    common::xlogic_time_t m_clock{0};
    common::xlogic_time_t m_timestamp{0};
    std::string m_random_seed{};
    std::string m_table_account{};
    uint64_t m_table_commit_height{0};
    uint64_t m_total_lock_tgas_token{0};

    xtop_action_execution_param() = default;
    xtop_action_execution_param(const data::xblock_consensus_para_t & cs_para) {
        m_clock = cs_para.get_clock();
        m_timestamp = cs_para.get_timestamp();
        m_random_seed = cs_para.get_random_seed();
        m_table_account = cs_para.get_table_account();
        m_table_commit_height = cs_para.get_table_proposal_height() >= 3 ? cs_para.get_table_proposal_height() - 3 : 0;
        m_total_lock_tgas_token = cs_para.get_total_lock_tgas_token();
    }
};
using xcontract_execution_param_t = xtop_action_execution_param;

NS_END2
