// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcluster_query_manager.h"

#include "xdata/xblocktool.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xstake/xstake_algorithm.h"
#include "xstore/xtgas_singleton.h"
#include "xutility/xhash.h"
#include "xvm/manager/xcontract_address_map.h"

using namespace top::data;

NS_BEG2(top, xrpc)
using base::xcontext_t;
using common::xversion_t;
using data::xaccount_ptr_t;
using data::xtransaction_ptr_t;
using data::xtransaction_t;
using std::string;
using std::to_string;
using store::xstore_face_t;

#define CLUSTER_REGISTER_V1_METHOD(func_name)                                                                                                                                      \
    m_query_method_map.emplace(std::pair<std::string, query_method_handler>{std::string{#func_name}, std::bind(&xcluster_query_manager::func_name, this, _1)})

xcluster_query_manager::xcluster_query_manager(observer_ptr<store::xstore_face_t> store,
                                               observer_ptr<base::xvblockstore_t> block_store,
                                               xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service)
  : m_store(store), m_block_store(block_store), m_txpool_service(txpool_service), m_bh(m_store.get(), m_block_store.get(), nullptr) {
    CLUSTER_REGISTER_V1_METHOD(getAccount);
    CLUSTER_REGISTER_V1_METHOD(getTransaction);
    CLUSTER_REGISTER_V1_METHOD(get_transactionlist);
    CLUSTER_REGISTER_V1_METHOD(get_property);
    CLUSTER_REGISTER_V1_METHOD(getBlock);
    CLUSTER_REGISTER_V1_METHOD(getChainInfo);
    CLUSTER_REGISTER_V1_METHOD(getIssuanceDetail);
    CLUSTER_REGISTER_V1_METHOD(getTimerInfo);
    CLUSTER_REGISTER_V1_METHOD(queryNodeInfo);
    CLUSTER_REGISTER_V1_METHOD(getElectInfo);
    CLUSTER_REGISTER_V1_METHOD(queryNodeReward);
    CLUSTER_REGISTER_V1_METHOD(listVoteUsed);
    CLUSTER_REGISTER_V1_METHOD(queryVoterDividend);
    CLUSTER_REGISTER_V1_METHOD(queryProposal);
    CLUSTER_REGISTER_V1_METHOD(getStandbys);
    CLUSTER_REGISTER_V1_METHOD(getCGP);
    CLUSTER_REGISTER_V1_METHOD(getLatestTables);
}

void xcluster_query_manager::call_method(xjson_proc_t & json_proc) {
    const string & method = json_proc.m_request_json["method"].asString();
    auto iter = m_query_method_map.find(method);
    if (iter != m_query_method_map.end()) {
        (iter->second)(json_proc);
    }
}

void xcluster_query_manager::getAccount(xjson_proc_t & json_proc) {
    assert(nullptr != m_store);
    const string & account = json_proc.m_request_json["params"]["account_addr"].asString();
    json_proc.m_response_json["data"] = m_bh.parse_account(account);
}

std::string xcluster_query_manager::tx_exec_status_to_str(uint8_t exec_status) {
    if (exec_status == enum_xunit_tx_exec_status_success) {
        return "success";
    } else {
        return "failure";
    }
}

void xcluster_query_manager::getTransaction(xjson_proc_t & json_proc) {
    const string account = json_proc.m_request_json["params"]["account_addr"].asString();
    const string & tx_hash_str = json_proc.m_request_json["params"]["tx_hash"].asString();
    uint256_t tx_hash = hex_to_uint256(tx_hash_str);
    xtransaction_t * tx_ptr = nullptr;
    xcons_transaction_ptr_t cons_tx_ptr = nullptr;
    if (m_txpool_service != nullptr) {
        cons_tx_ptr = m_txpool_service->query_tx(account, tx_hash);
        if (cons_tx_ptr != nullptr) {
            tx_ptr = cons_tx_ptr->get_transaction();
        }
    }
    json_proc.m_response_json["data"] = m_bh.parse_tx(tx_hash, tx_ptr);
}

void xcluster_query_manager::get_transactionlist(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    uint32_t start_num = json_proc.m_request_json["params"]["start_num"].asUInt();
    uint32_t total_num = json_proc.m_request_json["params"]["total_num"].asUInt();
    xdbg("start_num: %d, total_num: %d, account: %s", start_num, total_num, owner.c_str());

    json_proc.m_response_json["data"] = "waiting for new consensus";
}

void xcluster_query_manager::get_property(xjson_proc_t & json_proc) {
    const string & account = json_proc.m_request_json["params"]["account_addr"].asString();
    const string & type = json_proc.m_request_json["params"]["type"].asString();
    string value{};
    vector<string> value_list{};
    xJson::Value result_json;
    if (RPC_PROPERTY_STRING == type) {
        m_store->string_get(account, json_proc.m_request_json["params"]["data"].asString(), value);
    } else if (RPC_PROPERTY_LIST == type) {
        m_store->list_get_all(account, json_proc.m_request_json["params"]["data"].asString(), value_list);
    } else if (RPC_PROPERTY_MAP == type) {
        m_store->map_get(account, json_proc.m_request_json["params"]["data"][0].asString(), json_proc.m_request_json["params"]["data"][1].asString(), value);
    }

    if (!value.empty()) {
        result_json["property_value"].append(to_hex_str(value));
    } else if (!value_list.empty()) {
        for (size_t i = 0; i < value_list.size(); ++i) {
            result_json["property_value"].append(to_hex_str(value_list[i]));
        }
    } else {
        result_json["property_value"].resize(0);
    }
    // result_json["property_value"] = property_json;
    json_proc.m_response_json["data"] = result_json;
}

void xcluster_query_manager::getBlock(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    base::xvaccount_t _owner_vaddress(owner);
    xdbg("account: %s", owner.c_str());

    std::string type = "height";
    auto height = json_proc.m_request_json["params"]["height"].asString();
    xdbg("height: %s", height.c_str());
    if (height == "latest") {
        type = "last";
    }
    data::xblock_ptr_t bp{nullptr};
    xJson::Value result_json;
    if (type == "height") {
        uint64_t hi = std::stoull(height);
        xdbg("height: %llu", hi);
        auto vb = m_block_store->load_block_object(_owner_vaddress, hi, 0, true);
        xblock_t * bp = dynamic_cast<xblock_t *>(vb.get());
        result_json["value"] = m_bh.get_block_json(bp);
    } else if (type == "last") {
        auto vb = m_block_store->get_latest_committed_block(_owner_vaddress);
        xblock_t * bp = dynamic_cast<xblock_t *>(vb.get());
        result_json["value"] = m_bh.get_block_json(bp);
    }

    json_proc.m_response_json["data"] = result_json;
}

void xcluster_query_manager::getChainInfo(xjson_proc_t & json_proc) {
    xJson::Value jv;
    base::xvaccount_t _timer_vaddress(sys_contract_beacon_timer_addr);
    auto vb = m_block_store->load_block_object(_timer_vaddress, 0, 0, true);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        jv["first_timerblock_hash"] = bp->get_block_hash_hex_str();
        jv["first_timerblock_stamp"] = static_cast<xJson::UInt64>(bp->get_timestamp());
    }
    jv["init_total_locked_token"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token));
    jv["total_gas_shard"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard));
    jv["validator_group_count"] = XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_count);
    auto onchain_total_lock_tgas_token = store::xtgas_singleton::get_instance().get_cache_total_lock_tgas_token();
    jv["token_price"] = xblockchain2_t::get_token_price(onchain_total_lock_tgas_token);

    xJson::Value tj;
    m_bh.set_single_property(tj, sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
    jv["network_activate_time"] = tj[xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY]["activation_time"].asUInt64();

    json_proc.m_response_json["data"] = jv;
}

