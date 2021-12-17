// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xserializable_based_on.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_to_string_helper.h"
#include "xdata/xchain_param.h"
#include "xdata/xrootblock.h"

#include <map>
#include <string>
#include <vector>

NS_BEG2(top, data)

class xtcc_transaction_t : public xserializable_based_on<void> {
public:
    xtcc_transaction_t() {
#define XADD_ONCHAIN_GOVERNANCE_PARAMETER(NAME)                                                                                                                                    \
    m_initial_values.insert(                                                                                                                                                       \
        {top::config::x##NAME##_onchain_goverance_parameter_t::name,                                                                                                               \
         top::config::xto_string_helper_t<top::config::x##NAME##_onchain_goverance_parameter_t::type>::to_string(top::config::x##NAME##_onchain_goverance_parameter_t::value)})

#define XADD_ONCHAIN_GOVERNANCE_PARAMETER2(NAME, VALUE) m_initial_values.insert({top::config::x##NAME##_onchain_goverance_parameter_t::name, (VALUE)})

        // election:
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(election_rotation_count_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_minimum_rotation_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size);

        XADD_ONCHAIN_GOVERNANCE_PARAMETER(rec_standby_pool_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zec_standby_pool_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation);

        // slash related
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);                                 // default minimun 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_validator_slash_credit);            // validator credit score 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_auditor_slash_credit);              // auditor credit score 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);          // award node persent
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);  // award node vote
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit);                     // validator credit score 0.03;
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit);                       // auditor credit score 0.03
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration);
#if defined(XCHAIN_FORKED_BY_DEFAULT)
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(initial_creditscore);
#endif

        // stake:
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment);

        // consensus
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cpu_gas_exchange_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_max_number);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(application_contract_code_max_len);

        // tcc
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_tcc_proposal_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tcc_proposal_expire_time);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER2(tcc_member, get_tcc_onchain_committee_list());
        // whitelist
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist);
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO)
        XADD_ONCHAIN_GOVERNANCE_PARAMETER2(whitelist, get_genesis_whitelist());
