#include "get_block.h"

#include "xbase/xbase.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xbasic/xutility.h"
#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xdata/xslash.h"
#include "xdata/xtableblock.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xfull_tableblock.h"
#include "xrouter/xrouter.h"
#include "xrpc/xuint_format.h"
#include "xstake/xstake_algorithm.h"
#include "xstore/xaccount_context.h"
#include "xstore/xtgas_singleton.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"

#include <cstdint>
#include <iostream>

using namespace top::data;

namespace top {

namespace chain_info {

using namespace std;
using namespace base;
using namespace store;
using namespace xrpc;

bool get_block_handle::handle(std::string request) {
    m_js_req.clear();
    m_js_rsp.clear();

    xJson::Reader reader;
    set_result("ok");
    if (!reader.parse(request, m_js_req)) {
        set_result("json parse error");
        return true;
    }

    std::string action = m_js_req["action"].asString();
    auto iter = m_query_method_map.find(action);
    if (iter != m_query_method_map.end()) {
        iter->second();
    } else {
        xdbg("get_block action %s nonexist!", action.c_str());
        return false;
    }

    return true;
}

void get_block_handle::getAccount() {
    std::string account = m_js_req["account_addr"].asString();
    if (account.empty()) {
        xwarn("getAccount:account is empty ");
        return;
    }  
    try {
        m_js_rsp["value"] = parse_account(account);
    } catch (exception & e) {
        set_result(std::string(e.what()));
    }
}

xJson::Value get_block_handle::parse_account(const std::string & account) {
    xJson::Value result_json;
    xaccount_ptr_t account_ptr = m_store->query_account(account);
    if (account_ptr != nullptr) {
        // string freeze_fee{};
        result_json["account_addr"] = account;
        result_json["created_time"] = static_cast<xJson::UInt64>(account_ptr->get_account_create_time());
        result_json["balance"] = static_cast<xJson::UInt64>(account_ptr->balance());
        result_json["gas_staked_token"] = static_cast<xJson::UInt64>(account_ptr->tgas_balance());
        result_json["disk_staked_token"] = static_cast<xJson::UInt64>(account_ptr->disk_balance());
        result_json["vote_staked_token"] = static_cast<xJson::UInt64>(account_ptr->vote_balance());
        result_json["lock_balance"] = static_cast<xJson::UInt64>(account_ptr->lock_balance());
        // result_json["lock_balance"] = static_cast<xJson::UInt64>(account_ptr->lock_balance());
        // result_json["lock_gas"] = static_cast<xJson::UInt64>(account_ptr->lock_tgas());
        result_json["burned_token"] = static_cast<xJson::UInt64>(account_ptr->burn_balance());
        result_json["unused_vote_amount"] = static_cast<xJson::UInt64>(account_ptr->unvote_num());
        result_json["nonce"] = static_cast<xJson::UInt64>(account_ptr->account_send_trans_number());
        uint256_t last_hash = account_ptr->account_send_trans_hash();
        result_json["latest_tx_hash"] = uint_to_str(last_hash.data(), last_hash.size());
        uint64_t last_hash_xxhash64 = static_cast<xJson::UInt64>(utl::xxh64_t::digest(last_hash.data(), last_hash.size()));
        result_json["latest_tx_hash_xxhash64"] = uint64_to_str(last_hash_xxhash64);
        result_json["latest_unit_height"] = static_cast<xJson::UInt64>(account_ptr->get_chain_height());
        result_json["recv_tx_num"] = static_cast<xJson::UInt64>(account_ptr->account_recv_trans_number());

        auto timer_height = get_timer_height();
        auto onchain_total_lock_tgas_token = xtgas_singleton::get_instance().get_cache_total_lock_tgas_token();
        auto token_price = account_ptr->get_token_price(onchain_total_lock_tgas_token);
        auto total_gas = account_ptr->get_total_tgas(token_price);
        auto available_gas = account_ptr->get_available_tgas(timer_height, token_price);
        auto free_gas = account_ptr->get_free_tgas();
        auto used_gas = account_ptr->calc_decayed_tgas(timer_height);
        uint64_t unused_free_gas = 0;
        uint64_t unused_stake_gas = 0;
        if (free_gas > used_gas) {
            unused_free_gas = free_gas - used_gas;
            unused_stake_gas = total_gas - free_gas;
        } else {
            unused_stake_gas = available_gas;
        }
        result_json["unused_free_gas"] = static_cast<xJson::UInt64>(unused_free_gas);
        result_json["unused_stake_gas"] = static_cast<xJson::UInt64>(unused_stake_gas);
        result_json["available_gas"] = static_cast<xJson::Int64>(total_gas - used_gas);
        result_json["total_gas"] = static_cast<xJson::UInt64>(total_gas);
        result_json["total_free_gas"] = static_cast<xJson::UInt64>(free_gas);
        result_json["total_stake_gas"] = static_cast<xJson::UInt64>(total_gas - free_gas);

        set_redeem_token_num(account_ptr, result_json);

        result_json["table_id"] = account_map_to_table_id(common::xaccount_address_t{account}).get_subaddr();
        common::xnetwork_id_t nid{0};
        std::shared_ptr<router::xrouter_face_t> router = std::make_shared<router::xrouter_t>();
        vnetwork::xcluster_address_t addr = router->sharding_address_from_account(common::xaccount_address_t{account}, nid, common::xnode_type_t::consensus_validator);
        result_json["zone_id"] = addr.zone_id().value();
        result_json["cluster_id"] = addr.cluster_id().value();
        result_json["group_id"] = addr.group_id().value();

        const std::string prop_name(XPROPERTY_PLEDGE_VOTE_KEY);
        query_account_property(result_json, account, prop_name);
        result_json["vote_staked_index"] = result_json[XPROPERTY_PLEDGE_VOTE_KEY];
        result_json.removeMember(XPROPERTY_PLEDGE_VOTE_KEY);
        return result_json;
    } else {
        throw xrpc::xrpc_error{xrpc::enum_xrpc_error_code::rpc_shard_exec_error, "account not found on chain"};
    }
}

void get_block_handle::getGeneralInfos() {
    xJson::Value j;
    j["shard_num"] = XGET_CONFIG(validator_group_count);
    j["shard_gas"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard));
    j["init_pledge_token"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token));
    j["genesis_time"] = static_cast<xJson::UInt64>(xrootblock_t::get_rootblock()->get_cert()->get_gmtime());
    auto onchain_total_lock_tgas_token = xtgas_singleton::get_instance().get_cache_total_lock_tgas_token();
    j["token_price"] = xunit_bstate_t::get_token_price(onchain_total_lock_tgas_token);
    std::map<std::string, std::string> ms;
    m_store->map_copy_get(sys_contract_zec_reward_addr, xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, ms);
    if (!ms.empty()) {
        j["total_issuance"] = (xJson::UInt64)base::xstring_utl::touint64(ms["total"]);
    }
    m_js_rsp["value"] = j;
}

void get_block_handle::getRootblockInfo() {
    xJson::Value j;
    xrootblock_t::get_rootblock_data(j);
    m_js_rsp["value"] = j;
}

uint64_t get_block_handle::get_timer_height() const {
    auto vb = m_block_store->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_rpc_get_cert_blk);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        return bp->get_height();
    } else {
        return 0;
    }
}

void get_block_handle::getCGP() {
    xJson::Value j;
    std::string addr = sys_contract_rec_tcc_addr;
    std::string prop_name = ONCHAIN_PARAMS;
    query_account_property(j, addr, prop_name);
    m_js_rsp["value"] = j[prop_name];
}

void get_block_handle::getTimerInfo() {
    xJson::Value j;
    auto timer_clock = get_timer_clock();
    if (timer_clock != 0) {
        j["timer_clock"] = static_cast<xJson::UInt64>(timer_clock);
    }
    auto timer_height = get_timer_height();
    if (timer_height != 0) {
        j["timer_height"] = static_cast<xJson::UInt64>(timer_height);
    }
    m_js_rsp["value"] = j;
}