void xcluster_query_manager::getIssuanceDetail(xjson_proc_t & json_proc) {
    auto get_zec_workload_map = [&](observer_ptr<store::xstore_face_t> store,
                                                 common::xaccount_address_t const & contract_address,
                                                 std::string const & property_name,
                                                 uint64_t height,
                                                 xJson::Value & json) {
        std::map<std::string, std::string> workloads;
        if (store->get_map_property(contract_address.value(), height - 1, property_name, workloads) != 0) {
            xwarn("[xcluster_query_manager::getIssuanceDetail] get_zec_workload_map contract_address: %s, height: %llu, property_name: %s",
                contract_address.value().c_str(),
                height,
                property_name.c_str());
            return;
        }

        xdbg("[xcluster_query_manager::getIssuanceDetail] get_zec_workload_map contract_address: %s, height: %llu, property_name: %s, workloads size: %d",
                contract_address.value().c_str(),
                height,
                property_name.c_str(),
                workloads.size());
        //if (store->map_copy_get(contract_address.value(), property_name, workloads) != 0) return;
        xJson::Value jm;
        for (auto m : workloads) {
            auto detail = m.second;
            base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
            xstake::cluster_workload_t workload;
            workload.serialize_from(stream);
            xJson::Value jn;
            jn["cluster_total_workload"] = workload.cluster_total_workload;
            auto const & key_str = workload.cluster_id;
            common::xcluster_address_t cluster;
            base::xstream_t key_stream(xcontext_t::instance(), (uint8_t *)key_str.data(), key_str.size());
            key_stream >> cluster;
            for (auto node : workload.m_leader_count) {
                jn[node.first] = node.second;
            }
            jm[cluster.group_id().to_string()] = jn;
        }
        json[property_name] = jm;
    };

    uint64_t height = json_proc.m_request_json["params"]["height"].asUInt64();
    if (height == 0) {
        xwarn("[xcluster_query_manager::getIssuanceDetail] height: %llu", height);
        return;
    }

    xJson::Value j;

    std::string xissue_detail_str;
    if (m_store->get_string_property(sys_contract_zec_reward_addr, height, xstake::XPROPERTY_REWARD_DETAIL, xissue_detail_str) != 0) {
        xwarn("[xcluster_query_manager::getIssuanceDetail] contract_address: %s, height: %llu, property_name: %s",
            sys_contract_zec_reward_addr,
            height,
            xstake::XPROPERTY_REWARD_DETAIL);
        return;
    }
    xstake::xissue_detail issue_detail;
    issue_detail.from_string(xissue_detail_str);
    xdbg("[xcluster_query_manager::getIssuanceDetail] reward contract height: %llu, onchain_timer_round: %llu, m_zec_vote_contract_height: %llu, "
    "m_zec_workload_contract_height: %llu, m_zec_reward_contract_height: %llu, "
    "m_edge_reward_ratio: %u, m_archive_reward_ratio: %u, "
    "m_validator_reward_ratio: %u, m_auditor_reward_ratio: %u, m_vote_reward_ratio: %u, m_governance_reward_ratio: %u",
        height,
        issue_detail.onchain_timer_round,
        issue_detail.m_zec_vote_contract_height,
        issue_detail.m_zec_workload_contract_height,
        issue_detail.m_zec_reward_contract_height,
        issue_detail.m_edge_reward_ratio,
        issue_detail.m_archive_reward_ratio,
        issue_detail.m_validator_reward_ratio,
        issue_detail.m_auditor_reward_ratio,
        issue_detail.m_vote_reward_ratio,
        issue_detail.m_governance_reward_ratio);
    xJson::Value jv;
    jv["onchain_timer_round"]           = (xJson::UInt64)issue_detail.onchain_timer_round;
    jv["zec_vote_contract_height"]      = (xJson::UInt64)issue_detail.m_zec_vote_contract_height;
    jv["zec_workload_contract_height"]  = (xJson::UInt64)issue_detail.m_zec_workload_contract_height;
    jv["zec_reward_contract_height"]    = (xJson::UInt64)issue_detail.m_zec_reward_contract_height;
    jv["edge_reward_ratio"]             = issue_detail.m_edge_reward_ratio;
    jv["archive_reward_ratio"]          = issue_detail.m_archive_reward_ratio;
    jv["validator_reward_ratio"]        = issue_detail.m_validator_reward_ratio;
    jv["auditor_reward_ratio"]          = issue_detail.m_auditor_reward_ratio;
    jv["vote_reward_ratio"]             = issue_detail.m_vote_reward_ratio;
    jv["governance_reward_ratio"]       = issue_detail.m_governance_reward_ratio;
    jv["validator_group_count"]         = (xJson::UInt)issue_detail.m_validator_group_count;
    jv["auditor_group_count"]           = (xJson::UInt)issue_detail.m_auditor_group_count;
    std::map<std::string, std::string> contract_auditor_votes;
    if (m_store->get_map_property(sys_contract_zec_vote_addr, issue_detail.m_zec_vote_contract_height, xstake::XPORPERTY_CONTRACT_TICKETS_KEY, contract_auditor_votes) != 0) {
        xwarn("[xcluster_query_manager::getIssuanceDetail] contract_address: %s, height: %llu, property_name: %s",
            sys_contract_zec_vote_addr,
            issue_detail.m_zec_vote_contract_height,
            xstake::XPORPERTY_CONTRACT_TICKETS_KEY);
    }
    xJson::Value jvt;
    for (auto const & table_vote : contract_auditor_votes) {
        auto const & contract = table_vote.first;
        auto const & auditor_votes_str = table_vote.second;
        xJson::Value jvt1;
        std::map<std::string, std::string> auditor_votes;
        base::xstream_t stream(xcontext_t::instance(), (uint8_t *)auditor_votes_str.data(), auditor_votes_str.size());
        stream >> auditor_votes;
        for (auto const & node_vote: auditor_votes) {
            jvt1[node_vote.first] = (xJson::UInt64)base::xstring_utl::touint64(node_vote.second);
        }
        jvt[contract] = jvt1;
    }
    jv["table_votes"] = jvt;
    xJson::Value jw1;
    common::xaccount_address_t contract_addr{sys_contract_zec_reward_addr};
    std::string prop_name = xstake::XPORPERTY_CONTRACT_WORKLOAD_KEY;
    get_zec_workload_map(m_store, contract_addr, prop_name, issue_detail.m_zec_reward_contract_height + 1, jw1);
    jv["auditor_workloads"] = jw1[prop_name];
    xJson::Value jw2;
    prop_name = xstake::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY;
    get_zec_workload_map(m_store, contract_addr, prop_name, issue_detail.m_zec_reward_contract_height + 1, jw2);
    jv["validator_workloads"] = jw2[prop_name];
    xJson::Value jr;
    for (auto const & node_reward : issue_detail.m_node_rewards) {
        std::stringstream ss;
        ss  << "edge_reward: " << static_cast<uint64_t>(node_reward.second.m_edge_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_edge_reward % xstake::REWARD_PRECISION)
            << ", archive_reward: " << static_cast<uint64_t>(node_reward.second.m_archive_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_archive_reward % xstake::REWARD_PRECISION)
            << ", validator_reward: " << static_cast<uint64_t>(node_reward.second.m_validator_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_validator_reward % xstake::REWARD_PRECISION)
            << ", auditor_reward: " << static_cast<uint64_t>(node_reward.second.m_auditor_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_auditor_reward % xstake::REWARD_PRECISION)
            << ", voter_reward: " << static_cast<uint64_t>(node_reward.second.m_vote_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_vote_reward % xstake::REWARD_PRECISION)
            << ", self_reward: " << static_cast<uint64_t>(node_reward.second.m_self_reward / xstake::REWARD_PRECISION)
            << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_self_reward % xstake::REWARD_PRECISION);
        jr[node_reward.first] = ss.str();
    }
    jv["node_rewards"] = jr;
    std::stringstream ss;
    ss << std::setw(40) << std::setfill('0') << issue_detail.m_zec_reward_contract_height + 1;
    auto key = ss.str();
    j[key] = jv;

    json_proc.m_response_json["data"] = j;
}

