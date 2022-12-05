// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"

#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xchain_fork/xutility.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xcommon/xrole_type.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"

using top::base::xcontext_t;
using top::base::xstream_t;
using top::base::xstring_utl;

NS_BEG2(top, xstake)

xtable_vote_contract::xtable_vote_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtable_vote_contract::setup() {
    const int old_tables_count = 256;
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xtable_vote_contract::setup] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s\n", getpid(), SELF_ADDRESS().to_string().c_str());
        return;
    }

    // vote related
    std::map<std::string, uint64_t> adv_get_votes_detail;
    for (auto i = 1; i <= data::system_contract::XPROPERTY_SPLITED_NUM; i++) {
        std::string property;
        property = property + data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(i);
        MAP_CREATE(property);
        {
            std::map<std::string, std::map<std::string, uint64_t>> votes_detail;
            for (auto j = 0; j < old_tables_count; j++) {
                auto table_addr = std::string{sys_contract_sharding_vote_addr} + "@" + base::xstring_utl::tostring(j);
                std::vector<std::pair<std::string, std::string>> db_kv_112;
                chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{table_addr}, property, db_kv_112);
                for (auto const & _p : db_kv_112) {
                    base::xvaccount_t vaccount{_p.first};
                    auto account_table_id = vaccount.get_ledger_subaddr();
                    if (static_cast<uint16_t>(account_table_id) != static_cast<uint16_t>(table_id)) {
                        continue;
                    }
                    std::map<std::string, uint64_t> votes;
                    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)_p.second.c_str(), (uint32_t)_p.second.size());
                    stream >> votes;
                    for (auto const & vote : votes) {
                        if (votes_detail[_p.first].count(vote.first)) {
                            votes_detail[_p.first][vote.first] += vote.second;
                        } else {
                            votes_detail[_p.first][vote.first] = vote.second;
                        }
                    }
                }
            }
            for (auto const & vote_detail : votes_detail) {
                for (auto const & adv_get_votes : vote_detail.second) {
                    if (adv_get_votes_detail.count(adv_get_votes.first)) {
                        adv_get_votes_detail[adv_get_votes.first] += adv_get_votes.second;
                    } else {
                        adv_get_votes_detail[adv_get_votes.first] = adv_get_votes.second;
                    }
                }
                xstream_t stream(xcontext_t::instance());
                stream << vote_detail.second;
                std::string vote_info_str = std::string((char *)stream.data(), stream.size());
                MAP_SET(property, vote_detail.first, vote_info_str);
            }
        }
    }

    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY);
    {
        for (auto const & adv_get_votes : adv_get_votes_detail) {
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_get_votes.first, base::xstring_utl::tostring(adv_get_votes.second));
        }
    }

    STRING_CREATE(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY);
}

// vote related
void xtable_vote_contract::voteNode(vote_info_map_t const & vote_info) {
    XMETRICS_TIME_RECORD("sysContract_tableVote_vote_node");
    auto const timestamp = TIME();
    auto const & account = common::xaccount_address_t{SOURCE_ADDRESS()};
    xinfo("[xtable_vote_contract::voteNode] timer round: %" PRIu64 ", src_addr: %s, self addr: %s, pid: %d\n",
          timestamp,
          account.to_string().c_str(),
          SELF_ADDRESS().to_string().c_str(),
          getpid());

    XCONTRACT_ENSURE(common::is_t0(account) || common::is_t8(account), "only T0 or T8 account can vote for an account");

    const data::xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(trans_ptr->get_tx_type() == data::xtransaction_type_vote,
                     "xtable_vote_contract::voteNode: transaction_type must be xtransaction_type_vote");
    XMETRICS_PACKET_INFO("sysContract_tableVote_vote_node", "timer round", std::to_string(timestamp), "voter address", account.to_string());

    if (chain_fork::xutility_t::is_forked(fork_points::v1_9_0_vote_contract_update_point, timestamp)) {
        xinfo("xtable_vote_contract::voteNode use new version to set vote");
        set_vote_info_v2(account, vote_info, true);
    } else {
        xinfo("xtable_vote_contract::voteNode not forked yet, %lu, use old version to set vote", timestamp);
        set_vote_info(account, vote_info, true);
    }
}

