// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"

#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xbasic/xhex.h"
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

std::string calc_voter_tickets_storage_property_name(common::xaccount_address_t const & voter) {
    uint32_t const sub_map_no = (utl::xxh32_t::digest(voter.to_string()) % data::system_contract::XPROPERTY_SPLITED_NUM) + 1;
    return std::string{data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE} + "-" + std::to_string(sub_map_no);
}

std::string const xtable_vote_contract::flag_upload_tickets_10900{"0"};
std::string const xtable_vote_contract::flag_withdraw_tickets_10900{"1"};
std::string const xtable_vote_contract::flag_reset_tickets{"2"};
std::string const xtable_vote_contract::flag_upload_tickets_10901{"3"};
std::string const xtable_vote_contract::flag_withdraw_tickets_10901{"4"};
std::string const xtable_vote_contract::flag_upload_tickets_10902{"5"};
std::string const xtable_vote_contract::flag_withdraw_tickets_10902{"6"};
static constexpr size_t legacy_flag_length{7};

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
    auto const timestamp = TIME();

    if (!chain_fork::xutility_t::is_forked(fork_points::v10902_enable_voting, timestamp)) {
        xkinfo("voteNode is not enabled for now");
        XCONTRACT_ENSURE(false, "voteNode is not enabled yet");
        top::unreachable();
    }

    auto const & account = common::xaccount_address_t{SOURCE_ADDRESS()};
    xinfo("[xtable_vote_contract::voteNode] timer round: %" PRIu64 ", src_addr: %s, self addr: %s, pid: %d\n",
          timestamp,
          account.to_string().c_str(),
          SELF_ADDRESS().to_string().c_str(),
          getpid());

    XCONTRACT_ENSURE(common::is_t0_address(account) || common::is_t8_address(account), "only T0 or T8 account can vote for an account");

    const data::xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(trans_ptr->get_tx_type() == data::xtransaction_type_vote,
                     "xtable_vote_contract::voteNode: transaction_type must be xtransaction_type_vote");
    XMETRICS_PACKET_INFO("sysContract_tableVote_vote_node", "timer round", std::to_string(timestamp), "voter address", account.to_string());

    do {
        if (chain_fork::xutility_t::is_forked(fork_points::v10902_enable_voting, timestamp) ||
            chain_fork::xutility_t::is_forked(fork_points::v10901_enable_voting, timestamp) ||
            chain_fork::xutility_t::is_forked(fork_points::v10900_upgrade_table_tickets_contract, timestamp)) {
            set_vote_info_v2(account, vote_info, true);
            break;
        }

        set_vote_info(account, vote_info, true);
        break;

    } while (false);
}

void xtable_vote_contract::unvoteNode(vote_info_map_t const & vote_info) {
    auto const timestamp = TIME();

    if (!chain_fork::xutility_t::is_forked(fork_points::v10902_enable_voting, timestamp)) {
        xkinfo("voteNode is not enabled for now");
        XCONTRACT_ENSURE(false, "unvoteNode is not enabled yet");
        top::unreachable();
    }

    auto const & account = common::xaccount_address_t{SOURCE_ADDRESS()};
    xinfo("[xtable_vote_contract::unvoteNode] timer round: %" PRIu64 ", src_addr: %s, self addr: %s", timestamp, account.to_string().c_str(), SELF_ADDRESS().to_string().c_str());

    XCONTRACT_ENSURE(common::is_t0_address(account) || common::is_t8_address(account), "only T0 or T8 can withdraw votes from an account");

    const data::xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(trans_ptr->get_tx_type() == data::xtransaction_type_abolish_vote,
                     "xtable_vote_contract::unvoteNode: transaction_type must be xtransaction_type_abolish_vote");
    XMETRICS_PACKET_INFO("sysContract_tableVote_unvote_node", "timer round", std::to_string(timestamp), "unvoter address", account.to_string());


    do {
        if (chain_fork::xutility_t::is_forked(fork_points::v10902_enable_voting, timestamp)) {
            set_vote_info_v2(account, vote_info, false);
            STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_withdraw_tickets_10902);
            break;
        }

        if (chain_fork::xutility_t::is_forked(fork_points::v10901_enable_voting, timestamp)) {
            set_vote_info_v2(account, vote_info, false);
            STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_withdraw_tickets_10901);
            break;
        }

        if (chain_fork::xutility_t::is_forked(fork_points::v10900_upgrade_table_tickets_contract, timestamp)) {
            set_vote_info_v2(account, vote_info, false);
            STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_withdraw_tickets_10900);
            break;
        }

        set_vote_info(account, vote_info, false);
        break;

    } while (false);
}

void xtable_vote_contract::set_vote_info(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool const b_vote) {
    // votes process
    handle_votes(account, vote_info, b_vote, true);
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
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
    } catch (std::runtime_error & e) {
        xdbg("[update_adv_votes] MAP COPY GET error:%s", e.what());
        throw;
    }

    uint64_t timer = TIME();
    xinfo("[xtable_vote_contract::commit_stake] split table vote trx %" PRIu64, timer);
    split_and_report(sys_contract_rec_registration_addr, "update_batch_stake_v2", adv_votes);
}