void xcluster_query_manager::getTimerInfo(xjson_proc_t & json_proc) {
    chain_info::get_block_handle bh(m_store.get(), m_block_store.get(), nullptr);
    xJson::Value jv;
    jv["timer_height"] = static_cast<xJson::UInt64>(bh.get_timer_clock());
    json_proc.m_response_json["data"] = jv;
}

void xcluster_query_manager::queryNodeInfo(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    xdbg("account: %s, target: %s", owner.c_str(), target.c_str());

    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_registration_addr;
    std::string prop_name = xstake::XPORPERTY_CONTRACT_REG_KEY;
    m_bh.set_single_property(jv, contract_addr, prop_name);

    if (target == "") {
        json_proc.m_response_json["data"] = jv[prop_name];
    } else {
        json_proc.m_response_json["data"] = jv[prop_name][target];
    }
}

void xcluster_query_manager::getElectInfo(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    xdbg("account: %s, target: %s", owner.c_str(), target.c_str());

    std::vector<std::string> ev;
    xJson::Value j;

    std::string addr = sys_contract_zec_elect_consensus_addr;
    auto property_names = top::data::election::get_property_name_by_addr(common::xaccount_address_t{addr});
    for (auto property : property_names) {
        m_bh.set_single_native_property(j, addr, property);
        if (j["auditor"].isMember(target)) {
            ev.push_back("auditor");
        }
        if (j["validator"].isMember(target)) {
            ev.push_back("validator");
        }
    }

    addr = sys_contract_rec_elect_archive_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    m_bh.set_single_native_property(j, addr, prop_name);
    if (j["archive"].isMember(target)) {
        ev.push_back("archiver");
    }

    addr = sys_contract_rec_elect_edge_addr;
    prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    m_bh.set_single_native_property(j, addr, prop_name);
    if (j["edge"].isMember(target)) {
        ev.push_back("edger");
    }

    std::string elect_info;
    if (ev.empty()) {
        elect_info = "Now it is not elected as any node role.";
    } else {
        elect_info = "Now it is elected as ";
        for (size_t i = 0; i < ev.size(); ++i) {
            elect_info = elect_info + ev[i];
            if (i == ev.size() - 1) {
                elect_info = elect_info + ".";
            } else {
                elect_info = elect_info + ", ";
            }
        }
    }
    json_proc.m_response_json["data"] = elect_info;
}