void xtable_vote_contract::unvoteNode(vote_info_map_t const & vote_info) {
    XMETRICS_TIME_RECORD("sysContract_tableVote_unvote_node");
    auto const timestamp = TIME();
    auto const & account = common::xaccount_address_t{SOURCE_ADDRESS()};
    xinfo("[xtable_vote_contract::unvoteNode] timer round: %" PRIu64 ", src_addr: %s, self addr: %s", timestamp, account.to_string().c_str(), SELF_ADDRESS().to_string().c_str());

    XCONTRACT_ENSURE(common::is_t0(account) || common::is_t8(account), "only T0 or T8 can withdraw votes from an account");

    const data::xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(trans_ptr->get_tx_type() == data::xtransaction_type_abolish_vote,
                     "xtable_vote_contract::unvoteNode: transaction_type must be xtransaction_type_abolish_vote");
    XMETRICS_PACKET_INFO("sysContract_tableVote_unvote_node", "timer round", std::to_string(timestamp), "unvoter address", account.to_string());

    
    if (chain_fork::xutility_t::is_forked(fork_points::v1_9_0_vote_contract_update_point, timestamp)) {
        xinfo("xtable_vote_contract::unvoteNode use new version to set vote");
        set_vote_info_v2(account, vote_info, false);
    } else {
        xinfo("xtable_vote_contract::unvoteNode not forked yet, %lu, use old version to set vote", timestamp);
        set_vote_info(account, vote_info, false);
    }
}

void xtable_vote_contract::set_vote_info(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote) {
    // votes process
    handle_votes(account, vote_info, b_vote);
    // check update time interval
    if (!is_expire(TIME())) {
        xdbg("[xtable_vote_contract::set_vote_info]  is not expire pid: %d, b_vote: %d\n", getpid(), b_vote);
        return;
    }
    // call other contracts
    commit_stake();
    commit_total_votes_num();
}

void xtable_vote_contract::commit_stake() {
    std::map<std::string, std::string> adv_votes;

    try {
        XMETRICS_TIME_RECORD("sysContract_tableVote_get_property_contract_pollable_key");
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
    } catch (std::runtime_error & e) {
        xdbg("[update_adv_votes] MAP COPY GET error:%s", e.what());
        throw;
    }

    uint64_t timer = TIME();
    xinfo("[xtable_vote_contract::commit_stake] split table vote trx %" PRIu64, timer);
    split_and_report(sys_contract_rec_registration_addr, "update_batch_stake_v2", adv_votes);
}

void xtable_vote_contract::set_vote_info_v2(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote) {
    auto all_time_ineffective_votes = get_all_time_ineffective_votes(account);
    if (!b_vote) {
        auto vote_info_to_del = vote_info;
        del_all_time_ineffective_votes(vote_info_to_del, all_time_ineffective_votes);
        if (!vote_info_to_del.empty()) {
            handle_votes(account, vote_info_to_del, b_vote);
        }
        STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, xstring_utl::tostring(1));
    } else {
        add_all_time_ineffective_votes(TIME(), vote_info, all_time_ineffective_votes);
    }
    set_all_time_ineffective_votes(account, all_time_ineffective_votes);
}

int32_t xtable_vote_contract::get_node_info(const common::xaccount_address_t & account, data::system_contract::xreg_node_info & reg_node_info) {
    xdbg("[xtable_vote_contract::get_node_info] node account: %s, pid: %d\n", account.to_string().c_str(), getpid());

    std::string reg_node_str;
    int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, account.to_string(), reg_node_str, sys_contract_rec_registration_addr);
    if (ret || reg_node_str.empty()) {
        return -1;
    }
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)reg_node_str.data(), reg_node_str.size());
    if (stream.size() > 0) {
        reg_node_info.serialize_from(stream);
    }

    return 0;
}