void get_block_handle::getIssuanceDetail() {
    auto get_zec_workload_map =
        [&](store::xstore_face_t * store, common::xaccount_address_t const & contract_address, std::string const & property_name, uint64_t height, xJson::Value & json) {
            std::map<std::string, std::string> workloads;
            if (store->get_map_property(contract_address.value(), height - 1, property_name, workloads) != 0) {
                xwarn("[grpc::getIssuanceDetail] get_zec_workload_map contract_address： %s, height: %llu, property_name: %s",
                      contract_address.value().c_str(),
                      height,
                      property_name.c_str());
                return;
            }

            xdbg("[grpc::getIssuanceDetail] get_zec_workload_map contract_address： %s, height: %llu, property_name: %s, workloads size: %d",
                 contract_address.value().c_str(),
                 height,
                 property_name.c_str(),
                 workloads.size());
            // if (store->map_copy_get(contract_address.value(), property_name, workloads) != 0) return;
            xJson::Value jm;
            for (auto m : workloads) {
                auto detail = m.second;
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                xstake::cluster_workload_t workload;
                workload.serialize_from(stream);
                xJson::Value jn;
                jn["cluster_total_workload"] = workload.cluster_total_workload;
                auto const & key_str = m.first;
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

    uint64_t height = m_js_req["height"].asUInt64();
    if (height == 0) {
        xwarn("[grpc::getIssuanceDetail] height: %llu", height);
        return;
    }

    xJson::Value j;

    std::string xissue_detail_str;
    if (m_store->get_string_property(sys_contract_zec_reward_addr, height, xstake::XPROPERTY_REWARD_DETAIL, xissue_detail_str) != 0) {
        xwarn("[grpc::getIssuanceDetail] contract_address： %s, height: %llu, property_name: %s", sys_contract_zec_reward_addr, height, xstake::XPROPERTY_REWARD_DETAIL);
        return;
    }
    xstake::xissue_detail issue_detail;
    if (issue_detail.from_string(xissue_detail_str) <= 0) {
        xwarn("[grpc::getIssuanceDetail] deserialize failed");
    }

    xdbg(
        "[grpc::getIssuanceDetail] reward contract height: %llu, onchain_timer_round: %llu, m_zec_vote_contract_height: %llu, "
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
    jv["onchain_timer_round"] = (xJson::UInt64)issue_detail.onchain_timer_round;
    jv["zec_vote_contract_height"] = (xJson::UInt64)issue_detail.m_zec_vote_contract_height;
    jv["zec_workload_contract_height"] = (xJson::UInt64)issue_detail.m_zec_workload_contract_height;
    jv["zec_reward_contract_height"] = (xJson::UInt64)issue_detail.m_zec_reward_contract_height;
    jv["edge_reward_ratio"] = issue_detail.m_edge_reward_ratio;
    jv["archive_reward_ratio"] = issue_detail.m_archive_reward_ratio;
    jv["validator_reward_ratio"] = issue_detail.m_validator_reward_ratio;
    jv["auditor_reward_ratio"] = issue_detail.m_auditor_reward_ratio;
    jv["vote_reward_ratio"] = issue_detail.m_vote_reward_ratio;
    jv["governance_reward_ratio"] = issue_detail.m_governance_reward_ratio;
    jv["validator_group_count"] = (xJson::UInt)issue_detail.m_validator_group_count;
    jv["auditor_group_count"] = (xJson::UInt)issue_detail.m_auditor_group_count;
    std::map<std::string, std::string> contract_auditor_votes;
    if (m_store->get_map_property(sys_contract_zec_vote_addr, issue_detail.m_zec_vote_contract_height, xstake::XPORPERTY_CONTRACT_TICKETS_KEY, contract_auditor_votes) != 0) {
        xwarn("[grpc::getIssuanceDetail] contract_address： %s, height: %llu, property_name: %s",
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
        for (auto const & node_vote : auditor_votes) {
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
        ss << "edge_reward: " << static_cast<uint64_t>(node_reward.second.m_edge_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_edge_reward % xstake::REWARD_PRECISION)
           << ", archive_reward: " << static_cast<uint64_t>(node_reward.second.m_archive_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_archive_reward % xstake::REWARD_PRECISION)
           << ", validator_reward: " << static_cast<uint64_t>(node_reward.second.m_validator_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_validator_reward % xstake::REWARD_PRECISION)
           << ", auditor_reward: " << static_cast<uint64_t>(node_reward.second.m_auditor_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_auditor_reward % xstake::REWARD_PRECISION)
           << ", voter_reward: " << static_cast<uint64_t>(node_reward.second.m_vote_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_vote_reward % xstake::REWARD_PRECISION)
           << ", self_reward: " << static_cast<uint64_t>(node_reward.second.m_self_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_self_reward % xstake::REWARD_PRECISION);
        jr[node_reward.first] = ss.str();
    }
    jv["node_rewards"] = jr;
    std::stringstream ss;
    ss << std::setw(40) << std::setfill('0') << issue_detail.m_zec_reward_contract_height + 1;
    auto key = ss.str();
    j[key] = jv;

    m_js_rsp["data"] = j;
}

void get_block_handle::getWorkloadDetail() {
    uint64_t height = m_js_req["height"].asUInt64();
    if (height == 0) {
        xwarn("[grpc::getWorkloadDetail] height: %llu", height);
        return;
    }

    std::error_code ec;
    xJson::Value value;
    top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{ sys_contract_zec_reward_addr }, height, top::contract::xjson_format_t::detail, value, ec);
    if (ec) {
        value["query_status"] = ec.message();
    }
    m_js_rsp["value"] = value;
}

uint64_t get_block_handle::get_timer_clock() const {
    auto vb = m_block_store->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_rpc_get_timer_clock);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        return bp->get_clock();
    } else {
        return 0;
    }
}

void get_block_handle::parse_run_contract(xJson::Value & j, const xaction_t & action) {
    if (is_sys_contract_address(common::xaccount_address_t{action.get_account_addr()})) {
        j["paras"] = to_hex_str(action.get_action_param());
        // xdbg("get_block shard contract: %s", source_action.get_account_addr().c_str());
        return;
    }
    xJson::Value jvas;
    if (!action.get_action_param().empty()) {
        xstream_t stream(xcontext_t::instance(), (uint8_t *)action.get_action_param().data(), action.get_action_param().size());
        uint8_t arg_num{0}, arg_type{0};
        uint64_t arg_uint64{0};
        string arg_str;
        bool arg_bool;
        stream >> arg_num;
        xdbg("get_block arg_num %d", arg_num);
        while (arg_num--) {
            stream >> arg_type;
            xdbg("get_block arg_type %d", arg_type);
            xJson::Value jva;
            switch (arg_type) {
            case ARG_TYPE_UINT64:
                stream >> arg_uint64;
                xdbg("get_block arg_uint64 %d", arg_uint64);
                jva[std::to_string(arg_num)] = static_cast<unsigned long long>(arg_uint64);
                jvas.append(jva);
                break;
            case ARG_TYPE_STRING:
                stream >> arg_str;
                xdbg("get_block arg_str %s", arg_str.c_str());
                jva[std::to_string(arg_num)] = arg_str;
                jvas.append(jva);
                break;
            case ARG_TYPE_BOOL:
                stream >> arg_bool;
                xdbg("get_block arg_bool %d", arg_bool);
                jva[std::to_string(arg_num)] = arg_bool;
                jvas.append(jva);
                break;
            default:
                jvas["err"] = "type error";
                break;
            }
        }
    }
    j["paras"] = jvas;
}

void get_block_handle::set_alias_name_info(xJson::Value & j, const xaction_t & action) {
    j["name"] = action.get_action_param();
}

void get_block_handle::parse_create_contract_account(xJson::Value & j, const xaction_t & action) {
    xaction_deploy_contract deploy;
    deploy.parse(action);
    j["tgas_limit"] = static_cast<unsigned long long>(deploy.m_tgas_limit);
    j["code"] = deploy.m_code;
}

void get_block_handle::set_account_keys_info(xJson::Value & j, const xaction_t & action) {
    xaction_set_account_keys set_account;
    set_account.parse(action);
    j["account_key"] = set_account.m_account_key;
    j["key_value"] = set_account.m_key_value;
}

void get_block_handle::set_lock_token_info(xJson::Value & j, const xaction_t & action) {
    xaction_lock_account_token lock_token;
    lock_token.parse(action);
    j["amount"] = static_cast<unsigned long long>(lock_token.m_amount);
    j["params"] = to_hex_str(lock_token.m_params);
    j["unlock_type"] = lock_token.m_unlock_type;
    for (auto v : lock_token.m_unlock_values) {
        j["unlock_values"].append(v);
    }
    j["version"] = lock_token.m_version;
}

void get_block_handle::set_unlock_token_info(xJson::Value & j, const xaction_t & action) {
    xaction_unlock_account_token unlock_token;
    unlock_token.parse(action);
    j["lock_tran_hash"] = unlock_token.m_lock_tran_hash;
    for (auto s : unlock_token.m_signatures) {
        j["signatures"].append(s);
    }
    j["version"] = unlock_token.m_version;
}

void get_block_handle::set_create_sub_account_info(xJson::Value & j, const xaction_t & action) {
}

xJson::Value get_block_handle::get_tx_exec_result(const std::string & account, uint64_t block_height, xtransaction_ptr_t tx_ptr, xlightunit_tx_info_ptr_t & recv_txinfo, const std::string & rpc_version) {
    xJson::Value jv;
    if (account.empty()) {
        return jv;
    }

    base::xvaccount_t _account_vaddress(account);
    auto vb = m_block_store->load_block_object(_account_vaddress, block_height, 0, true, metrics::blockstore_access_from_rpc_get_unit);
    auto block_ptr = dynamic_cast<xblock_t *>(vb.get());
    if (block_ptr == nullptr) {
        throw xrpc_error{enum_xrpc_error_code::rpc_shard_exec_error, "account address does not exist or block height does not exist"};
    }

    jv["height"] = static_cast<xJson::UInt64>(block_height);
    if (rpc_version == RPC_VERSION_V2) {
        jv["account"] = account;
    }
    auto tx_info = block_ptr->get_tx_info(tx_ptr->get_digest_str());
    if (tx_info != nullptr) {
        jv["used_gas"] = tx_info->get_used_tgas();
        if (tx_info->is_self_tx()) {
            jv["exec_status"] = xtransaction_t::tx_exec_status_to_str(tx_info->get_tx_exec_status());
            jv["used_deposit"] = tx_info->get_used_deposit();
        }
        if (tx_info->is_confirm_tx()) {
            jv["used_deposit"] = tx_info->get_used_deposit();
            // TODO(jimmy) should read recv tx exec status from recv tx unit
            if (recv_txinfo != nullptr) {
                jv["recv_tx_exec_status"] = xtransaction_t::tx_exec_status_to_str(recv_txinfo->get_tx_exec_status());
                jv["exec_status"] = xtransaction_t::tx_exec_status_to_str(tx_info->get_tx_exec_status() | recv_txinfo->get_tx_exec_status());
            }
        }
        if (tx_info->is_recv_tx()) {
            recv_txinfo = tx_info;  // TODO(jimmy) refactor here future
        }
    }
    return jv;
}

xJson::Value get_block_handle::parse_tx(xtransaction_t * tx_ptr, const std::string & version) {
    xJson::Value ori_tx_info;
    if (tx_ptr == nullptr) {
        return ori_tx_info;
    }
    tx_ptr->parse_to_json(ori_tx_info, version);
    ori_tx_info["authorization"] = uint_to_str(tx_ptr->get_authorization().data(), tx_ptr->get_authorization().size());
    if (version == RPC_VERSION_V1) {
        ori_tx_info["tx_action"]["sender_action"] = ori_tx_info["sender_action"];
        ori_tx_info.removeMember("sender_action");
        ori_tx_info["tx_action"]["receiver_action"] = ori_tx_info["receiver_action"];
        ori_tx_info.removeMember("receiver_action");
        // for sys shard addr, the account must return with table id suffix
        ori_tx_info["tx_action"]["receiver_action"]["tx_receiver_account_addr"] = tx_ptr->get_target_addr();
    } else {
        ori_tx_info["receiver_account"] = tx_ptr->get_target_addr();
    }
    return ori_tx_info;
}

void get_block_handle::update_tx_state(xJson::Value & result_json, const xJson::Value & cons, const std::string & rpc_version) {
    const xtx_exec_json_key jk(rpc_version);
    if (cons[jk.m_confirm]["exec_status"].asString() == "success") {
        result_json["tx_state"] = "success";
    } else if (cons[jk.m_confirm]["exec_status"].asString() == "failure") {
        result_json["tx_state"] = "fail";
    } else if (cons[jk.m_send]["height"].asUInt64() > 0) {
        result_json["tx_state"] = "queue";
    } else {
        result_json["tx_state"] = "pending";
    }
}

xJson::Value get_block_handle::parse_tx(const uint256_t & tx_hash, xtransaction_t * cons_tx_ptr, const std::string & rpc_version) {
    std::string tx_hash_str = std::string(reinterpret_cast<char*>(tx_hash.data()), tx_hash.size());
    base::xvtransaction_store_ptr_t tx_store_ptr = m_block_store->query_tx(tx_hash_str, base::enum_transaction_subtype_all);
    xJson::Value result_json;
    xJson::Value cons;
    if (tx_store_ptr != nullptr && tx_store_ptr->get_raw_tx() != nullptr) {
        xtransaction_ptr_t tx_ptr;
        auto tx = dynamic_cast<xtransaction_t*>(tx_store_ptr->get_raw_tx());
        tx->add_ref();
        tx_ptr.attach(tx);
        
        const xtx_exec_json_key jk(rpc_version);
        xlightunit_tx_info_ptr_t recv_txinfo = nullptr;
        // burn tx & self tx only 1 consensus
        if (tx_ptr->get_target_addr() != black_hole_addr && (tx_ptr->get_source_addr() != tx_ptr->get_target_addr())) {
            cons[jk.m_send] = get_tx_exec_result(tx_store_ptr->get_send_addr(), tx_store_ptr->get_send_block_height(), tx_ptr, recv_txinfo, rpc_version);
            auto beacon_tx_fee = txexecutor::xtransaction_fee_t::cal_service_fee(tx_ptr->get_source_addr(), tx_ptr->get_target_addr());
            cons[jk.m_send]["tx_fee"] = static_cast<xJson::UInt64>(beacon_tx_fee);
            cons[jk.m_recv] = get_tx_exec_result(tx_store_ptr->get_recv_addr(), tx_store_ptr->get_recv_block_height(), tx_ptr, recv_txinfo, rpc_version);
        }
        cons[jk.m_confirm] = get_tx_exec_result(tx_store_ptr->get_send_addr(), tx_store_ptr->get_confirm_block_height(), tx_ptr, recv_txinfo, rpc_version);
        result_json["tx_consensus_state"] = cons;

        update_tx_state(result_json, cons, rpc_version);

        auto ori_tx_info = parse_tx(tx_ptr.get(), rpc_version);
        result_json["original_tx_info"] = ori_tx_info;

        return result_json;
    } else {
        if (cons_tx_ptr == nullptr) {
            throw xrpc_error{enum_xrpc_error_code::rpc_shard_exec_error, "account address or transaction hash error/does not exist"};
        } else {
            auto ori_tx_info = parse_tx(cons_tx_ptr, rpc_version);
            result_json["original_tx_info"] = ori_tx_info;
            result_json["tx_consensus_state"] = cons;
            result_json["tx_state"] = "queue";
            return result_json;
        }
    }
}

void get_block_handle::parse_asset_out(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_asset_out ao;
        auto ret = ao.parse(action);
        if (ret != 0) {
            return;
        }
        j["amount"] = static_cast<xJson::UInt64>(ao.m_asset_out.m_amount);
        j["token_name"] = (ao.m_asset_out.m_token_name == "") ? XPROPERTY_ASSET_TOP : ao.m_asset_out.m_token_name;
    } catch (...) {
        xdbg("xaction_asset_out parse error");
    }
}

void get_block_handle::parse_asset_in(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_asset_in ai;
        auto ret = ai.parse(action);
        if (ret != 0) {
            return;
        }
        j["amount"] = static_cast<xJson::UInt64>(ai.m_asset.m_amount);
        j["token_name"] = (ai.m_asset.m_token_name == "") ? XPROPERTY_ASSET_TOP : ai.m_asset.m_token_name;
    } catch (...) {
        xdbg("xaction_asset_in parse error");
    }
}