void xcluster_query_manager::set_sharding_vote_prop(xjson_proc_t & json_proc, std::string & prop_name) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();

    xJson::Value jv;

    auto const & table_id = data::account_map_to_table_id(common::xaccount_address_t{target}).get_subaddr();
    auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_vote_addr}, table_id);
    xdbg("account: %s, target: %s, addr: %s, prop: %s", owner.c_str(), target.c_str(), shard_reward_addr.c_str(), prop_name.c_str());
    m_bh.set_single_property(jv, shard_reward_addr.value(), prop_name);

    if (target == "") {
        json_proc.m_response_json["data"] = jv[prop_name];
    } else {
        json_proc.m_response_json["data"] = jv[prop_name][target];
    }
}

void xcluster_query_manager::set_sharding_reward_claiming_prop(xjson_proc_t & json_proc, std::string & prop_name) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();

    xJson::Value jv = m_bh.parse_sharding_reward(target, prop_name);
    json_proc.m_response_json["data"] = jv;
}

void xcluster_query_manager::queryNodeReward(xjson_proc_t & json_proc) {
    std::string prop_name = xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY;
    set_sharding_reward_claiming_prop(json_proc, prop_name);
}

void xcluster_query_manager::listVoteUsed(xjson_proc_t & json_proc) {
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    uint32_t sub_map_no = (utl::xxh32_t::digest(target) % 4) + 1;
    std::string prop_name;
    prop_name = prop_name + xstake::XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(sub_map_no);
    set_sharding_vote_prop(json_proc, prop_name);
}