std::map<std::string, uint64_t> xtable_vote_contract::get_table_votes_detail(common::xaccount_address_t const & account){
    std::map<std::string, uint64_t> votes_table;
    std::string vote_info_str;
    uint32_t sub_map_no = (utl::xxh32_t::digest(account.to_string()) % data::system_contract::XPROPERTY_SPLITED_NUM) + 1;
    std::string property = std::string{data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE} + "-" + std::to_string(sub_map_no);
    {
        XMETRICS_TIME_RECORD("sysContract_tableVote_get_property_contract_votes_key");
        std::string vote_info_str;
        // here if not success, means account has no vote info yet, so vote_info_str is empty, using above default votes_table directly
        if (MAP_GET2(property, account.to_string(), vote_info_str)){
            xwarn("[xtable_vote_contract::handle_votes] get property empty, account %s", account.to_string().c_str());
        }
        if (!vote_info_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_info_str.c_str(), (uint32_t)vote_info_str.size());
            stream >> votes_table;
        }
    }
    return votes_table;
}

void xtable_vote_contract::handle_votes(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote) {
    std::map<std::string, uint64_t> votes_table = get_table_votes_detail(account);

    auto pid = getpid();
    for (auto const & entity : vote_info) {
        auto const & adv_account = entity.first;
        auto const & votes = entity.second;

        xinfo("[xtable_vote_contract::handle_votes] b_vote: %d, voter: %s, node: %s, votes: %u, pid: %d\n", b_vote, account.to_string().c_str(), adv_account.c_str(), votes, pid);
        common::xaccount_address_t address{adv_account};
        if(b_vote){
            data::system_contract::xreg_node_info node_info;
            auto ret = get_node_info(address, node_info);
            XCONTRACT_ENSURE(ret == 0, "xtable_vote_contract::handle_votes: node not exist");
            XCONTRACT_ENSURE(node_info.has<common::xminer_type_t::advance>(), "xtable_vote_contract::handle_votes: only auditor can be voted");
        }
        uint64_t node_total_votes = get_advance_tickets(address);
        calc_advance_tickets(address, votes, votes_table, b_vote, node_total_votes);
        add_advance_tickets(address, node_total_votes);
    }
    update_table_votes_detail(account, votes_table);
}

void xtable_vote_contract::calc_advance_tickets(common::xaccount_address_t const & adv_account, uint64_t votes, std::map<std::string, uint64_t> & votes_table, bool b_vote, uint64_t & node_total_votes){
    uint64_t old_vote_tickets = 0;
    if (!b_vote) {
        auto iter = votes_table.find(adv_account.to_string());
        if (iter != votes_table.end()) {
            old_vote_tickets = iter->second;
        }
        XCONTRACT_ENSURE(iter != votes_table.end(), "xtable_vote_contract::calc_advance_tickets: vote not found");
        XCONTRACT_ENSURE(votes <= old_vote_tickets, "xtable_vote_contract::calc_advance_tickets: votes not enough");
        if (votes == old_vote_tickets) {
            votes_table.erase(iter);
        } else {
            votes_table[adv_account.to_string()] -= votes;
        }

        node_total_votes -= votes;
    } else {
        auto min_votes_pernode_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_votes_pernode_num);
        XCONTRACT_ENSURE(votes >= min_votes_pernode_num, "xtable_vote_contract::handle_votes: lower than lowest votes");

        votes_table[adv_account.to_string()] += votes;

        auto max_vote_nodes_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num);
        XCONTRACT_ENSURE(votes_table.size() <= max_vote_nodes_num, "xtable_vote_contract::handle_votes: beyond the maximum nodes that can be voted by you");
        node_total_votes += votes;
    }
}

void xtable_vote_contract::update_table_votes_detail(common::xaccount_address_t const & account, std::map<std::string, uint64_t> const & votes_table){
    std::string property = std::string{data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE} + "-" +
                           std::to_string((utl::xxh32_t::digest(account.to_string()) % data::system_contract::XPROPERTY_SPLITED_NUM) + 1);
    if (votes_table.size() == 0) {
        MAP_REMOVE(property, account.to_string());
    } else {
        xstream_t stream(xcontext_t::instance());
        stream << votes_table;
        std::string vote_info_str = std::string((char *)stream.data(), stream.size());
        {
            XMETRICS_TIME_RECORD("sysContract_tableVote_set_property_contract_voter_key");
            MAP_SET(property, account.to_string(), vote_info_str);
        }
    }
}

