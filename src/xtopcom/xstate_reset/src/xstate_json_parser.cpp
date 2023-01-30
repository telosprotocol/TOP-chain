// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_json_parser.h"

#include "xstate_reset/reset_point_data/v10902_table_tickets_reset.inc"

NS_BEG2(top, state_reset)

xstate_json_parser::xstate_json_parser(base::xvaccount_t const & table_account, std::string const & fork_name) : m_table_account{table_account}, m_fork_name{fork_name} {
#define CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, num)                                                                                                                              \
    case num: {                                                                                                                                                                    \
        m_json_data = json::parse(FORK_NAME##_Ta0000_##num);                                                                                                                       \
        if (!m_json_data.empty()) {                                                                                                                                                \
            assert(m_json_data.find(m_table_account.get_account()) != m_json_data.end());                                                                                          \
            m_json_data = m_json_data.at(m_table_account.get_account());                                                                                                           \
        }                                                                                                                                                                          \
        break;                                                                                                                                                                     \
    }

    // CONCAT_SHARDING_VARIABLE_NAME(TEST_FORK, 0);

#define SHARDING_SWITCH_CASE_64(FORK_NAME)                                                                                                                                         \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 0);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 1);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 2);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 3);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 4);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 5);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 6);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 7);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 8);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 9);                                                                                                                                   \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 10);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 11);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 12);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 13);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 14);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 15);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 16);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 17);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 18);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 19);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 20);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 21);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 22);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 23);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 24);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 25);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 26);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 27);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 28);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 29);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 30);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 31);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 32);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 33);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 34);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 35);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 36);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 37);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 38);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 39);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 40);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 41);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 42);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 43);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 44);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 45);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 46);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 47);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 48);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 49);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 50);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 51);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 52);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 53);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 54);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 55);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 56);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 57);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 58);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 59);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 60);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 61);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 62);                                                                                                                                  \
    CONCAT_SHARDING_VARIABLE_NAME(FORK_NAME, 63);

#define ADD_ONE_FORK(FORK_NAME)                                                                                                                                                    \
    switch (m_table_account.get_zone_index()) {                                                                                                                                    \
    case base::enum_chain_zone_consensus_index: {                                                                                                                                  \
        auto table_id = static_cast<uint16_t>(m_table_account.get_ledger_subaddr());                                                                                               \
        switch (table_id) {                                                                                                                                                        \
            SHARDING_SWITCH_CASE_64(FORK_NAME)                                                                                                                                     \
        default:                                                                                                                                                                   \
            xerror("unknonw table_id %u", table_id);                                                                                                                               \
        }                                                                                                                                                                          \
        break;                                                                                                                                                                     \
    }                                                                                                                                                                              \
    case base::enum_chain_zone_evm_index: {                                                                                                                                        \
        m_json_data = json::parse(FORK_NAME##_Ta0004_0);                                                                                                                           \
        if (!m_json_data.empty()) {                                                                                                                                                \
            assert(m_json_data.find(m_table_account.get_account()) != m_json_data.end());                                                                                          \
            m_json_data = m_json_data.at(m_table_account.get_account());                                                                                                           \
        }                                                                                                                                                                          \
        break;                                                                                                                                                                     \
    }                                                                                                                                                                              \
    default: {                                                                                                                                                                     \
        xerror("not support this table for now : %s", m_table_account.get_address().c_str());                                                                                      \
        break;                                                                                                                                                                     \
    }                                                                                                                                                                              \
    }

    // if (m_fork_name == "TEST_FORK") {
    //     ADD_ONE_FORK(TEST_FORK);
        // } else if (m_fork_name == "xxx") {
        //     ADD_ONE_FORK(xxx);
    // } else {
    //     xerror("not support this fork :%s", m_fork_name.c_str());
    // }
    if (m_fork_name == "v10902_table_tickets_reset") {
        ADD_ONE_FORK(v10902_table_tickets_reset)
    }

#undef CONCAT_SHARDING_VARIABLE_NAME
#undef SHARDING_SWITCH_CASE_64
#undef ADD_ONE_FORK
}

NS_END2