void xtable_vote_contract::set_vote_info_v2(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool const b_vote) {
    auto all_time_ineffective_votes = get_all_time_ineffective_votes(account);
#if defined(DEBUG)
    {
        xdbg("voter %s %s tickets; time:%" PRIu64, account.to_string().c_str(), b_vote ? "deposit" : "withdraw", TIME());
        for (auto const & d : vote_info) {
            xdbg("\tadv:%s;tickets:%" PRIu64, d.first.c_str(), d.second);
        }
    }
    {
        xdbg("voter %s read ineffective votes", account.to_string().c_str());
        for (auto const & d : all_time_ineffective_votes) {
            xdbg("\tvoteTime:%" PRIu64 "\n", d.first);
            for (auto const & detail : d.second) {
                xdbg("\tadv:%s;tickets:%" PRIu64, detail.first.c_str(), detail.second);
            }
        }
    }
#endif
    if (!b_vote) {
        auto vote_info_to_del = vote_info;
        del_all_time_ineffective_votes(vote_info_to_del, all_time_ineffective_votes);
        if (!vote_info_to_del.empty()) {
            handle_votes(account, vote_info_to_del, b_vote, true);
        }
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

void xtable_vote_contract::handle_votes(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool const b_vote, bool const check_tickets_recver) {
    std::map<std::string, uint64_t> votes_table = get_table_votes_detail(account);

    for (auto const & entity : vote_info) {
        auto const & adv_account = entity.first;
        auto const & votes = entity.second;

        xinfo("[xtable_vote_contract::handle_votes] b_vote: %d, voter: %s, node: %s, votes: %u", b_vote, account.to_string().c_str(), adv_account.c_str(), votes);
        common::xaccount_address_t address{adv_account};
        if (b_vote && check_tickets_recver) {
            data::system_contract::xreg_node_info node_info;
            auto const ret = get_node_info(address, node_info);
            XCONTRACT_ENSURE(ret == 0, "xtable_vote_contract::handle_votes: node not exist");
            XCONTRACT_ENSURE(node_info.has<common::xminer_type_t::advance>(), "xtable_vote_contract::handle_votes: only auditor can be voted");
        }
        uint64_t node_total_votes = get_advance_tickets(address);
        calc_advance_tickets(address, votes, votes_table, b_vote, node_total_votes);
        add_advance_tickets(address, node_total_votes);
    }
    update_table_votes_detail(account, votes_table);
}

void xtable_vote_contract::calc_advance_tickets(common::xaccount_address_t const & adv_account, uint64_t const votes, std::map<std::string, uint64_t> & votes_table, bool const b_vote, uint64_t & node_total_votes){
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
            MAP_SET(property, account.to_string(), vote_info_str);
        }
    }
}

void xtable_vote_contract::add_advance_tickets(common::xaccount_address_t const & advance_account, uint64_t tickets) {
    xdbg("[xtable_vote_contract::add_advance_tickets] adv account: %s, tickets: %llu, pid:%d\n", advance_account.to_string().c_str(), tickets, getpid());

    if (tickets == 0) {
        REMOVE(xvm::xcontract::enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, advance_account.to_string());
    } else {
        WRITE(xvm::xcontract::enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, base::xstring_utl::tostring(tickets), advance_account.to_string());
    }
}