void get_block_handle::parse_pledge_token(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_pledge_token ai;
        auto ret = ai.parse(action);
        if (ret != 0) {
            return;
        }
        j["amount"] = static_cast<xJson::UInt64>(ai.m_asset.m_amount);
        j["token_name"] = (ai.m_asset.m_token_name == "") ? XPROPERTY_ASSET_TOP : ai.m_asset.m_token_name;
    } catch (...) {
        xdbg("xaction_pledge_token parse error");
    }
}

void get_block_handle::parse_redeem_token(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_redeem_token ai;
        auto ret = ai.parse(action);
        if (ret != 0) {
            return;
        }
        j["amount"] = static_cast<xJson::UInt64>(ai.m_asset.m_amount);
        j["token_name"] = (ai.m_asset.m_token_name == "") ? XPROPERTY_ASSET_TOP : ai.m_asset.m_token_name;
    } catch (...) {
        xdbg("xaction_redeem_token parse error");
    }
}

void get_block_handle::parse_pledge_token_vote(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_pledge_token_vote ai;
        auto ret = ai.parse(action);
        if (ret != 0) {
            return;
        }
        j["vote_num"] = static_cast<xJson::UInt64>(ai.m_vote_num);
        j["lock_duration"] = ai.m_lock_duration;
    } catch (...) {
        xdbg("parse_pledge_token_vote parse error");
    }
}