void xtable_vote_contract::add_advance_tickets(common::xaccount_address_t const & advance_account, uint64_t tickets) {
    xdbg("[xtable_vote_contract::add_advance_tickets] adv account: %s, tickets: %llu, pid:%d\n", advance_account.to_string().c_str(), tickets, getpid());

    if (tickets == 0) {
        XMETRICS_TIME_RECORD("sysContract_tableVote_remove_property_contract_pollable_key");
        REMOVE(enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, advance_account.to_string());
    } else {
        XMETRICS_TIME_RECORD("sysContract_tableVote_set_property_contract_pollable_key");
        WRITE(enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, base::xstring_utl::tostring(tickets), advance_account.to_string());
    }
}

uint64_t xtable_vote_contract::get_advance_tickets(common::xaccount_address_t const & advance_account) {
    std::string value_str;

    {
        XMETRICS_TIME_RECORD("sysContract_tableVote_get_property_contract_pollable_key");
        int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, advance_account.to_string(), value_str);
        if (ret || value_str.empty()) {
            return 0;
        }
    }
    return base::xstring_utl::touint64(value_str);
}

bool xtable_vote_contract::is_expire(const common::xlogic_time_t onchain_timer_round) {
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
    common::xlogic_time_t new_time_height = onchain_timer_round;
    common::xlogic_time_t old_time_height = 0;

    std::string time_height_str = STRING_GET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY);
    if (!time_height_str.empty()) {
        old_time_height = xstring_utl::touint64(time_height_str);
    }
    XCONTRACT_ENSURE(new_time_height >= old_time_height, "new_time_height < old_time_height error");
    xdbg("[xtable_vote_contract::is_expire] new_time_height: %llu, old_time_height: %lld, timer_interval: %d, pid: %d\n",
         new_time_height,
         old_time_height,
         timer_interval,
         getpid());

    if (new_time_height - old_time_height <= timer_interval) {
        return false;
    }

    STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, xstring_utl::tostring(new_time_height));
    return true;
}

void xtable_vote_contract::commit_total_votes_num() {
    uint64_t total_votes = 0;
    std::map<std::string, std::string> pollables;
    try {
        XMETRICS_TIME_RECORD("sysContract_tableVote_get_property_contract_pollable_key");
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, pollables);
    } catch (std::runtime_error & e) {
        xdbg("[xtable_vote_contract::commit_total_votes_num] MAP COPY GET error:%s", e.what());
        throw;
    }

    uint64_t timer = TIME();
    xinfo("[xtable_vote_contract::commit_total_votes_num] split table vote trx %" PRIu64, timer);
    split_and_report(sys_contract_zec_vote_addr, "on_receive_shard_votes_v2", pollables);
}

void xtable_vote_contract::split_and_report(std::string const& report_contract, std::string const& report_func, std::map<std::string, std::string> const& report_content) {
    auto timer = TIME();
    if (report_content.empty()) {
        base::xstream_t  call_stream(base::xcontext_t::instance());
        call_stream << timer;
        call_stream << report_content;
        xinfo("[xtable_vote_contract::split_and_report] the report content empty");
        CALL(common::xaccount_address_t{report_contract}, report_func, std::string((char *)call_stream.data(), call_stream.size()));
        return;
    }

    auto res = trx_split_helper(report_content, XVOTE_TRX_LIMIT);
    for (std::size_t i = 0; i < res.size(); ++i) {
        base::xstream_t  call_stream(base::xcontext_t::instance());
        call_stream << timer;
        call_stream << res[i];
        xinfo("[xtable_vote_contract::split_and_report] the report content size %u, round %u", res[i].size(), i + 1);
        CALL(common::xaccount_address_t{report_contract}, report_func, std::string((char *)call_stream.data(), call_stream.size()));
    }
}