uint64_t xtable_vote_contract::get_advance_tickets(common::xaccount_address_t const & advance_account) {
    std::string value_str;

    {
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
    // uint64_t total_votes = 0;
    std::map<std::string, std::string> pollables;
    try {
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

void xtable_vote_contract::on_timer(common::xlogic_time_t const) {
    auto const timestamp = TIME();
    auto const & contract_address = SELF_ADDRESS();
    auto flag = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY);
    bool reset_touched;

    do {
        if (chain_fork::xutility_t::is_forked(fork_points::v10902_table_tickets_reset, timestamp)) {
#if defined(XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_GALILEO) || defined(XBUILD_BOUNTY)
            std::map<common::xaccount_address_t, vote_info_map_t> const contract_ticket_reset_data{};
            std::vector<common::xaccount_address_t> const contract_ticket_clear_data{};
#else
            std::map<common::xaccount_address_t, vote_info_map_t> const contract_ticket_reset_data{
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T00000LSpJQpbgAX1NVG9Z1xff8Do7yLPeB9bivm"},  // voter
                    vote_info_map_t{
                        std::pair<std::string, uint64_t>{"T00000LSpJQpbgAX1NVG9Z1xff8Do7yLPeB9bivm", 21470000}
                    }
                },
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T80000e419a25a278984bbdc9fd13b9a1124c31b7b9d8f"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T00000LL2XWarXm7LdqJ31yAw9NFSAcnRTBXSMCR", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LLQihYMwRDDeytKptzHM3VxpEoABYz4Qne", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LLz5pwzVeN5cGD5F1CNfvvp1HmUnVrVdp1", 3200000},
                        std::pair<std::string, uint64_t>{"T00000LNPLyVS2ARTRc1CB6v61w12v6tVrKVt2jm", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LRbDf962M5o1dBDV4sENKLGdvrcMFaqHe5", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LSJnBmr1HGBb98E8J2mAzrJHBaUJDt3mdg", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LUajtWFWw6k9XAxx9k3Hzi6BM1QmLg8aH9", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LUsioNY2eUYro1ABzAH5jkDLENnVj4T45T", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LV59MnzTeDhL5cgDfqvo6Wqw9z38ZFPLBD", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LV8ypojCxqsWiqTjvn26Y4EkdwjNp7HAn1", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LVEx38TFk9nXapXPi1bBoG7VyfEB9EGtWo", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LX9EspYELp1GZjMH1bFgVSWPy6xoPH9pNC", 3200000},
                        std::pair<std::string, uint64_t>{"T00000LZcStzruL5YNDkzU1DHLGeqputuGVFNucV", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LamxJFU1v65RW9dwcku4XoEKchSzjfauhh", 10000000},
                        std::pair<std::string, uint64_t>{"T00000Lbci1RFQMJUpyGA3EJbYEq4DP1kieXVK4i", 10194819},
                        std::pair<std::string, uint64_t>{"T00000LTPZ6DMXbhdQY2hwr1q3XLXK5mcjraqSUs", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LRhBnfTjAs7cxVxH7LCrgJLvr6oGeURAxx", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LbpeEX3NT9ivXXLSGpbjRpBCVSihtc3ZUy", 10000000},
                        std::pair<std::string, uint64_t>{"T00000Lc4cxGkSd3Ww78kLGCLCp8ST4DYTKvL6Tr", 5190502},
                        std::pair<std::string, uint64_t>{"T00000LczKtQH9R6W6dFQtWGg6Z8paU35bQ3rceR", 1600000},
                        std::pair<std::string, uint64_t>{"T00000Ldtg6RkFTg5mpZAttkwVv8UzyfyqFZnint", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LeW4yvkwMex8LRCjZDTVTz64QzkVsUxAeq", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LfVPF4BQx5hQSP5LpL64gDCedvXjpApPrK", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LhsQ8nti6Eug2FKQ5Wzeosq6q9RDHEf6pZ", 1600000},
                    }
                },
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T80000174dba8a0f1adedb1d2ec9d7bcf7f365bd5358e9"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T80000dba88236ddb99ffa1c31a6ca65ca6a8c642109ad", 2060296}
                    }
                },
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T800008de3d618bf8cd6cd95bbf44d5d51f6137f669711"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T8000027363d7be59b5537a6a5ce7de9dbefa826f70b63", 255431}
                    }
                },
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T80000896640d688ceb552c4b498b32a6cc0acb21e64cf"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T800000139af69c102783786e46f7cb900a11d8efe0480", 5000},
                        std::pair<std::string, uint64_t>{"T00000LMBQ52vWWq17L7VCVVZkwMD1m1W4hZZEXR", 200000},
                        std::pair<std::string, uint64_t>{"T80000b7a8155da635d974d06ea8b25885164ca226d1d5", 11000},
                        std::pair<std::string, uint64_t>{"T00000LNTsgQq8sGgXmJSNMqEpze9bnqjrTpbk4K", 11000},
                        std::pair<std::string, uint64_t>{"T00000LLbZMiWkKQdgNbMrsfJ2wW2b2ay6aegco8", 400011},
                        std::pair<std::string, uint64_t>{"T8000056ab5fa5b3cc76881184119d410826cdc7eb26f6", 4000},
                        std::pair<std::string, uint64_t>{"T00000LUswbTgsTpUCbgn4Dh1y3seenQwNJD8Gbv", 631000},
                        std::pair<std::string, uint64_t>{"T00000LPEFL9d7ZXosYfA9XQGWe2z7HRB8DYzFJ6", 458000},
                        std::pair<std::string, uint64_t>{"T00000LLbUf4MyKSZm6nMqFeL5wH3frCUUssThYH", 1000},
                        std::pair<std::string, uint64_t>{"T00000LRLZ9dAmzxvtHxEv8fa6cGfKGB3n8b1w7K", 1000},
                    }
                },
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T800003821c220de1dd1d282a1f9d32dfcabb8345b4be1"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T00000LMBQ52vWWq17L7VCVVZkwMD1m1W4hZZEXR", 1317286}
                    }
                }
            };

            std::vector<common::xaccount_address_t> const contract_ticket_clear_data{
                common::xaccount_address_t{"T800001753d40631a3ad31568c3141272cac45692888d1"}
            };