void get_block_handle::parse_redeem_token_vote(xJson::Value & j, const xaction_t & action) {
    try {
        xaction_redeem_token_vote ai;
        auto ret = ai.parse(action);
        if (ret != 0) {
            return;
        }
        j["vote_num"] = static_cast<xJson::UInt64>(ai.m_vote_num);
    } catch (...) {
        xdbg("parse_pledge_token_vote parse error");
    }
}

xJson::Value get_block_handle::parse_action(const xaction_t & action) {
    xJson::Value j;

    switch (action.get_action_type()) {
    case xaction_type_asset_out:
        parse_asset_out(j, action);
        break;

    case xaction_type_create_contract_account:
        parse_create_contract_account(j, action);
        break;

    case xaction_type_run_contract:
        parse_run_contract(j, action);
        break;

    case xaction_type_asset_in:
        parse_asset_in(j, action);
        break;

    case xaction_type_pledge_token:
        parse_pledge_token(j, action);
        break;

    case xaction_type_redeem_token:
        parse_redeem_token(j, action);
        break;

    case xaction_type_pledge_token_vote:
        parse_pledge_token_vote(j, action);
        break;

    case xaction_type_redeem_token_vote:
        parse_redeem_token_vote(j, action);
        break;

    default:
        break;
    }

    return j;
}

void get_block_handle::getTransaction() {
    uint256_t hash = top::data::hex_to_uint256(m_js_req["tx_hash"].asString());
    std::string version = m_js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    std::string tx_hash_str = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    try {
        m_js_rsp["value"] = parse_tx(hash, nullptr, version);
        base::xvtransaction_store_ptr_t tx_store_ptr = m_block_store->query_tx(tx_hash_str, base::enum_transaction_subtype_all);
        if (tx_store_ptr != nullptr) {
            if (tx_store_ptr->get_raw_tx() != nullptr) {
                auto tx = dynamic_cast<xtransaction_t*>(tx_store_ptr->get_raw_tx());
                tx->add_ref();
                xtransaction_ptr_t tx_ptr;
                tx_ptr.attach(tx);

                data::xaction_t action;
                action.set_account_addr(tx->get_source_addr());
                action.set_action_type(tx->get_source_action_type());
                action.set_action_name(tx->get_source_action_name());
                action.set_action_param(tx->get_source_action_para());
                auto jsa = parse_action(action);

                action.set_account_addr(tx->get_origin_target_addr());
                action.set_action_type(tx->get_target_action_type());
                action.set_action_name(tx->get_target_action_name());
                action.set_action_param(tx->get_target_action_para());
                auto jta = parse_action(action);
                
                if (version == RPC_VERSION_V2) {
                    m_js_rsp["value"]["original_tx_info"]["sender_action_param"] = jsa;
                    m_js_rsp["value"]["original_tx_info"]["receiver_action_param"] = jta;
                } else {
                    m_js_rsp["value"]["original_tx_info"]["tx_action"]["sender_action"]["action_param"] = jsa;
                    m_js_rsp["value"]["original_tx_info"]["tx_action"]["receiver_action"]["action_param"] = jta;
                }
            }
        }
    } catch (exception & e) {
        set_result(e.what());
    }
}

void get_block_handle::getRecs() {
    // get_nodes(sys_contract_rec_elect_rec_addr);
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_rec_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_0";
    query_account_property(j, addr, prop_name);
    m_js_rsp["value"] = j["root_beacon"];
    m_js_rsp["chain_id"] = j["chain_id"];
}

void get_block_handle::getZecs() {
    // get_nodes(sys_contract_rec_elect_zec_addr);
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_zec_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_0";
    query_account_property(j, addr, prop_name);
    m_js_rsp["value"] = j["sub_beacon"];
    m_js_rsp["chain_id"] = j["chain_id"];
}

void get_block_handle::getEdges() {
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_edge_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    query_account_property(j, addr, prop_name);
    m_js_rsp["value"] = j["edge"];
    m_js_rsp["chain_id"] = j["chain_id"];
}

void get_block_handle::getArcs() {
    xJson::Value j;
    std::string const addr = sys_contract_rec_elect_archive_addr;
    auto property_name = top::data::election::get_property_by_group_id(common::xarchive_group_id);
    query_account_property(j, addr, property_name);
    m_js_rsp["value"] = j["archive"];
    m_js_rsp["chain_id"] = j["chain_id"];
}

void get_block_handle::getFullNodes() {
    xJson::Value j;
    std::string const addr = sys_contract_rec_elect_archive_addr;
    auto property_name = top::data::election::get_property_by_group_id(common::xfull_node_group_id);
    query_account_property(j, addr, property_name);
    m_js_rsp["value"] = j["full_node"];
    m_js_rsp["chain_id"] = j["chain_id"];
}

void get_block_handle::getConsensus() {
    std::string addr = sys_contract_zec_elect_consensus_addr;
    auto property_names = top::data::election::get_property_name_by_addr(common::xaccount_address_t{addr});
    for (auto const & property : property_names) {
        xJson::Value j;
        query_account_property(j, addr, property);
        std::string cluster_name = "cluster" + property.substr(property.find('_') + 1);
        m_js_rsp["value"][cluster_name] = j;
    }
    // xJson::Value j1;
    // std::string addr = sys_contract_zec_elect_consensus_addr;
    // std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    // query_account_property(j1, addr, prop_name);
    // m_js_rsp["value"]["cluster1"] = j1;

    // xJson::Value j2;
    // prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_2";
    // query_account_property(j2, addr, prop_name);
    // m_js_rsp["value"]["cluster2"] = j2;
}

void get_block_handle::getStandbys() {
    xJson::Value j;
    std::string addr = sys_contract_rec_standby_pool_addr;
    std::string prop_name = XPROPERTY_CONTRACT_STANDBYS_KEY;
    query_account_property(j, addr, prop_name);
    m_js_rsp["value"] = j;
}

void get_block_handle::queryNodeInfo() {
    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_registration_addr;
    std::string prop_name = xstake::XPORPERTY_CONTRACT_REG_KEY;
    query_account_property(jv, contract_addr, prop_name);
    m_js_rsp["value"] = jv[prop_name];
}