std::vector<std::map<std::string, std::string>> xtable_vote_contract::trx_split_helper(std::map<std::string, std::string> const& report_content, std::size_t limit) {
    std::vector<std::map<std::string, std::string>> res;

    if (report_content.size() == 0) {
        xinfo("[xtable_vote_contract::trx_split_helper] the report content size zero");
        return res;
    }

    if (report_content.size() <= limit) {
        xinfo("[xtable_vote_contract::trx_split_helper] the report content size %u", report_content.size());
        res.push_back(report_content);
    } else {

        uint16_t count = 0;
        std::map<std::string, std::string> split_report_content;
        for (auto const& item: report_content) {
            count++;
            split_report_content[item.first] = item.second;
            if (count % limit == 0) {
                xinfo("[xtable_vote_contract::trx_split_helper] the report content size %u, count %u", report_content.size(), count);
                res.push_back(split_report_content);
                count = 0;
                split_report_content.clear();
            }

        }

        if (!split_report_content.empty()) {
            xinfo("[xtable_vote_contract::trx_split_helper] the report content size %u, count %u", report_content.size(), split_report_content.size());
            res.push_back(split_report_content);
        }
    }

    return res;
}

void xtable_vote_contract::on_timer(common::xlogic_time_t const timestamp) {
    if (!chain_fork::xutility_t::is_forked(fork_points::v1_9_0_vote_contract_update_point, timestamp)) {
        xinfo("xtable_vote_contract::on_timer not forked yet, %lu", timestamp);
        return;
    }

    auto const & source_addr = SOURCE_ADDRESS();
    auto const & self_addr = SELF_ADDRESS().to_string();

    std::string base_addr;
    uint32_t table_id{0};
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    XCONTRACT_ENSURE(source_addr == self_addr, "invalid source addr's call!");
    XCONTRACT_ENSURE(base_addr == sys_contract_sharding_vote_addr, "invalid source addr's call!");

    xinfo("[xtable_vote_contract::on_timer] timer: %lu, account: %s", timestamp, self_addr.c_str());

    auto all_effective_votes = get_and_update_all_effective_votes_of_all_account(timestamp);
    auto flag = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY);
    if (all_effective_votes.empty() && flag != xstring_utl::tostring(1)) {
        xinfo("[xtable_vote_contract::on_timer] no vote to upload");
        return;
    }
    for (auto const & v : all_effective_votes) {
        handle_votes(v.first, v.second, true);
    }
    // get update data
    std::map<std::string, std::string> adv_votes;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
    // call other contracts
    split_and_report(sys_contract_rec_registration_addr, "update_batch_stake_v2", adv_votes);
    split_and_report(sys_contract_zec_vote_addr, "on_receive_shard_votes_v2", adv_votes);
    xinfo("[xtable_vote_contract::on_timer] split table vote finish, time: %lu", timestamp);
    STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, xstring_utl::tostring(0));
}

std::map<common::xaccount_address_t, xtable_vote_contract::vote_info_map_t> xtable_vote_contract::get_and_update_all_effective_votes_of_all_account(uint64_t const timestamp) {
    if (!MAP_PROPERTY_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY)) {
        xwarn("[xtable_vote_contract::get_all_effective_votes_of_all_account] XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY not created yet");
        return {};
    }

    std::map<std::string, std::string> ineffective_votes_str_map;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, ineffective_votes_str_map);
    if (ineffective_votes_str_map.empty()) {
        xwarn("[xtable_vote_contract::get_all_effective_votes_of_all_account] XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY empty");
        return {};
    }

    auto ineffective_period = XGET_ONCHAIN_GOVERNANCE_PARAMETER(table_vote_ineffective_period);
    std::map<common::xaccount_address_t, xtable_vote_contract::vote_info_map_t> all_effective_votes;
    for (auto const & p : ineffective_votes_str_map) {
        auto const & voter = p.first;
        auto const & ineffective_votes_str = p.second;

        std::map<std::uint64_t, vote_info_map_t> all_time_ineffective_votes;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)ineffective_votes_str.c_str(), (uint32_t)ineffective_votes_str.size());
        stream >> all_time_ineffective_votes;

        vote_info_map_t effective_votes;
        for (auto it = all_time_ineffective_votes.begin(); it != all_time_ineffective_votes.end();) {
            if (it->first + ineffective_period > timestamp) {
                it++;
                continue;
            }
            auto const & vote_infos = it->second;
            for (auto const & vote_info : vote_infos) {
                auto const & acc = vote_info.first;
                auto const & num = vote_info.second;
                if (effective_votes.count(acc)) {
                    effective_votes.at(acc) += num;
                } else {
                    effective_votes.insert({acc, num});
                }
            }
            all_time_ineffective_votes.erase(it++);
        }
        base::xstream_t new_stream(base::xcontext_t::instance());
        stream << all_time_ineffective_votes;
        if (effective_votes.empty()) {
            continue;
        } else {
            if (all_time_ineffective_votes.empty()) {
                MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter);
            } else {
                base::xstream_t new_stream(base::xcontext_t::instance());
                stream << all_time_ineffective_votes;
                std::string new_ineffective_votes_str{std::string((char *)stream.data(), stream.size())};
                MAP_SET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter, new_ineffective_votes_str);
            }
            all_effective_votes.insert({common::xaccount_address_t{voter}, effective_votes});
        }
    }
    return all_effective_votes;
}