#endif

            reset_touched = reset_v10902(flag, contract_ticket_reset_data, contract_ticket_clear_data);
            break;
        }

        if (chain_fork::xutility_t::is_forked(fork_points::v10901_table_tickets_reset, timestamp)) {
#if defined(XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_GALILEO) || defined(XBUILD_BOUNTY)
            std::map<common::xaccount_address_t, vote_info_map_t> const contract_ticket_reset_data{};
            std::vector<common::xaccount_address_t> const contract_ticket_clear_data{};
#else
            std::map<common::xaccount_address_t, vote_info_map_t> const contract_ticket_reset_data{
                std::pair<common::xaccount_address_t, vote_info_map_t>{
                    common::xaccount_address_t{"T80000e419a25a278984bbdc9fd13b9a1124c31b7b9d8f"},  // voter
                    vote_info_map_t{                                                               // {adv, ticket count}
                        std::pair<std::string, uint64_t>{"T00000LL2XWarXm7LdqJ31yAw9NFSAcnRTBXSMCR", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LhsQ8nti6Eug2FKQ5Wzeosq6q9RDHEf6pZ", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LX9EspYELp1GZjMH1bFgVSWPy6xoPH9pNC", 3200000},
                        std::pair<std::string, uint64_t>{"T00000LLz5pwzVeN5cGD5F1CNfvvp1HmUnVrVdp1", 3200000},
                        std::pair<std::string, uint64_t>{"T00000LczKtQH9R6W6dFQtWGg6Z8paU35bQ3rceR", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LRbDf962M5o1dBDV4sENKLGdvrcMFaqHe5", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LV8ypojCxqsWiqTjvn26Y4EkdwjNp7HAn1", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LUsioNY2eUYro1ABzAH5jkDLENnVj4T45T", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LUajtWFWw6k9XAxx9k3Hzi6BM1QmLg8aH9", 1600000},
                        std::pair<std::string, uint64_t>{"T00000LSJnBmr1HGBb98E8J2mAzrJHBaUJDt3mdg", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LamxJFU1v65RW9dwcku4XoEKchSzjfauhh", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LVEx38TFk9nXapXPi1bBoG7VyfEB9EGtWo", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LeW4yvkwMex8LRCjZDTVTz64QzkVsUxAeq", 10000000},
                        std::pair<std::string, uint64_t>{"T00000Ldtg6RkFTg5mpZAttkwVv8UzyfyqFZnint", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LLQihYMwRDDeytKptzHM3VxpEoABYz4Qne", 10000000},
                        std::pair<std::string, uint64_t>{"T00000LY2gm2ApdNQTnEuSdnqJRn6zG9gPyptpXJ", 2000000},
                        std::pair<std::string, uint64_t>{"T00000LMBQ52vWWq17L7VCVVZkwMD1m1W4hZZEXR", 1317286}
                    }
                }
            };

            std::vector<common::xaccount_address_t> const contract_ticket_clear_data{
                common::xaccount_address_t{"T800003821c220de1dd1d282a1f9d32dfcabb8345b4be1"},  // voter
            };
#endif
            reset_touched = reset_v10901(flag, contract_ticket_reset_data, contract_ticket_clear_data);
            break;
        }

        xinfo("xtable_vote_contract::on_timer not forked yet, %" PRIu64, static_cast<uint64_t>(timestamp));
        XCONTRACT_ENSURE(false, "fork point not reached");
        top::unreachable();

    } while (false);

    if (reset_touched) {  // re-calculating aggregated table ticket data (XPORPERTY_CONTRACT_POLLABLE_KEY).
        reset_table_tickets_data();

        flag = flag_reset_tickets;
        xdbg("table %s sets reset flag", contract_address.to_string().c_str());
    }

    auto const & source_addr = SOURCE_ADDRESS();

    std::string base_addr;
    uint32_t table_id{0};
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    XCONTRACT_ENSURE(source_addr == contract_address.to_string(), "invalid source addr's call!");
    XCONTRACT_ENSURE(base_addr == sys_contract_sharding_vote_addr, "invalid source addr's call!");

    xinfo("[xtable_vote_contract::on_timer] timer: %lu, account: %s", timestamp, contract_address.to_string().c_str());

    auto const all_effective_votes = get_and_update_all_effective_votes_of_all_account(timestamp);
    do {
        if (all_effective_votes.empty()) {
            if (flag == flag_withdraw_tickets_10902 || flag == flag_withdraw_tickets_10901 || flag == flag_withdraw_tickets_10900 || flag.length() >= legacy_flag_length) {
                xinfo("xtable_vote_contract::on_timer: table %s effective votes empty but needs to be uploaded %s", contract_address.to_string().c_str(), flag.c_str());
                break;
            }

            if (flag == flag_reset_tickets) {
                xinfo("xtable_vote_contract::on_timer: table %s effective votes empty but needs to be uploaded %s", contract_address.to_string().c_str(), flag.c_str());
                break;
            }

            xinfo("[xtable_vote_contract::on_timer] no vote to upload. flag %s", flag.c_str());
            return;
        }
    } while (false);

    for (auto const & v : all_effective_votes) {
        handle_votes(v.first, v.second, true, false);
    }

    // get update data
    std::map<std::string, std::string> adv_votes;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
    xdbg("xtable_vote_contract::on_timer: table %s property %s read %zu adv",
         contract_address.to_string().c_str(),
         data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY,
         adv_votes.size());

    // call other contracts
    split_and_report(sys_contract_rec_registration_addr, "update_batch_stake_v2", adv_votes);
    split_and_report(sys_contract_zec_vote_addr, "on_receive_shard_votes_v2", adv_votes);
    xinfo("[xtable_vote_contract::on_timer] split table vote finish, time: %lu", timestamp);
    if (chain_fork::xutility_t::is_forked(fork_points::v10902_table_tickets_reset, timestamp)) {
        STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_upload_tickets_10902);
    } else if (chain_fork::xutility_t::is_forked(fork_points::v10901_table_tickets_reset, timestamp)) {
        STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_upload_tickets_10901);
    } else {
        STRING_SET(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY, flag_upload_tickets_10900);
    }
    xdbg("table %s sets upload flag", contract_address.to_string().c_str());
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

    std::map<common::xaccount_address_t, vote_info_map_t> all_effective_votes;
    for (auto const & p : ineffective_votes_str_map) {
        auto const & voter = p.first;
        auto const & ineffective_votes_str = p.second;

        if (ineffective_votes_str.empty()) {
            xwarn("voter %s property %s empty", voter.c_str(), data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY);
        }

        std::map<std::uint64_t, vote_info_map_t> all_time_ineffective_votes;
        if (!ineffective_votes_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)ineffective_votes_str.c_str(), (uint32_t)ineffective_votes_str.size());
            stream >> all_time_ineffective_votes;