void get_block_handle::queryNodeReward() {
    xJson::Value jv;
    std::string prop_name = xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY;
    std::string target = m_js_req["node_account_addr"].asString();
    m_js_rsp["value"] = parse_sharding_reward(target, prop_name);
}

xJson::Value get_block_handle::parse_sharding_reward(const std::string & target, const std::string & prop_name) {
    xJson::Value jv;
    if (target == "") {
        if (prop_name == xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY) {
            for (size_t i = 0; i < enum_vbucket_has_tables_count; ++i) {
                auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, i);
                xdbg("target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.c_str(), prop_name.c_str());
                xJson::Value j;

                query_account_property(j, shard_reward_addr.value(), prop_name);

                auto tmp = j[prop_name];
                for (auto i : tmp.getMemberNames()) {
                    jv[i] = tmp[i];
                }
            }
        } else {
            for (size_t i = 0; i < enum_vbucket_has_tables_count; ++i) {
                auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, i);
                xJson::Value j;
                for (int sub_map_no = 1; sub_map_no <= 4; sub_map_no++) {
                    std::string prop_name = std::string(xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE) + "-" + std::to_string(sub_map_no);
                    xdbg("[get_block_handle::parse_sharding_reward] target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.c_str(), prop_name.c_str());
                    query_account_property(j, shard_reward_addr.value(), prop_name);
                    auto tmp = j[prop_name];
                    for (auto i : tmp.getMemberNames()) {
                        xdbg("[get_block_handle::parse_sharding_reward] --- %s", i.c_str());
                        jv[i] = tmp[i];
                    }
                }
            }
        }
    } else {
        auto const & table_id = data::account_map_to_table_id(common::xaccount_address_t{target}).get_subaddr();
        auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, table_id);
        xdbg("[get_block_handle::parse_sharding_reward] target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.c_str(), prop_name.c_str());
        query_account_property(jv, shard_reward_addr.value(), prop_name);
        jv = jv[prop_name][target];
    }

    return jv;
}

void get_block_handle::getLatestBlock() {
    std::string owner = m_js_req["account_addr"].asString();
    auto vblock = m_block_store->get_latest_committed_block(owner, metrics::blockstore_access_from_rpc_get_block_committed_block);
    data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
    if (owner == sys_contract_zec_slash_info_addr) {
        assert(false);
        std::error_code ec;
        xJson::Value value;
        top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{ owner }, bp->get_height(), top::contract::xjson_format_t::detail, value, ec);
        if (ec) {
            value["query_status"] = ec.message();
        }

        m_js_rsp["value"] = value;
    } else {
        auto value = get_block_json(bp);
        m_js_rsp["value"] = value;
    }
}

void get_block_handle::getLatestFullBlock() {
    std::string owner = m_js_req["account_addr"].asString();
    auto vblock = m_block_store->get_latest_committed_full_block(owner, metrics::blockstore_access_from_rpc_get_block_full_block);
    data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
    if (bp) {
        xJson::Value jv;
        jv["height"] = static_cast<xJson::UInt64>(bp->get_height());
        if (bp->is_fulltable()) {
            xfull_tableblock_t* ftp = dynamic_cast<xfull_tableblock_t*>(bp);
            auto root_hash = ftp->get_fullstate_hash();
            jv["root_hash"] = to_hex_str(root_hash);
            base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(bp, metrics::statestore_access_from_rpc_get_fullbock);
            data::xtablestate_ptr_t tablestate = bstate != nullptr ? std::make_shared<data::xtable_bstate_t>(bstate.get()) : nullptr;
            if (tablestate != nullptr) {
                jv["account_size"] = static_cast<xJson::UInt64>(tablestate->get_account_size());
            }
        }
        m_js_rsp["value"] = jv;
    }
}

void get_block_handle::getBlockByHeight() {
    std::string owner = m_js_req["account_addr"].asString();
    uint64_t height = m_js_req["height"].asUInt64();
    if (owner == sys_contract_zec_slash_info_addr) {
        std::error_code ec;
        xJson::Value value;
        top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{ owner }, height, top::contract::xjson_format_t::detail, value, ec);
        if (ec) {
            value["query_status"] = ec.message();
        }

        m_js_rsp["value"] = value;
    } else {
        auto vblock = m_block_store->load_block_object(base::xvaccount_t(owner), height, 0, true, metrics::blockstore_access_from_rpc_get_block_by_height);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        auto value = get_block_json(bp);
        m_js_rsp["value"] = value;
    }
}

void get_block_handle::getBlock() {
    std::string type = m_js_req["type"].asString();
    std::string owner = m_js_req["account_addr"].asString();
    std::string version = m_js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    base::xvaccount_t _owner_vaddress(owner);

    xJson::Value value;
    if (type == "height") {
        uint64_t height = m_js_req["height"].asUInt64();
        auto vblock = m_block_store->load_block_object(_owner_vaddress, height, 0, true, metrics::blockstore_access_from_rpc_get_block_load_object);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        value = get_block_json(bp, version);

        if (owner == sys_contract_zec_slash_info_addr) {
            xJson::Value slash_prop;
            std::error_code ec;
            top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{ owner }, height, top::contract::xjson_format_t::detail, slash_prop, ec);
            value["property_info"] = slash_prop;
        } else if (owner.find(sys_contract_sharding_statistic_info_addr) != std::string::npos) {
            xJson::Value statistic_prop;
            std::error_code ec;
            top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{ owner }, height, top::contract::xjson_format_t::detail, statistic_prop, ec);
            value["statistic_info"] = statistic_prop;
        }
    } else if (type == "last") {
        auto vblock = m_block_store->get_latest_committed_block(_owner_vaddress, metrics::blockstore_access_from_rpc_get_block_committed_block);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        value = get_block_json(bp, version);
    }

    m_js_rsp["value"] = value;
}

void get_block_handle::getProperty() {
    std::string type = m_js_req["type"].asString();
    std::string owner = m_js_req["account_addr"].asString();
    base::xvaccount_t _owner_vaddress(owner);

    xJson::Value value;
    uint64_t height = 0;
    if (type == "last") {
        height = m_block_store->get_latest_committed_block_height(_owner_vaddress, metrics::blockstore_access_from_rpc_get_block_committed_height);
    } else if (type == "height") {
        height = m_js_req["height"].asUInt64();
    }

    std::string prop_name = m_js_req["prop"].asString();
    xJson::Value jv;
    query_account_property(jv, owner, prop_name, height);
    m_js_rsp["value"] = jv;
}

void get_block_handle::set_redeem_token_num(xaccount_ptr_t ac, xJson::Value & value) {
    std::map<std::string, std::string> lock_txs;
    ac->map_get(XPROPERTY_LOCK_TOKEN_KEY, lock_txs);

    uint64_t tgas_redeem_num(0);
    uint64_t disk_redeem_num(0);
    if (lock_txs.empty()) {
        value["unlock_gas_staked"] = static_cast<xJson::UInt64>(tgas_redeem_num);
        value["unlock_disk_staked"] = static_cast<xJson::UInt64>(disk_redeem_num);
        return;
    }
    for (auto tx : lock_txs) {
        std::string v = tx.second;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)v.c_str(), (uint32_t)v.size());
        uint64_t clock_timer;
        std::string raw_input;
        stream >> clock_timer;
        stream >> raw_input;

        data::xaction_lock_account_token lock_action;
        lock_action.parse_param(raw_input);
        xdbg("browser unlock type: %d, amount: %d", lock_action.m_unlock_type, lock_action.m_amount);
        if (lock_action.m_unlock_type == xaction_lock_account_token::UT_time) {
            if (lock_action.m_version == 0) {
                tgas_redeem_num += lock_action.m_amount;
            } else {
                disk_redeem_num += lock_action.m_amount;
            }
        }
    }
    value["unlock_gas_staked"] = static_cast<xJson::UInt64>(tgas_redeem_num);
    value["unlock_disk_staked"] = static_cast<xJson::UInt64>(disk_redeem_num);
    return;
}