void xcluster_query_manager::queryVoterDividend(xjson_proc_t & json_proc) {
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    uint32_t sub_map_no = (utl::xxh32_t::digest(target) % 4) + 1;
    std::string prop_name;
    prop_name = prop_name + xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);
    set_sharding_reward_claiming_prop(json_proc, prop_name);
}

void xcluster_query_manager::queryProposal(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    std::string proposal_version = json_proc.m_request_json["params"]["proposal_version"].asString();
    xdbg("account: %s, target: %s, proposal_version: %s", owner.c_str(), target.c_str(), proposal_version.c_str());

    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_tcc_addr;
    std::string prop_name = PROPOSAL_MAP_ID;
    m_bh.set_single_property(jv, contract_addr, prop_name);

    if (target == "") {
        json_proc.m_response_json["data"] = jv[prop_name];
    } else {
        target = "proposal_id_" + target;
        json_proc.m_response_json["data"] = jv[prop_name][target];
    }
}

void xcluster_query_manager::getStandbys(xjson_proc_t & json_proc) {
    std::string target = json_proc.m_request_json["params"]["node_account_addr"].asString();
    xdbg("target: %s", target.c_str());

    xJson::Value jv;
    std::string addr = sys_contract_rec_standby_pool_addr;
    std::string prop_name = XPROPERTY_CONTRACT_STANDBYS_KEY;
    m_bh.set_single_native_property(jv, addr, prop_name);
    if (target == "") {
        json_proc.m_response_json["data"] = jv;
    } else {
        for (auto j1 : jv.getMemberNames()) {
            auto j2 = jv[j1];
            for (auto j3 : j2) {
                if (j3["node_id"].asString() == target) {
                    json_proc.m_response_json["data"] = j3;
                }
            }
        }
    }
}

void xcluster_query_manager::getCGP(xjson_proc_t & json_proc) {
    xJson::Value jv;
    std::string addr = sys_contract_rec_tcc_addr;
    std::string prop_name = ONCHAIN_PARAMS;
    m_bh.set_single_property(jv, addr, prop_name);
    json_proc.m_response_json["data"] = jv[prop_name];
}

void xcluster_query_manager::getLatestTables(xjson_proc_t & json_proc) {
    std::string owner = json_proc.m_request_json["params"]["account_addr"].asString();
    xdbg("getLatestTables account: %s", owner.c_str());

    xJson::Value jv;
    for(auto i = 0; i < enum_vbucket_has_tables_count; ++i) {
        std::string addr = xblocktool_t::make_address_shard_table_account(i);
        auto vb = m_block_store->get_latest_committed_block(addr);
        jv.append(static_cast<xJson::UInt64>(vb->get_height()));
        xdbg("getLatestTables addr %s, height %ull", addr.c_str(), vb->get_height());
    }

    json_proc.m_response_json["data"] = jv;
}
NS_END2