#if defined(DEBUG)
            {
                for (auto const & d : all_time_ineffective_votes) {
                    xwarn("voter %s current read ineffective data: voteTime: %" PRIu64, voter.c_str(), d.first);
                    for (auto const & detail : d.second) {
                        xwarn("\tadv:%s tickets:%" PRIu64, detail.first.c_str(), detail.second);
                    }
                }
            }

            if (stream.size() > 0) {
                xwarn("voter %s stream has extra data", voter.c_str());

                auto const * ptr = stream.data();
                size_t const sz = stream.size();

                xbytes_t data{ptr, ptr + sz};
                xstream_t stream_cp(base::xcontext_t::instance(), data.data(), (uint32_t)data.size());
                std::map<std::uint64_t, vote_info_map_t> extra_data;
                while (stream_cp.size() > 0) {
                    stream_cp >> extra_data;

                    for (auto const & d : extra_data) {
                        xwarn("\tvoteTime: %" PRIu64, d.first);
                        for (auto const & detail : d.second) {
                            xwarn("\t\tadv:%s tickets:%" PRIu64, detail.first.c_str(), detail.second);
                        }
                    }
                }
            }
#endif

        }

        vote_info_map_t effective_votes;
        for (auto it = all_time_ineffective_votes.begin(); it != all_time_ineffective_votes.end();) {
            if (it->first + ineffective_period > timestamp) {
                ++it;
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

        if (effective_votes.empty()) {
            continue;
        }

#if defined(DEBUG)
        {
            std::string log = voter + "\n" + "\teffectived:";
            for (auto const & d : effective_votes) {
                log += "\tadv:" + d.first + ";tickets:" + std::to_string(d.second) + "\n";
            }
            xdbg("effective tickets:\n%s", log.c_str());
        }
        {
            std::string log = voter + "\n";
            for (auto const & d : all_time_ineffective_votes) {
                log += "\t1voteTime:" + std::to_string(d.first) + ";\n";
                for (auto const & detail : d.second) {
                    log += "\t\tadv:" + detail.first + ";tickets:" + std::to_string(detail.second) + "\n";
                }
            }
            xdbg("all_time_ineffective_votes serialized to stream:%s", log.c_str());
        }
#endif

        if (all_time_ineffective_votes.empty()) {
            MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter);
        } else {

#if defined(DEBUG)
            {
                std::string log = voter + "\n";
                for (auto const & d : all_time_ineffective_votes) {
                    log += "\t2voteTime:" + std::to_string(d.first) + ";\n";
                    for (auto const & detail : d.second) {
                        log += "\t\tadv:" + detail.first + ";tickets:" + std::to_string(detail.second) + "\n";
                    }
                }
                xdbg("all_time_ineffective_votes serialized to stream:%s", log.c_str());
            }
#endif

            base::xstream_t new_stream(base::xcontext_t::instance());
            new_stream << all_time_ineffective_votes;
            std::string new_ineffective_votes_str{reinterpret_cast<char *>(new_stream.data()), static_cast<size_t>(new_stream.size())};
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter, new_ineffective_votes_str);
        }
        all_effective_votes.insert({common::xaccount_address_t{voter}, effective_votes});
    }
    return all_effective_votes;
}

std::map<common::xlogic_time_t, xtable_vote_contract::vote_info_map_t> xtable_vote_contract::get_all_time_ineffective_votes(common::xaccount_address_t const & account) {
    if (!MAP_PROPERTY_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY)) {
        xwarn("[xtable_vote_contract::get_all_time_ineffective_votes] XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY not created yet");
        return {};
    }
    std::map<common::xlogic_time_t, vote_info_map_t> all_time_ineffective_votes;
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

#if defined(DEBUG)
    {
        xdbg("voter %s set ineffective votes", account.to_string().c_str());
        for (auto const & d : all_time_ineffective_votes) {
            xdbg("\tvoteTime:%" PRIu64 "\n", d.first);
            for (auto const & detail : d.second) {
                xdbg("\tadv:%s;tickets:%" PRIu64, detail.first.c_str(), detail.second);
            }
        }
    }
#endif

    base::xstream_t stream(base::xcontext_t::instance());
    stream << all_time_ineffective_votes;
    std::string new_ineffective_votes_str{std::string((char *)stream.data(), stream.size())};
    MAP_SET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, account.to_string(), new_ineffective_votes_str);
}