void get_block_handle::set_shared_info(xJson::Value & root, xblock_t * bp) {
    root["owner"] = bp->get_block_owner();
    root["height"] = static_cast<unsigned long long>(bp->get_height());
    if (bp->is_unitblock()) {
        root["table_height"] = static_cast<unsigned long long>(bp->get_parent_block_height());    
    } else {
        root["table_height"] = static_cast<unsigned long long>(bp->get_height());    
    }
    root["hash"] = bp->get_block_hash_hex_str();
    root["prev_hash"] = to_hex_str(bp->get_last_block_hash());
    root["timestamp"] = static_cast<unsigned long long>(bp->get_timestamp());
}

void get_block_handle::set_header_info(xJson::Value & header, xblock_t * bp) {
    header["timerblock_height"] = static_cast<unsigned long long>(bp->get_timerblock_height());
    // header["status"] = p_header->m_block_status;
    header["version"] = bp->get_block_version();
    auto auditor = bp->get_cert()->get_auditor();
    header["auditor_xip"] = xdatautil::xip_to_hex(auditor);
    std::string addr;
    if (auditor.high_addr != 0 && auditor.low_addr != 0 && get_node_id_from_xip2(auditor) != 0x3FF) {
        if (contract::xcontract_manager_t::get_account_from_xip(auditor, addr) == 0) {
            header["auditor"] = addr;
        }
    }

    auto validator = bp->get_cert()->get_validator();
    header["validator_xip"] = xdatautil::xip_to_hex(validator);
    // header["validator"] = xdatautil::xip_to_hex(validator);
    if (validator.high_addr != 0 && validator.low_addr != 0 && get_node_id_from_xip2(validator) != 0x3FF) {
        if (contract::xcontract_manager_t::get_account_from_xip(validator, addr) == 0) {
            header["validator"] = addr;
        }
    }
    if (bp->is_tableblock()) {
        header["multisign_auditor"] = to_hex_str(bp->get_cert()->get_audit_signature());
        header["multisign_validator"] = to_hex_str(bp->get_cert()->get_verify_signature());
    }
}

bool is_prop_name_already_set_property(const std::string & prop_name) {
    static std::set<std::string> property_names = {
        XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY,
        XPROPERTY_CONTRACT_STANDBYS_KEY,
        XPROPERTY_CONTRACT_GROUP_ASSOC_KEY,
    };

    auto iter = property_names.find(prop_name);
    if (iter != property_names.end()) {
        return true;
    }
    if (prop_name.size() > 3 && XPROPERTY_CONTRACT_ELECTION_RESULT_KEY == prop_name.substr(0, 3)) {
        return true;
    }
    return false;
}

bool is_prop_name_not_set_property(const std::string & prop_name) {
    static std::set<std::string> property_names = {
        xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY,
        xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY,
        xstake::XPORPERTY_CONTRACT_REG_KEY,
        xstake::XPORPERTY_CONTRACT_TICKETS_KEY,
        xstake::XPORPERTY_CONTRACT_WORKLOAD_KEY,
        xstake::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY,
        xstake::XPORPERTY_CONTRACT_TASK_KEY,
        xstake::XPORPERTY_CONTRACT_VOTES_KEY1,
        xstake::XPORPERTY_CONTRACT_VOTES_KEY2,
        xstake::XPORPERTY_CONTRACT_VOTES_KEY3,
        xstake::XPORPERTY_CONTRACT_VOTES_KEY4,
        xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
        xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
        xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
        xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
        xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY,
        xstake::XPORPERTY_CONTRACT_REFUND_KEY,
        xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
        xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY,
        xstake::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY,
        xstake::XPROPERTY_CONTRACT_SLASH_INFO_KEY,
        xstake::XPROPERTY_REWARD_DETAIL,
        PROPOSAL_MAP_ID,
        VOTE_MAP_ID
    };

    auto iter = property_names.find(prop_name);
    if (iter != property_names.end()) {
        return true;
    }
    return false;
}

bool query_special_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, xaccount_ptr_t unitstate) {
    if (is_prop_name_already_set_property(prop_name)) {
        top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{owner}, unitstate, prop_name, top::contract::xjson_format_t::detail, jph);
        return true;
    } else if (is_prop_name_not_set_property(prop_name)) {
        xJson::Value jm;
        top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{owner}, unitstate, prop_name, top::contract::xjson_format_t::detail, jm);
        jph[prop_name] = jm;
        return true;
    }

    if (XPROPERTY_PLEDGE_VOTE_KEY == prop_name) {
        base::xvproperty_t* propobj = unitstate->get_bstate()->get_property_object(prop_name);
        base::xmapvar_t<std::string> * var_obj = dynamic_cast<base::xmapvar_t<std::string> *>(propobj);
        std::map<std::string, std::string> pledge_votes = var_obj->query();
        for (auto & v : pledge_votes) {
            uint64_t vote_num{0};
            uint16_t duration{0};
            uint64_t lock_time{0};
            // TODO(jimmy)
            xaccount_context_t::deserilize_vote_map_field(v.first, duration, lock_time);
            xaccount_context_t::deserilize_vote_map_value(v.second, vote_num);
            xdbg("pledge_redeem_vote %d, %d, %d", vote_num, duration, lock_time);
            if (vote_num == 0)
                continue;
            xJson::Value j;
            j["vote_num"] = static_cast<unsigned long long>(vote_num);
            if (duration != 0)
                j["lock_token"] = static_cast<unsigned long long>(xaccount_context_t::get_top_by_vote(vote_num, duration));
            else {
                auto propobj_str = unitstate->get_bstate()->load_string_var(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY);
                j["lock_token"] = propobj_str->query();
            }
            j["duration"] = duration;
            j["lock_time"] = static_cast<unsigned long long>(lock_time);
            jph[XPROPERTY_PLEDGE_VOTE_KEY].append(j);
        }
        return true;
    }
    return false;
}

void get_block_handle::query_account_property_base(xJson::Value & jph, const std::string & owner, const std::string & prop_name, xaccount_ptr_t unitstate) {
    if (unitstate == nullptr) {
        xwarn("get_block_handle::query_account_property fail-query unit state.account=%s", owner.c_str());
        return;
    }
    if (false == unitstate->get_bstate()->find_property(prop_name)) {
        xwarn("get_block_handle::query_account_property fail-find property.account=%s,prop_name=%s", owner.c_str(), prop_name.c_str());
        return;
    }
    if (true == query_special_property(jph, owner, prop_name, unitstate)) {
        return;
    }

    base::xvproperty_t* propobj = unitstate->get_bstate()->get_property_object(prop_name);
    if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_map) {
        auto propobj_map = unitstate->get_bstate()->load_string_map_var(prop_name);
        auto values = propobj_map->query();
        xJson::Value j;
        for (auto & v : values) {
            j[v.first] = v.second;
        }
        jph[prop_name] = j;
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string) {
        auto propobj_str = unitstate->get_bstate()->load_string_var(prop_name);
        jph[prop_name] = propobj_str->query();
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_string_deque) {
        auto propobj_deque = unitstate->get_bstate()->load_string_deque_var(prop_name);
        auto values = propobj_deque->query();
        for (auto & v : values) {
            jph[prop_name].append(v);
        }
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_token) {
        auto propobj = unitstate->get_bstate()->load_token_var(prop_name);
        base::vtoken_t balance = propobj->get_balance();
        jph[prop_name] = std::to_string(balance);
    } else if (propobj->get_obj_type() == base::enum_xobject_type_vprop_uint64) {
        auto propobj = unitstate->get_bstate()->load_uint64_var(prop_name);
        uint64_t value = propobj->get();
        jph[prop_name] = std::to_string(value);
    }
}

void get_block_handle::query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name) {
    xdbg("get_block_handle::query_account_property account=%s,prop_name=%s", owner.c_str(), prop_name.c_str());
    // load newest account state
    xaccount_ptr_t unitstate = m_store->query_account(owner);
    query_account_property_base(jph, owner, prop_name, unitstate);
}