#else
        XADD_ONCHAIN_GOVERNANCE_PARAMETER2(whitelist, "T00000LabjxtsmwfhVW7RK1ezUZNUMdKUv6gxTG5,T00000LZgWW5jsGR4Bg62ZZWbncioWQKDRXtVyJN,T00000LNTsgQq8sGgXmJSNMqEpze9bnqjrTpbk4K,T00000LZtiM94bYBmiC5261Y6MnggQNTjyrFb6Ja,T00000LaPL3pVbkxEfE8wSCanWFtnBVoQaxWkejS,T00000LUkbBh9rPA1RFcbKrJbwj5uQgHK93ADJAW,T00000LcvZaypD3bHFHHh4PsXsy5ASDGAD4mmpFr,T00000LbgaCLnuxnqaqPh8e9EpeNqsRju6MuCr4z,T00000LSmt4xfNdC2v8xQHRSbfSQ7b5aC1UTXWfo,T00000LcM8Pn37SRF5RaTHZLsRW4wudDVE9BAXy8,T00000Lg24i2TweBikod4UJwmMS8TqgwvHooFYMp,T00000LZiwnEtvrRaxEVNoZxU6mp1CFFwvA3JRhQ,T00000LS2WKtMuE7moMekoiYhk78rdFYvJzayMDK,T00000LZkFxsyiz8nJuruAzdtvi8YhZpmL2U9dXK,T00000LMxyqFyLC5ZWhH9AWdgFcK4bbL1kxyw11W,T00000LhXXoXe5KD6furgsEKJRpT3SuZLuN1MaCf,T00000LgspJnjWPFMwqrMF7KHi3rQKQLYp8E1Yxz,T00000LYpGz3V6QJ52SGumo1yWwX7LyxrQqjDLdq,T00000LQnwSvLmPFvjDgVuJNCLGh6WGKwCkf56uj,T00000LTD37o7rLoPFcgpHWjDQxESSxrGvaNrpGt,T00000LL9s2YtuNUcAoUUrnxcSXBd4W6wSdzjemf,T00000LMzNdxFwAxmd2ZVBreLK2vSS49WzCJVoVG,T00000LMvBMjfoqDQC73oSWo5sD67P5xjk9fQaCe,T00000Lbip8pqD91fVpJHLzyVZAQiQCsmbxyEs6y,T00000LYkwwpkJop7HEMVuqbsDZr4RaHjrF1wssP,T00000LPBpGtTM45MNdfAiTjugfRCiS7WtaqVcm1,T00000LgdJEmsZD8obxJCXUHpy3ZcGhtDbwCRH2h,T00000LhHZFAXAch4J43H62GKbXDNfrW34mDb8m9,T00000LQu5vJcL6EGZy2ZzZusUFrtySYYndSZkw3,T00000LT1KJ2acbULwtY8oYSTHFxayDAF7dzLv4V,T00000Lc8DpPBmVciGm74DLUM9gFhEZwBFZu5ddf,T00000LMhzYYjuDKxbgb1etKQd57CP8xcbyb7RG9,T00000LSkemDwk8kFWmXLoasgX6C3ffGRxVPjDLf,T00000LhLwuX3Z5BgKpxQZx6c8dAgJXMKF6CTBst,T00000LWY6p1GfHJQ65NyiXGnEtc51i71ujuGDJQ,T00000LMnPbhyrVSjuZPTibWGWK112sCor6aVcvm,T00000LYE6tEd3hiDuS1r2qdv8TTM1n6pPh8gNqp,T00000Lca54DkBgCy4KGnaNCpwsPEeWKAtCd4ZXH,T00000LZyVYYEM5DZd6sBDpdrLhoEzGgUGEGWFK3,T00000LgeZE4QqTGB3932gVH1mmpAwF27zyv7zDz,T00000LhmH4c2difu4BqTUag9bb4adAPPPMUZU55,T00000LSSiNCEtthd2ZPYSfoXRc8cC1dQh8hTFpm,T00000LWCo6FDr2w7fai9LdvQRAyDhratqubngX6,T00000LT1fiim7qkMnQE3aSFMcJPAjJWNzdCh3bx,T00000LT6H8Cuxr1g5vR3sjHDSEim2hds6PL6kSU,T00000LRW3SKquMLh1vPardd7t13xQzQdUKebqrY,T00000LbcZK2pLDDkdfYUciSkSSkSo8hKsJHzVhv,T00000LMUmS4evMZvVxGCM9kwiaZdbHzY6cv8ccx,T00000Lek4eDGNa5Xgnfj1tPaAnBZBX6AiCwFJFf,T00000LMMzgZNF1P9bBQsxbmPsvj6SnD3M2SkFNo,T00000LQH62BkqCYpu5TBd2DqTLh8j438gdfT4E1,T00000LWuAfQogkTN8tyuKJRoHBDtsmaZaqVhXrS,T00000LUXX3qFKX4bn6fvzGmGnY9usnz8JDUd4Pd,T00000LTVQ6xVhK9o1M5sqJ4AYmjMNjSGWF4f4py");
#endif
        // XADD_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);

        // mainnet node conditions
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_edges);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_archives);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes);

        // xstake contracts
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_stake_votes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_votes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(vote_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(validator_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(edge_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(archive_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(governance_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tx);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_zero_workload);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_zero_workload);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

        XADD_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_collection_interval);

#if defined(XCHAIN_FORKED_BY_DEFAULT)
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);
#endif

        std::vector<std::string> committee_addrs = xrootblock_t::get_tcc_initial_committee_addr();
        for (auto & v : committee_addrs) {
            m_initial_committee.push_back(v);
            xdbg("xtcc_transaction_t initial committee account %s", v.c_str());
        }

#undef XADD_ONCHAIN_GOVERNANCE_PARAMETER
    }

    int32_t do_write(base::xstream_t & stream) const override {
        const int32_t begin_size = stream.size();
        MAP_SERIALIZE_SIMPLE(stream, m_initial_values);
        VECTOR_SERIALIZE_SIMPLE(stream, m_initial_committee);
        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }

    int32_t do_read(base::xstream_t & stream) override {
        const int32_t begin_size = stream.size();
        MAP_DESERIALIZE_SIMPLE(stream, m_initial_values);
        VECTOR_DESERIALZE_SIMPLE(stream, m_initial_committee, std::string);
        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }

public:
    std::string get_genesis_whitelist() const;
    std::string get_tcc_onchain_committee_list() const;
    std::map<std::string, std::string> m_initial_values;
    std::vector<std::string> m_initial_committee;
};
using xtcc_transaction_ptr_t = std::shared_ptr<xtcc_transaction_t>;

NS_END2