void xtable_vote_contract::add_all_time_ineffective_votes(uint64_t const timestamp, vote_info_map_t const & vote_info, std::map<std::uint64_t, vote_info_map_t> & all_time_ineffective_votes) {
    for (auto const & entity : vote_info) {
        auto const & adv_account = entity.first;
        auto const votes = entity.second;

        xinfo("[xtable_vote_contract::add_all_time_ineffective_votes] adv account: %s, votes: %u", adv_account.c_str(), votes);
        {
            data::system_contract::xreg_node_info node_info;
            auto const ret = get_node_info(common::xaccount_address_t{adv_account}, node_info);
            XCONTRACT_ENSURE(ret == 0, "xtable_vote_contract::add_all_time_ineffective_votes: node not exist");
            XCONTRACT_ENSURE(node_info.has<common::xminer_type_t::advance>(), "xtable_vote_contract::add_all_time_ineffective_votes: only auditor can be voted");
        }
    }

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
        auto & old_vote_infos = top::get<vote_info_map_t>(*it_old);

        for (auto it_new = vote_info.begin(); it_new != vote_info.end();) {
            auto const & acc = top::get<std::string const>(*it_new);
            auto & votes = top::get<uint64_t>(*it_new);

            if (old_vote_infos.count(acc)) {
                if (old_vote_infos.at(acc) > votes) {
                    old_vote_infos.at(acc) -= votes;
                    it_new = vote_info.erase(it_new);
                } else if (old_vote_infos.at(acc) == votes) {
                    old_vote_infos.erase(acc);
                    it_new = vote_info.erase(it_new);
                } else {
                    votes -= old_vote_infos.at(acc);
                    old_vote_infos.erase(acc);
                    ++it_new;
                }
            } else {
                ++it_new;
            }
        }
        if (old_vote_infos.empty()) {
            it_old = decltype(it_old){all_time_ineffective_votes.erase((++it_old).base())};
        } else {
            ++it_old;
        }

        if (all_time_ineffective_votes.empty() || vote_info.empty()) {
            break;
        }
    }
}

std::string xtable_vote_contract::flag() const {
    return STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY);
}

std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, uint64_t>> xtable_vote_contract::tickets_data(std::string const & property_name) const {
    std::map<std::string, std::string> data;

    MAP_COPY_GET(property_name, data);

    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, uint64_t>> result;

    for (auto const & datum : data) {
        auto const & voter = common::xaccount_address_t{datum.first};
        auto const & raw_data = datum.second;
        if (raw_data.empty()) {
            result.emplace(voter, std::map<common::xaccount_address_t, uint64_t>{});
            continue;
        }
        std::map<common::xaccount_address_t, uint64_t> detail;

        xstream_t stream{xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(raw_data.data())), static_cast<uint32_t>(raw_data.size())};
        stream >> detail;

        result.emplace(voter, std::move(detail));
    }

    return result;
}

std::map<common::xaccount_address_t, std::map<common::xlogic_time_t, std::map<common::xaccount_address_t, uint64_t>>> xtable_vote_contract::ineffective_data() const {
    if (!MAP_PROPERTY_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY)) {
        return {};
    }

    std::map<std::string, std::string> ineffective_votes_str_map;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, ineffective_votes_str_map);
    if (ineffective_votes_str_map.empty()) {
        return {};
    }

    std::map<common::xaccount_address_t, std::map<common::xlogic_time_t, std::map<common::xaccount_address_t, uint64_t>>> result;
    for (auto const & p : ineffective_votes_str_map) {
        auto voter = common::xaccount_address_t{p.first};
        auto const & ineffective_votes_str = p.second;

        if (!ineffective_votes_str.empty()) {
            std::map<common::xlogic_time_t, std::map<common::xaccount_address_t, uint64_t>> all_time_ineffective_votes;
            xstream_t stream{
                xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(ineffective_votes_str.c_str())), static_cast<uint32_t>(ineffective_votes_str.size())};
            stream >> all_time_ineffective_votes;

            result.emplace(std::move(voter), std::move(all_time_ineffective_votes));
        } else {
            result.emplace(std::move(voter), std::map<common::xlogic_time_t, std::map<common::xaccount_address_t, uint64_t>>{});
        }
    }

    return result;
}

std::map<common::xaccount_address_t, uint64_t> xtable_vote_contract::table_tickets_data() const {
    std::map<std::string, std::string> adv_votes;
    try {
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("table_tickets_data fail to get XPORPERTY_CONTRACT_POLLABLE_KEY. msg %s", eh.what());
        return {};
    }

    std::map<common::xaccount_address_t, uint64_t> result;
    for (auto const & datum : adv_votes) {
        result[common::xaccount_address_t{datum.first}] = base::xstring_utl::touint64(datum.second);
    }

    return result;
}