std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> xtable_vote_contract::get_all_time_ineffective_votes(common::xaccount_address_t const & account) {
    if (!MAP_PROPERTY_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY)) {
        xwarn("[xtable_vote_contract::get_all_time_ineffective_votes] XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY not created yet");
        return {};
    }
    std::map<std::uint64_t, vote_info_map_t> all_time_ineffective_votes;
    std::string ineffective_votes_str;
    if (MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, account.to_string(), ineffective_votes_str)) {
        // here if not success, means account has no vote info yet, so vote_info_str is empty, using above default votes_table directly
        xwarn("[xtable_vote_contract::get_all_time_ineffective_votes] get XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY of %s empty", account.to_string().c_str());
    }
    if (!ineffective_votes_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)ineffective_votes_str.c_str(), (uint32_t)ineffective_votes_str.size());
        stream >> all_time_ineffective_votes;
    }
    return all_time_ineffective_votes;
}

void xtable_vote_contract::set_all_time_ineffective_votes(common::xaccount_address_t const & account, std::map<std::uint64_t, vote_info_map_t> const & all_time_ineffective_votes) {
    // TODO: add fork
    if (!MAP_PROPERTY_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY)) {
        MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY);
    }
    base::xstream_t stream(base::xcontext_t::instance());
    stream << all_time_ineffective_votes;
    std::string new_ineffective_votes_str{std::string((char *)stream.data(), stream.size())};
    MAP_SET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, account.to_string(), new_ineffective_votes_str);
}

void xtable_vote_contract::add_all_time_ineffective_votes(uint64_t const timestamp, vote_info_map_t const & vote_info, std::map<std::uint64_t, vote_info_map_t> & all_time_ineffective_votes) {
    if (all_time_ineffective_votes.count(timestamp)) {
        auto & vote_infos = all_time_ineffective_votes.at(timestamp);
        for (auto const & v : vote_info) {
            auto const & adv = v.first;
            auto const & votes = v.second;
            if (vote_infos.count(adv)) {
                vote_infos.at(adv) += votes;
            } else {
                vote_infos.insert({adv, votes});
            }
        }
    } else {
        all_time_ineffective_votes.insert({timestamp, vote_info});
    }
}

void xtable_vote_contract::del_all_time_ineffective_votes(vote_info_map_t & vote_info, std::map<std::uint64_t, vote_info_map_t> & all_time_ineffective_votes) {
    for (auto it_old = all_time_ineffective_votes.rbegin(); it_old != all_time_ineffective_votes.rend();) {
        auto & old_vote_infos = it_old->second;
        for (auto it_new = vote_info.begin(); it_new != vote_info.end();) {
            auto const & acc = it_new->first;
            auto & votes = it_new->second;
            if (old_vote_infos.count(acc)) {
                if (old_vote_infos.at(acc) > votes) {
                    old_vote_infos.at(acc) -= votes;
                    vote_info.erase(it_new++);
                } else if (old_vote_infos.at(acc) == votes) {
                    old_vote_infos.erase(acc);
                    vote_info.erase(it_new++);
                } else {
                    votes -= old_vote_infos.at(acc);
                    old_vote_infos.erase(acc);
                    it_new++;
                }
            } else {
                it_new++;
            }
        }
        if (old_vote_infos.empty()) {
            all_time_ineffective_votes.erase((++it_old).base());
        } else {
            it_old++;
        }
    }
}

NS_END2