void get_block_handle::query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, const uint64_t height) {
    xdbg("get_block_handle::query_account_property account=%s,prop_name=%s,height=%llu", owner.c_str(), prop_name.c_str(),height);
    // load newest account state
    base::xvaccount_t _vaddr(owner);
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddr, height, 0, false, metrics::blockstore_access_from_rpc_get_block_query_propery);
    if (_block == nullptr) {
        xdbg("get_block_handle::query_account_property block %s, height %llu, not exist", owner.c_str(), height);
        return;
    }

    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xdbg("get_block_handle::query_account_property %s, height %llu, genesis or nil block", owner.c_str(), height);
        return;
    }

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(),metrics::statestore_access_from_rpc_query_propery);
    xaccount_ptr_t unitstate = nullptr;
    if (bstate != nullptr) {
        unitstate = std::make_shared<xunit_bstate_t>(bstate.get());
    }

    query_account_property_base(jph, owner, prop_name, unitstate);
}

void get_block_handle::set_accumulated_issuance_yearly(xJson::Value & j, const std::string & value) {
    xJson::Value jv;
    xstake::xaccumulated_reward_record record;
    xstream_t stream(xcontext_t::instance(), (uint8_t *)value.c_str(), (uint32_t)value.size());
    record.serialize_from(stream);
    jv["last_issuance_time"] = (xJson::UInt64)record.last_issuance_time;
    jv["issued_until_last_year_end"] = (xJson::UInt64) static_cast<uint64_t>(record.issued_until_last_year_end / xstake::REWARD_PRECISION);
    jv["issued_until_last_year_end_decimals"] = (xJson::UInt) static_cast<uint32_t>(record.issued_until_last_year_end % xstake::REWARD_PRECISION);
    j[xstake::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY] = jv;
}

void get_block_handle::set_unqualified_node_map(xJson::Value & j, std::map<std::string, std::string> const & ms) {
    xunqualified_node_info_t summarize_info;
    for (auto const & m : ms) {
        auto detail = m.second;
        if (!detail.empty()) {
            base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
            summarize_info.serialize_from(stream);
        }

        xJson::Value jvn;
        xJson::Value jvn_auditor;
        for (auto const & v : summarize_info.auditor_info) {
            xJson::Value auditor_info;
            auditor_info["vote_num"] = v.second.block_count;
            auditor_info["subset_num"] = v.second.subset_count;
            jvn_auditor[v.first.value()] = auditor_info;
        }

        xJson::Value jvn_validator;
        for (auto const & v : summarize_info.validator_info) {
            xJson::Value validator_info;
            validator_info["vote_num"] = v.second.block_count;
            validator_info["subset_num"] = v.second.subset_count;
            jvn_validator[v.first.value()] = validator_info;
        }

        jvn["auditor"] = jvn_auditor;
        jvn["validator"] = jvn_validator;
        j["unqualified_node"] = jvn;
    }
}

void get_block_handle::set_unqualified_slash_info_map(xJson::Value & j, std::map<std::string, std::string> const & ms) {
    xstake::xslash_info s_info;
    for (auto const & m : ms) {
        auto detail = m.second;
        if (!detail.empty()) {
            base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
            s_info.serialize_from(stream);
        }

        xJson::Value jvn;
        jvn["punish_time"] = (xJson::UInt64)s_info.m_punish_time;
        jvn["staking_lock_time"] = (xJson::UInt64)s_info.m_staking_lock_time;
        jvn["punish_staking"] = (xJson::UInt)s_info.m_punish_staking;

        j[m.first] = jvn;
    }
}

void get_block_handle::set_proposal_map(xJson::Value & j, std::map<std::string, std::string> & ms) {
    tcc::proposal_info pi;
    for (auto m : ms) {
        auto detail = m.second;
        base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        pi.deserialize(stream);

        xJson::Value jv;
        jv["proposal_id"] = pi.proposal_id;
        jv["proposal_type"] = (xJson::UInt)pi.type;
        jv["target"] = pi.parameter;
        jv["value"] = pi.new_value;
        // jv["modification_description"] = pi.modification_description;
        jv["proposer_account_addr"] = pi.proposal_client_address;
        jv["proposal_deposit"] = (xJson::UInt64)(pi.deposit);
        jv["effective_timer_height"] = (xJson::UInt64)(pi.effective_timer_height);
        jv["priority"] = pi.priority;
        // jv["cosigning_status"] = pi.cosigning_status;
        jv["voting_status"] = pi.voting_status;
        jv["expire_time"] = (xJson::UInt64)(pi.end_time);

        j[m.first] = jv;
    }
}

void get_block_handle::set_property_info(xJson::Value & jph, const std::map<std::string, std::string> & mph) {
    for (auto & ph : mph) {
        // jph[ph.first] = ph.second; error non utf-8
        jph.append(ph.first);
    }
}

static std::unordered_map<common::xnode_type_t, std::string> node_type_map{
    { common::xnode_type_t::consensus_auditor, "auditor" },
    { common::xnode_type_t::consensus_validator, "validator" },
    { common::xnode_type_t::edge, "edge" },
    { common::xnode_type_t::storage_archive, "archive" },
    { common::xnode_type_t::rec, "root_beacon" },
    { common::xnode_type_t::zec, "sub_beacon" },
    { common::xnode_type_t::storage_full_node, "full_node" }
};

void get_block_handle::set_addition_info(xJson::Value & body, xblock_t * bp) {
    auto _bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(bp, metrics::statestore_access_from_rpc_set_addition);
    if (nullptr == _bstate) {
        xwarn("get_block_handle::set_addition_info get target state fail.block=%s", bp->dump().c_str());
        return;
    }
    data::xunit_bstate_t state(_bstate.get());
    std::string elect_data;
    auto block_owner = bp->get_block_owner();

    static std::set<std::string> sys_block_owner{sys_contract_rec_elect_edge_addr,
                                                 sys_contract_rec_elect_archive_addr,
                                                 sys_contract_rec_elect_rec_addr,
                                                 sys_contract_rec_elect_zec_addr,
                                                 sys_contract_zec_elect_consensus_addr};
    if (sys_block_owner.find(block_owner) != std::end(sys_block_owner)) {
        using top::data::election::xelection_result_store_t;
        auto property_names = data::election::get_property_name_by_addr(common::xaccount_address_t{block_owner});
        xJson::Value jv;
        jv["round_no"] = static_cast<xJson::UInt64>(bp->get_height());
        jv["zone_id"] = common::xdefault_zone_id_value;
        for (auto const & property : property_names) {
            if (false == state.string_get(property, elect_data) || elect_data.empty()) {
                continue;
            }
            auto const & election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(elect_data), std::end(elect_data)});

            // depressed    jv["m_transaction_type"];
            common::xzone_id_t zid;
            if (block_owner == sys_contract_rec_elect_edge_addr) {
                zid = common::xedge_zone_id;
            } else if (block_owner == sys_contract_zec_elect_consensus_addr) {
                zid = common::xdefault_zone_id;
            } else {
                zid = common::xcommittee_zone_id;
            }

            for (auto const & election_result_info : election_result_store) {
                auto const network_id = top::get<common::xnetwork_id_t const>(election_result_info);
                auto const & election_type_results = top::get<data::election::xelection_network_result_t>(election_result_info);
                for (auto const & election_type_result : election_type_results) {
                    auto node_type = top::get<common::xnode_type_t const>(election_type_result);
                    auto const & election_result = top::get<data::election::xelection_result_t>(election_type_result);

                    for (auto const & cluster_result_info : election_result) {
                        auto const & cluster_id = top::get<common::xcluster_id_t const>(cluster_result_info);
                        auto const & cluster_result = top::get<xelection_cluster_result_t>(cluster_result_info);

                        for (auto const & group_result_info : cluster_result) {
                            auto const & group_id = top::get<common::xgroup_id_t const>(group_result_info);
                            auto const & group_result = top::get<xelection_group_result_t>(group_result_info);

                            for (auto const & node_info : group_result) {
                                auto const & node_id = top::get<xelection_info_bundle_t>(node_info).node_id();
                                if (node_id.empty()) {
                                    continue;
                                }
                                auto const & election_info = top::get<xelection_info_bundle_t>(node_info).election_info();
                                common::xip2_t xip2{network_id, zid, cluster_id, group_id, top::get<common::xslot_id_t const>(node_info), (uint16_t)group_result.size(), bp->get_height()};

                                xJson::Value j;
                                j["account"] = node_id.to_string();
                                j["public_key"] = to_hex_str(election_info.consensus_public_key.to_string());
                                j["group_id"] = xip2.group_id().value();
                                j["stake"] = static_cast<unsigned long long>(election_info.stake);
                                j["node_type"] = node_type_map[node_type];

                                if (group_result.group_version().has_value()) {
                                    j["version"] = static_cast<xJson::UInt64>(group_result.group_version().value());
                                }
                                j["start_timer_height"] = static_cast<xJson::UInt64>(group_result.start_time());
                                j["timestamp"] = static_cast<xJson::UInt64>(group_result.timestamp());
                                j["slot_id"] = top::get<common::xslot_id_t const>(node_info).value();
                                j["xip"] = xstring_utl::uint642hex(xip2.raw_high_part()) + ":" + xstring_utl::uint642hex(xip2.raw_low_part());

                                jv["elect_nodes"].append(j);
                            }
                        }
                    }
                }
            }
        }
        body["elect_transaction"] = jv;
    }
}