bool xtable_vote_contract::reset_v10901(std::string const & flag,
                                        std::map<common::xaccount_address_t, vote_info_map_t> const & contract_ticket_reset_data,
                                        std::vector<common::xaccount_address_t> const & contract_ticket_clear_data) {
    auto const & contract_address = SELF_ADDRESS();
    xdbg("table %s read flag %s", contract_address.to_string().c_str(), flag.c_str());

    bool reset_touched{false};

    if (flag == flag_upload_tickets_10900 || flag == flag_withdraw_tickets_10900) {
        for (auto const & voter_and_data : contract_ticket_reset_data) {
            auto const & voter = top::get<common::xaccount_address_t const>(voter_and_data);
            if (contract_address.table_id() != voter.table_id()) {
                continue;
            }

            xdbg("reset data: table address %s tableid %" PRIu16 " voter %s tableid %" PRIu16,
                 contract_address.to_string().c_str(),
                 contract_address.table_id().value(),
                 voter.to_string().c_str(),
                 voter.table_id().value());

            reset_touched = true;

            auto const & voter_data = top::get<vote_info_map_t>(voter_and_data);

            auto property_name = calc_voter_tickets_storage_property_name(voter);

            {
                std::string old_data;
                MAP_GET2(property_name, voter.to_string(), old_data);
                if (old_data.empty()) {
                    xwarn("voter %s old data: none", voter.to_string().c_str());
                } else {
                    xstream_t old_data_stream{xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(old_data.data())), static_cast<uint32_t>(old_data.size())};
                    vote_info_map_t old_tickets_data;
                    std::string log_old_data = std::string{"voter "} + voter.to_string() + " old data: ";
                    while (old_data_stream.size() > 0) {
                        old_data_stream >> old_tickets_data;

                        for (auto const & ticket_info : old_tickets_data) {
                            log_old_data += top::get<std::string const>(ticket_info) + ":" + std::to_string(top::get<uint64_t>(ticket_info)) + ";";
                        }
                    }
                    xwarn("%s", log_old_data.c_str());
                }
            }

            // All voting data for reset user are moved into @XPORPERTY_CONTRACT_VOTES_KEYX which needs to clear corresponding XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY
            xstream_t stream{xcontext_t::instance()};
            stream << voter_data;
            std::string voter_data_str{static_cast<char const *>(reinterpret_cast<char *>(stream.data())), static_cast<size_t>(stream.size())};
            MAP_SET(property_name, voter.to_string(), voter_data_str);

            xdbg("table %s reset property %s with data %s size %zu",
                 contract_address.to_string().c_str(),
                 property_name.c_str(),
                 top::to_hex_prefixed(voter_data_str).c_str(),
                 voter_data_str.size());

            if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s",
                     contract_address.to_string().c_str(),
                     data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY,
                     voter.to_string().c_str());
                MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string());
            }
        }

        for (auto const & voter : contract_ticket_clear_data) {
            if (contract_address.table_id() != voter.table_id()) {
                continue;
            }

            xdbg("clear data: table address %s tableid %" PRIu16 " voter %s tableid %" PRIu16,
                 contract_address.to_string().c_str(),
                 contract_address.table_id().value(),
                 voter.to_string().c_str(),
                 voter.table_id().value());

            reset_touched = true;

            auto const & property_name = calc_voter_tickets_storage_property_name(voter);

            {
                std::string old_data;
                MAP_GET2(property_name, voter.to_string(), old_data);
                if (old_data.empty()) {
                    xwarn("voter %s old data: none", voter.to_string().c_str());
                } else {
                    xstream_t old_data_stream{xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(old_data.data())), static_cast<uint32_t>(old_data.size())};
                    vote_info_map_t old_tickets_data;
                    std::string log_old_data = std::string{"voter "} + voter.to_string() + " old data: ";
                    while (old_data_stream.size() > 0) {
                        old_data_stream >> old_tickets_data;

                        for (auto const & ticket_info : old_tickets_data) {
                            log_old_data += top::get<std::string const>(ticket_info) + ":" + std::to_string(top::get<uint64_t>(ticket_info)) + ";";
                        }
                    }
                    xwarn("%s", log_old_data.c_str());
                }
            }

            // All voting data for clear user are cleared from @XPORPERTY_CONTRACT_VOTES_KEYX and meanwhile need to be cleaned from corresponding
            // XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY
            if (MAP_FIELD_EXIST(property_name, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s", contract_address.to_string().c_str(), property_name.c_str(), voter.to_string().c_str());
                MAP_REMOVE(property_name, voter.to_string());
            }

            if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s",
                     contract_address.to_string().c_str(),
                     data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY,
                     voter.to_string().c_str());
                MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string());
            }
        }
    }

    return reset_touched;
}

bool xtable_vote_contract::reset_v10902(std::string const & flag,
                                        std::map<common::xaccount_address_t, vote_info_map_t> const & contract_ticket_reset_data,
                                        std::vector<common::xaccount_address_t> const & contract_ticket_clear_data) {
    auto const & contract_address = SELF_ADDRESS();
    xdbg("table %s read flag %s", contract_address.to_string().c_str(), flag.c_str());

    bool reset_touched{false};

    if (flag == flag_upload_tickets_10900 || flag == flag_withdraw_tickets_10900 || flag == flag_upload_tickets_10901 || flag == flag_withdraw_tickets_10901 ||
        flag.length() >= legacy_flag_length) {  // special check on flag length, since to-be-reset state
                                                // has flag of value with logic time type which is in
                                                // length 7. for example, 9644333.
        for (auto const & voter_and_data : contract_ticket_reset_data) {
            auto const & voter = top::get<common::xaccount_address_t const>(voter_and_data);
            if (contract_address.table_id() != voter.table_id()) {
                continue;
            }

            xdbg("reset data: table address %s tableid %" PRIu16 " voter %s tableid %" PRIu16,
                 contract_address.to_string().c_str(),
                 contract_address.table_id().value(),
                 voter.to_string().c_str(),
                 voter.table_id().value());

            reset_touched = true;

            auto const & voter_data = top::get<vote_info_map_t>(voter_and_data);

            auto property_name = calc_voter_tickets_storage_property_name(voter);

            {
                std::string old_data;
                MAP_GET2(property_name, voter.to_string(), old_data);
                if (old_data.empty()) {
                    xwarn("voter %s old data: none", voter.to_string().c_str());
                } else {
                    xstream_t old_data_stream{xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(old_data.data())), static_cast<uint32_t>(old_data.size())};
                    vote_info_map_t old_tickets_data;
                    std::string log_old_data = std::string{"voter "} + voter.to_string() + " old data: ";
                    while (old_data_stream.size() > 0) {
                        old_data_stream >> old_tickets_data;

                        for (auto const & ticket_info : old_tickets_data) {
                            log_old_data += top::get<std::string const>(ticket_info) + ":" + std::to_string(top::get<uint64_t>(ticket_info)) + ";";
                        }
                    }
                    xwarn("%s", log_old_data.c_str());
                }
            }

            // All voting data for reset user are moved into @XPORPERTY_CONTRACT_VOTES_KEYX which needs to clear corresponding XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY
            xstream_t stream{xcontext_t::instance()};
            stream << voter_data;
            std::string voter_data_str{static_cast<char const *>(reinterpret_cast<char *>(stream.data())), static_cast<size_t>(stream.size())};
            MAP_SET(property_name, voter.to_string(), voter_data_str);

            xdbg("table %s reset property %s with data %s size %zu",
                 contract_address.to_string().c_str(),
                 property_name.c_str(),
                 top::to_hex_prefixed(voter_data_str).c_str(),
                 voter_data_str.size());

            if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s",
                     contract_address.to_string().c_str(),
                     data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY,
                     voter.to_string().c_str());
                MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string());
            }
        }

        for (auto const & voter : contract_ticket_clear_data) {
            if (contract_address.table_id() != voter.table_id()) {
                continue;
            }

            xdbg("clear data: table address %s tableid %" PRIu16 " voter %s tableid %" PRIu16,
                 contract_address.to_string().c_str(),
                 contract_address.table_id().value(),
                 voter.to_string().c_str(),
                 voter.table_id().value());

            reset_touched = true;

            auto const & property_name = calc_voter_tickets_storage_property_name(voter);

            {
                std::string old_data;
                MAP_GET2(property_name, voter.to_string(), old_data);
                if (old_data.empty()) {
                    xwarn("voter %s old data: none", voter.to_string().c_str());
                } else {
                    xstream_t old_data_stream{xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(old_data.data())), static_cast<uint32_t>(old_data.size())};
                    vote_info_map_t old_tickets_data;
                    std::string log_old_data = std::string{"voter "} + voter.to_string() + " old data: ";
                    while (old_data_stream.size() > 0) {
                        old_data_stream >> old_tickets_data;

                        for (auto const & ticket_info : old_tickets_data) {
                            log_old_data += top::get<std::string const>(ticket_info) + ":" + std::to_string(top::get<uint64_t>(ticket_info)) + ";";
                        }
                    }
                    xwarn("%s", log_old_data.c_str());
                }
            }

            // All voting data for clear user are cleared from @XPORPERTY_CONTRACT_VOTES_KEYX and meanwhile need to be cleaned from corresponding
            // XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY
            if (MAP_FIELD_EXIST(property_name, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s", contract_address.to_string().c_str(), property_name.c_str(), voter.to_string().c_str());
                MAP_REMOVE(property_name, voter.to_string());
            }

            if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string())) {
                xdbg("table %s cleared property %s for voter %s",
                     contract_address.to_string().c_str(),
                     data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY,
                     voter.to_string().c_str());
                MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_INEFFECTIVE_VOTES_KEY, voter.to_string());
            }
        }
    }

    return reset_touched;
}

void xtable_vote_contract::read_tickets_property_raw_data(std::string const & property_name, std::vector<std::string> & raw_data) const {
    std::map<std::string, std::string> data;

    MAP_COPY_GET(property_name, data);
    for (auto & datum : data) {
        raw_data.emplace_back(std::move(datum.second));
    }
}

xtable_vote_contract::vote_info_map_t xtable_vote_contract::get_origin_pollable_reset_data(std::vector<std::string> const & serialized_origin_data) const {
    vote_info_map_t result;

    for (auto const & raw_auditor_tickets_data : serialized_origin_data) {
        if (raw_auditor_tickets_data.empty()) {
            continue;
        }
        vote_info_map_t detail;

        xstream_t stream{
            xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(raw_auditor_tickets_data.data())), static_cast<uint32_t>(raw_auditor_tickets_data.size())};
        stream >> detail;

        for (auto const & ticket_data : detail) {
            result[top::get<std::string const>(ticket_data)] += top::get<uint64_t>(ticket_data);
        }
    }

    return result;
}

void xtable_vote_contract::reset_table_tickets_data(vote_info_map_t const & reset_data) {
    auto const & contract_address = SELF_ADDRESS();

    MAP_CLEAR(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY);
    if (!reset_data.empty()) {
        xkinfo("table %s re-calculating property %s adv count %zu",
               contract_address.to_string().c_str(),
               data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, reset_data.size());

        for (auto const & adv_get_votes : reset_data) {
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_get_votes.first, base::xstring_utl::tostring(adv_get_votes.second));
        }
    } else {
        xkinfo("table %s clean property %s", contract_address.to_string().c_str(), data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY);
    }
}

void xtable_vote_contract::reset_table_tickets_data() {
    std::vector<std::string> raw_data;
    read_tickets_property_raw_data(data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY1, raw_data);
    read_tickets_property_raw_data(data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY2, raw_data);
    read_tickets_property_raw_data(data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY3, raw_data);
    read_tickets_property_raw_data(data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY4, raw_data);

    auto const & auditor_tickets_data = get_origin_pollable_reset_data(raw_data);
    reset_table_tickets_data(auditor_tickets_data);
}


NS_END2