void get_block_handle::set_fullunit_state(xJson::Value & j_fu, data::xblock_t * bp) {
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(bp, metrics::statestore_access_from_rpc_set_fullunit);
    xassert(bstate != nullptr);
    data::xunit_bstate_t unitstate(bstate.get());

    j_fu["latest_send_trans_number"] = static_cast<unsigned long long>(unitstate.account_send_trans_number());
    j_fu["latest_send_trans_hash"] = to_hex_str(unitstate.account_send_trans_hash());
    j_fu["latest_recv_trans_number"] = static_cast<unsigned long long>(unitstate.account_recv_trans_number());
    j_fu["account_balance"] = static_cast<unsigned long long>(unitstate.balance());
    j_fu["burned_amount_change"] = static_cast<unsigned long long>(unitstate.burn_balance());
    j_fu["account_create_time"] = static_cast<unsigned long long>(unitstate.get_account_create_time());
}

void get_block_handle::set_body_info(xJson::Value & body, xblock_t * bp, const std::string & rpc_version) {
    auto block_level = bp->get_block_level();

    base::xvaccount_t _vaccount(bp->get_account());
    base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, bp, metrics::blockstore_access_from_rpc_get_block_set_table);
    bp->parse_to_json(body, rpc_version);
    if (block_level == base::enum_xvblock_level_unit) {
        set_addition_info(body, bp);
        auto block_class = bp->get_block_class();
        if (block_class == base::enum_xvblock_class_full) {
            set_fullunit_state(body["fullunit"], bp);
        }
    }
}

xJson::Value get_block_handle::get_block_json(xblock_t * bp, const std::string & rpc_version) {
    xJson::Value root;
    if (bp == nullptr) {
        return root;
    }

    if (bp->is_genesis_block() && bp->get_block_class() == base::enum_xvblock_class_nil && false == bp->check_block_flag(base::enum_xvblock_flag_stored)) {
        // genesis empty non-stored block, should not return
        return root;
    }

    // load input for raw tx get
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(base::xvaccount_t(bp->get_account()) ,bp, metrics::blockstore_access_from_rpc_get_block_json)) {
        xassert(false);  // db block should always load input success
        return root;
    }

    set_shared_info(root, bp);

    xJson::Value header;
    set_header_info(header, bp);
    root["header"] = header;

    xJson::Value body;
    set_body_info(body, bp, rpc_version);

    root["body"] = body;

    return root;
}

void get_block_handle::getSyncNeighbors() {
    auto roles = m_sync->get_neighbors();
    xJson::Value v;
    for (auto role : roles) {
        for (auto peer : role.second) {
            v[role.first].append(peer);
        }
    }
    m_js_rsp["value"] = v;
}

// void get_block_handle::get_sync_overview() {
//     sync::xsync_status_overview_t overview = m_sync->get_overview();

//     uint64_t total = overview.succeed + overview.failed;

//     float success_rate = 0.0;
//     if (total != 0)
//         success_rate = overview.succeed / total;
//     std::string str_success_rate;
//     char buf[32] = {0};
//     sprintf(buf, "%.2f", success_rate);
//     str_success_rate = buf;

//     m_js_rsp["value"]["syncing"] = (xJson::UInt64)overview.syncing_account;
//     m_js_rsp["value"]["send_bytes"] = (xJson::UInt64)overview.send_bytes;
//     m_js_rsp["value"]["recv_bytes"] = (xJson::UInt64)overview.recv_bytes;
//     m_js_rsp["value"]["sync_succeed"] = (xJson::UInt64)overview.succeed;
//     m_js_rsp["value"]["sync_failed"] = (xJson::UInt64)overview.failed;
//     m_js_rsp["value"]["success_rate"] = str_success_rate;
// }

// void get_block_handle::get_sync_detail_all_table() {
//     std::unordered_map<std::string, sync::xaccount_sync_status_t> accounts = m_sync->get_accounts();
//     for (auto & it : accounts) {
//         sync::xaccount_sync_status_t & status = it.second;
//         m_js_rsp["value"][it.first] = xJson::arrayValue;
//         m_js_rsp["value"][it.first][0] = status.is_complete;
//         m_js_rsp["value"][it.first][1] = (xJson::UInt64)status.max_height;
//         m_js_rsp["value"][it.first][2] = (xJson::UInt64)status.continuous_height;
//         m_js_rsp["value"][it.first][3] = status.sub_accounts;
//     }
// }

// void get_block_handle::get_sync_detail_processing_table() {
//     std::unordered_map<std::string, sync::xaccount_sync_status_t> accounts = m_sync->get_accounts();
//     for (auto & it : accounts) {
//         sync::xaccount_sync_status_t & status = it.second;

//         if (status.is_complete && status.sub_accounts == 0)
//             continue;

//         m_js_rsp["value"][it.first] = xJson::arrayValue;
//         m_js_rsp["value"][it.first][0] = status.is_complete;
//         m_js_rsp["value"][it.first][1] = (xJson::UInt64)status.max_height;
//         m_js_rsp["value"][it.first][2] = (xJson::UInt64)status.continuous_height;
//         m_js_rsp["value"][it.first][3] = status.sub_accounts;
//     }
// }

// void get_block_handle::get_all_sync_accounts() {
// #if 0
//     std::map<std::string, sync::height_info_t> infos = m_sync->get_all_sync_accounts();
//     for (auto & it : infos) {
//         std::string account_prefix;
//         uint32_t table_id = 0;

//         data::xdatautil::extract_parts(it.first, account_prefix, table_id);
//         std::string str_table_id = std::to_string(table_id);

//         m_js_rsp["value"][account_prefix][str_table_id][0] = (xJson::UInt64)it.second.cur_height;
//         m_js_rsp["value"][account_prefix][str_table_id][1] = (xJson::UInt64)it.second.behind_height;
//     }
// #endif
// }

// void get_block_handle::get_syncing_accounts() {
// #if 0
//     std::map<std::string, sync::height_info_t> infos = m_sync->get_syncing_accounts();
//     for (auto & it : infos) {
//         std::string account_prefix;
//         uint32_t table_id = 0;

//         data::xdatautil::extract_parts(it.first, account_prefix, table_id);
//         std::string str_table_id = std::to_string(table_id);

//         m_js_rsp["value"][account_prefix][str_table_id][0] = (xJson::UInt64)it.second.cur_height;
//         m_js_rsp["value"][account_prefix][str_table_id][1] = (xJson::UInt64)it.second.behind_height;
//     }
// #endif
// }

}  // namespace chain_info
}  // namespace top
