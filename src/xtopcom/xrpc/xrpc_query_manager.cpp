#include "xrpc_query_manager.h"
#include "xbase/xbase.h"
#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_network_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtableblock.h"
#include "xdata/xtransaction_cache.h"
#include "xrouter/xrouter.h"
#include "xrpc/xuint_format.h"
#include "xrpc/xrpc_loader.h"
#include "xstore/xaccount_context.h"
#include "xstore/xtgas_singleton.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xblocktool.h"
#include "xstatestore/xstatestore_face.h"

#include <cstdint>
#include <iostream>

using namespace top::data;

namespace top {

namespace xrpc {

using namespace std;
using namespace base;
using namespace store;
using namespace xrpc;
const std::string INVALID_ACCOUNT = "Invalid Account!";

void xrpc_query_manager::call_method(std::string strMethod, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    auto iter = m_query_method_map.find(strMethod);
    if (iter != m_query_method_map.end()) {
        (iter->second)(js_req, js_rsp, strResult, nErrorCode);
    }
}

bool xrpc_query_manager::handle(std::string & strReq, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string action = js_req["action"].asString();
    auto iter = m_query_method_map.find(action);
    if (iter != m_query_method_map.end()) {
        if (action == "getAccount" || action == "getTransaction" || action == "getCGP" || action == "getTimerInfo" || action == "getIssuanceDetail" || action == "getTransaction" ||
            action == "getStandbys" || action == "queryNodeReward" || action == "queryNodeInfo" || action == "getGeneralInfos") {
            iter->second(js_req, js_rsp["value"], strResult, nErrorCode);
        } else {
            iter->second(js_req, js_rsp, strResult, nErrorCode);
        }
    } else {
        xinfo("get_block action:%s not exist!", action.c_str());
        strResult = "Method not Found!";
        return false;
    }

    return true;
}

void xrpc_query_manager::getAccount(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string account = js_req["account_addr"].asString();
    xdbg("xarc_query_manager::getAccount account: %s", account.c_str());

    // add top address check
    ADDRESS_CHECK_VALID(account)
    try {
        js_rsp = parse_account(account, strResult, nErrorCode);
    } catch (exception & e) {
        strResult = std::string(e.what());
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_unkown_error;
    }
}

xJson::Value xrpc_query_manager::parse_account(const std::string & account, string & strResult, uint32_t & nErrorCode) {
    xJson::Value result_json;
    data::xunitstate_ptr_t account_ptr = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(common::xaccount_address_t(account));
    if (account_ptr != nullptr && (!account_ptr->is_empty_state())) {
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
        result_json["latest_unit_height"] = static_cast<xJson::UInt64>(account_ptr->height());
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
        m_xrpc_query_func.query_account_property(result_json, account, prop_name, xfull_node_compatible_mode_t::incompatible);
        result_json["vote_staked_index"] = result_json[XPROPERTY_PLEDGE_VOTE_KEY];
        result_json.removeMember(XPROPERTY_PLEDGE_VOTE_KEY);
        return result_json;
    } else {
        strResult = "account not found on chain";
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        return result_json;
    }
}

void xrpc_query_manager::getGeneralInfos(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    xJson::Value j;
    j["shard_num"] = XGET_CONFIG(validator_group_count);
    j["shard_gas"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard));
    j["initial_total_gas_deposit"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_gas_deposit));
    j["genesis_time"] = static_cast<xJson::UInt64>(xrootblock_t::get_rootblock()->get_cert()->get_gmtime());
    auto onchain_total_lock_tgas_token = xtgas_singleton::get_instance().get_cache_total_lock_tgas_token();
    j["token_price"] = xunit_bstate_t::get_token_price(onchain_total_lock_tgas_token);
    std::string value;
    statestore::xstatestore_hub_t::instance()->map_get(zec_reward_contract_address, data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total", value);
    if (!value.empty()) {
        j["total_issuance"] = (xJson::UInt64)base::xstring_utl::touint64(value);
    }
    js_rsp = j;
}

void xrpc_query_manager::getRootblockInfo(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    xJson::Value j;
    xrootblock_t::get_rootblock_data(j);
    js_rsp["value"] = j;
}

uint64_t xrpc_query_manager::get_timer_height() const {
    auto vb = m_block_store->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_rpc_get_cert_blk);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        return bp->get_height();
    } else {
        return 0;
    }
}

void xrpc_query_manager::getCGP(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    xJson::Value j;
    std::string addr = sys_contract_rec_tcc_addr;
    std::string prop_name = ONCHAIN_PARAMS;
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    js_rsp = j[prop_name];
}

void xrpc_query_manager::getTimerInfo(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    xJson::Value j;
    auto timer_clock = get_timer_clock();
    if (timer_clock != 0) {
        j["timer_clock"] = static_cast<xJson::UInt64>(timer_clock);
    }
    auto timer_height = get_timer_height();
    if (timer_height != 0) {
        j["timer_height"] = static_cast<xJson::UInt64>(timer_height);
    }
    js_rsp = j;
}

void xrpc_query_manager::getIssuanceDetail(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    auto get_zec_workload_map =
        [&](common::xaccount_address_t const & contract_address, std::string const & property_name, uint64_t height, xJson::Value & json) {
            std::map<std::string, std::string> workloads;
            if (statestore::xstatestore_hub_t::instance()->get_map_property(contract_address, height - 1, property_name, workloads) != 0) {
                xwarn("[grpc::getIssuanceDetail] get_zec_workload_map contract_address: %s, height: %llu, property_name: %s",
                      contract_address.to_string().c_str(),
                      height,
                      property_name.c_str());
                return;
            }

            xdbg("[grpc::getIssuanceDetail] get_zec_workload_map contract_address: %s, height: %llu, property_name: %s, workloads size: %d",
                 contract_address.to_string().c_str(),
                 height,
                 property_name.c_str(),
                 workloads.size());
            xJson::Value jm;
            for (auto m : workloads) {
                auto detail = m.second;
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                data::system_contract::xgroup_workload_t workload;
                workload.serialize_from(stream);
                xJson::Value jn;
                jn["cluster_total_workload"] = workload.group_total_workload;
                auto const & key_str = m.first;
                common::xgroup_address_t group_address;
                base::xstream_t key_stream(xcontext_t::instance(), (uint8_t *)key_str.data(), key_str.size());
                key_stream >> group_address;
                if (version == RPC_VERSION_V3) {
                    xJson::Value array;
                    for (auto node : workload.m_leader_count) {
                        xJson::Value n;
                        n["account_addr"] = node.first;
                        n["workload"] = node.second;
                        array.append(n);
                    }
                    jn["miner_workload"] = array;
                    // if (common::has<common::xnode_type_t::evm_auditor>(group_address.type()) || common::has<common::xnode_type_t::evm_validator>(group_address.type())) {
                    //     jn["cluster_name"] = std::string{"evm"} + group_address.group_id().to_string();
                    // } else {
                    jn["cluster_name"] = group_address.group_id().to_string();
                    // }
                    jm.append(jn);
                } else {
                    for (auto node : workload.m_leader_count) {
                        jn[node.first] = node.second;
                    }
                    // if (common::has<common::xnode_type_t::evm_auditor>(group_address.type()) || common::has<common::xnode_type_t::evm_validator>(group_address.type())) {
                        // do not show in V2
                        // jm[std::string{"evm"} + group_address.group_id().to_string()] = jn;
                    // } else {
                    jm[group_address.group_id().to_string()] = jn;
                    // }
                }
            }
            json[property_name] = jm;
        };

    uint64_t height = js_req["height"].asUInt64();
    if (height == 0) {
        xwarn("[grpc::getIssuanceDetail] height: %llu", height);
        return;
    }

    xJson::Value j;

    std::string xissue_detail_str;    
    if (statestore::xstatestore_hub_t::instance()->get_string_property(zec_reward_contract_address, height, data::system_contract::XPROPERTY_REWARD_DETAIL, xissue_detail_str) != 0) {
        xwarn("[grpc::getIssuanceDetail] contract_address: %s, height: %llu, property_name: %s",
              sys_contract_zec_reward_addr,
              height,
              data::system_contract::XPROPERTY_REWARD_DETAIL);
        return;
    }
    data::system_contract::xissue_detail_v2 issue_detail;
    if (issue_detail.from_string(xissue_detail_str) <= 0) {
        xwarn("[grpc::getIssuanceDetail] deserialize failed");
    }

    xdbg(
        "[grpc::getIssuanceDetail] reward contract height: %llu, onchain_timer_round: %llu, m_zec_vote_contract_height: %llu, m_zec_workload_contract_height: %llu, "
        "m_zec_reward_contract_height: %llu, m_edge_reward_ratio: %u, m_archive_reward_ratio: %u, m_validator_reward_ratio: %u, m_auditor_reward_ratio: %u, "
        "m_vote_reward_ratio: %u, m_governance_reward_ratio: %u",
        height,
        issue_detail.onchain_timer_round,
        issue_detail.m_zec_vote_contract_height,
        issue_detail.m_zec_workload_contract_height,
        issue_detail.m_zec_reward_contract_height,
        issue_detail.m_edge_reward_ratio,
        issue_detail.m_archive_reward_ratio,
        issue_detail.m_validator_reward_ratio,
        issue_detail.m_auditor_reward_ratio,
        // issue_detail.m_evm_validator_reward_ratio,
        // issue_detail.m_evm_auditor_reward_ratio,
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
    // if (version == RPC_VERSION_V3) {
    //     jv["evm_validator_reward_ratio"] = issue_detail.m_evm_validator_reward_ratio;
    //     jv["evm_auditor_reward_ratio"] = issue_detail.m_evm_auditor_reward_ratio;
    // }
    jv["vote_reward_ratio"] = issue_detail.m_vote_reward_ratio;
    jv["governance_reward_ratio"] = issue_detail.m_governance_reward_ratio;
    jv["validator_group_count"] = (xJson::UInt)issue_detail.m_validator_group_count;
    jv["auditor_group_count"] = (xJson::UInt)issue_detail.m_auditor_group_count;
    if (version == RPC_VERSION_V3) {
        jv["evm_validator_group_count"] = (xJson::UInt)issue_detail.m_evm_validator_group_count;
        jv["evm_auditor_group_count"] = (xJson::UInt)issue_detail.m_evm_auditor_group_count;
    }
    std::map<std::string, std::string> contract_auditor_votes;
    if (statestore::xstatestore_hub_t::instance()->get_map_property(
            zec_vote_contract_address, issue_detail.m_zec_vote_contract_height, data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY, contract_auditor_votes) != 0) {
        xwarn("[grpc::getIssuanceDetail] contract_address: %s, height: %llu, property_name: %s",
              sys_contract_zec_vote_addr,
              issue_detail.m_zec_vote_contract_height,
              data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY);
    }
    xJson::Value jvt;
    if (version == RPC_VERSION_V3) {
        for (auto const & table_vote : contract_auditor_votes) {
            auto const & contract = table_vote.first;
            auto const & auditor_votes_str = table_vote.second;
            xJson::Value jvt1;
            std::map<std::string, std::string> auditor_votes;
            base::xstream_t stream(xcontext_t::instance(), (uint8_t *)auditor_votes_str.data(), auditor_votes_str.size());
            stream >> auditor_votes;
            xJson::Value array;
            for (auto const & node_vote : auditor_votes) {
                xJson::Value n;
                n["miner_addr"] = node_vote.first;
                n["vote_count"] = (xJson::UInt64)base::xstring_utl::touint64(node_vote.second);
                array.append(n);
            }
            jvt["vote_info"] = array;
            jvt["account_addr"] = contract;
            jv["table_votes"].append(jvt);
        }
    } else {
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
    }
    xJson::Value jw1;
    common::xaccount_address_t contract_addr{sys_contract_zec_reward_addr};
    std::string prop_name = data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY;
    get_zec_workload_map(contract_addr, prop_name, issue_detail.m_zec_reward_contract_height + 1, jw1);
    if (jw1[prop_name].empty()) {
        jv["auditor_workloads"] = xJson::Value::null;
    } else {
        jv["auditor_workloads"] = jw1[prop_name];
    }
    xJson::Value jw2;
    prop_name = data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY;
    get_zec_workload_map(contract_addr, prop_name, issue_detail.m_zec_reward_contract_height + 1, jw2);
    if (jw2[prop_name].empty()) {
        jv["validator_workloads"] = xJson::Value::null;
    } else {
        jv["validator_workloads"] = jw2[prop_name];
    }
    xJson::Value jr;
    for (auto const & node_reward : issue_detail.m_node_rewards) {
        if (version == RPC_VERSION_V3) {
            xJson::Value node_reward_json;
            node_reward_json["account_addr"] = node_reward.first;
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_edge_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_edge_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["edge_reward"] = ss.str();
            }
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_archive_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_archive_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["archive_reward"] = ss.str();
            }
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_validator_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_validator_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["validator_reward"] = ss.str();
            }
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_auditor_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_auditor_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["auditor_reward"] = ss.str();
            }
            // {
            //     std::stringstream ss;
            //     ss << static_cast<uint64_t>(node_reward.second.m_evm_validator_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
            //        << static_cast<uint32_t>(node_reward.second.m_evm_validator_reward % data::system_contract::REWARD_PRECISION);
            //     node_reward_json["evm_validator_reward"] = ss.str();
            // }
            // {
            //     std::stringstream ss;
            //     ss << static_cast<uint64_t>(node_reward.second.m_evm_auditor_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
            //        << static_cast<uint32_t>(node_reward.second.m_evm_auditor_reward % data::system_contract::REWARD_PRECISION);
            //     node_reward_json["evm_auditor_reward"] = ss.str();
            // }
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_vote_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_vote_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["voter_reward"] = ss.str();
            }
            {
                std::stringstream ss;
                ss << static_cast<uint64_t>(node_reward.second.m_self_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
                   << static_cast<uint32_t>(node_reward.second.m_self_reward % data::system_contract::REWARD_PRECISION);
                node_reward_json["self_reward"] = ss.str();
            }
            jr.append(node_reward_json);
        } else {
            std::stringstream ss;
            ss << "edge_reward: " << static_cast<uint64_t>(node_reward.second.m_edge_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
               << static_cast<uint32_t>(node_reward.second.m_edge_reward % data::system_contract::REWARD_PRECISION)
               << ", archive_reward: " << static_cast<uint64_t>(node_reward.second.m_archive_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6)
               << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_archive_reward % data::system_contract::REWARD_PRECISION)
               << ", validator_reward: " << static_cast<uint64_t>(node_reward.second.m_validator_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6)
               << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_validator_reward % data::system_contract::REWARD_PRECISION)
               << ", auditor_reward: " << static_cast<uint64_t>(node_reward.second.m_auditor_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6)
               << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_auditor_reward % data::system_contract::REWARD_PRECISION)
               << ", voter_reward: " << static_cast<uint64_t>(node_reward.second.m_vote_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6)
               << std::setfill('0') << static_cast<uint32_t>(node_reward.second.m_vote_reward % data::system_contract::REWARD_PRECISION)
               << ", self_reward: " << static_cast<uint64_t>(node_reward.second.m_self_reward / data::system_contract::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
               << static_cast<uint32_t>(node_reward.second.m_self_reward % data::system_contract::REWARD_PRECISION);
            jr[node_reward.first] = ss.str();
        }
    }
    if (jr.empty()) {
        jv["node_rewards"] = xJson::Value::null;
    } else {
        jv["node_rewards"] = jr;
    }
    std::stringstream ss;
    ss << std::setw(40) << std::setfill('0') << issue_detail.m_zec_reward_contract_height + 1;
    auto key = ss.str();
    if (version == RPC_VERSION_V3) {
        jv["reward_contract_height"] = key;
        js_rsp = jv;
    } else {
        j[key] = jv;
        js_rsp = j;
    }
}

void xrpc_query_manager::getWorkloadDetail(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    uint64_t height = js_req["height"].asUInt64();
    if (height == 0) {
        xwarn("[grpc::getWorkloadDetail] height: %llu", height);
        return;
    }

    std::error_code ec;
    xJson::Value value;
    top::contract::xcontract_manager_t::instance().get_contract_data(
        top::common::xaccount_address_t{sys_contract_zec_reward_addr}, height, top::contract::xjson_format_t::detail, value, ec);
    if (ec) {
        value["query_status"] = ec.message();
    }
    js_rsp["value"] = value;
}

uint64_t xrpc_query_manager::get_timer_clock() const {
    auto vb = m_block_store->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_rpc_get_timer_clock);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        return bp->get_clock();
    } else {
        return 0;
    }
}

void xrpc_query_manager::parse_run_contract(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::set_alias_name_info(xJson::Value & j, const xaction_t & action) {
    j["name"] = action.get_action_param();
}

void xrpc_query_manager::parse_create_contract_account(xJson::Value & j, const xaction_t & action) {
    xaction_deploy_contract deploy;
    deploy.parse(action);
    j["tgas_limit"] = static_cast<unsigned long long>(deploy.m_tgas_limit);
    j["code"] = deploy.m_code;
}

void xrpc_query_manager::set_account_keys_info(xJson::Value & j, const xaction_t & action) {
    xaction_set_account_keys set_account;
    set_account.parse(action);
    j["account_key"] = set_account.m_account_key;
    j["key_value"] = set_account.m_key_value;
}

void xrpc_query_manager::set_lock_token_info(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::set_unlock_token_info(xJson::Value & j, const xaction_t & action) {
    xaction_unlock_account_token unlock_token;
    unlock_token.parse(action);
    j["lock_tran_hash"] = unlock_token.m_lock_tran_hash;
    for (auto s : unlock_token.m_signatures) {
        j["signatures"].append(s);
    }
    j["version"] = unlock_token.m_version;
}

void xrpc_query_manager::set_create_sub_account_info(xJson::Value & j, const xaction_t & action) {
}

xJson::Value xrpc_query_manager::parse_tx(xtransaction_t * tx_ptr, const std::string & version) {
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

void xrpc_query_manager::update_tx_state(xJson::Value & result_json, const xJson::Value & cons, const std::string & rpc_version) {
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

int xrpc_query_manager::parse_tx(const uint256_t & tx_hash, xtransaction_t * cons_tx_ptr, const std::string & rpc_version, xJson::Value& result_json, std::string & strResult, uint32_t & nErrorCode) {
    std::string tx_hash_str = std::string(reinterpret_cast<char*>(tx_hash.data()), tx_hash.size());
    xtxindex_detail_ptr_t sendindex = xrpc_loader_t::load_tx_indx_detail(tx_hash_str, base::enum_transaction_subtype_send);
    xJson::Value cons;
    if (sendindex != nullptr) {
        auto ori_tx_info = parse_tx(sendindex->get_raw_tx().get(), rpc_version);
        result_json["original_tx_info"] = ori_tx_info;

        const xtx_exec_json_key jk(rpc_version);
        xJson::Value sendjson = xrpc_loader_t::parse_send_tx(sendindex);
        bool is_self_tx = sendindex->get_txindex()->is_self_tx() || sendindex->get_raw_tx()->get_target_addr() == black_hole_addr;
        if (is_self_tx) {
            cons[jk.m_confirm] = sendjson;  // XTODO set to confirm block info
        } else {
            cons[jk.m_send] = sendjson;
            data::enum_xunit_tx_exec_status recvtx_status;
            cons[jk.m_recv] = xrpc_loader_t::load_and_parse_recv_tx(tx_hash_str, sendindex, recvtx_status);
            if (cons[jk.m_recv]["height"].asUInt64() > 0) {  // only recv exist will load confirm
                cons[jk.m_confirm] = xrpc_loader_t::load_and_parse_confirm_tx(tx_hash_str, sendindex, recvtx_status);
            }
        }
        result_json["tx_consensus_state"] = cons;
        update_tx_state(result_json, cons, rpc_version);
    } else {
        if (cons_tx_ptr == nullptr) {
            // throw xrpc_error{enum_xrpc_error_code::rpc_shard_exec_error, "account address or transaction hash error/does not exist"};
            return 1;
        } else {
            auto ori_tx_info = parse_tx(cons_tx_ptr, rpc_version);
            result_json["original_tx_info"] = ori_tx_info;
            result_json["tx_consensus_state"] = cons;
            result_json["tx_state"] = "queue";  // queue in txpool
        }
    }
    return 0;
}

void xrpc_query_manager::parse_asset_out(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::parse_asset_in(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::parse_pledge_token(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::parse_redeem_token(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::parse_pledge_token_vote(xJson::Value & j, const xaction_t & action) {
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

void xrpc_query_manager::parse_redeem_token_vote(xJson::Value & j, const xaction_t & action) {
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

xJson::Value xrpc_query_manager::parse_action(const xaction_t & action) {
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

void xrpc_query_manager::getTransaction(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode) {
    const string & account = js_req["account_addr"].asString();
    uint256_t hash = top::data::hex_to_uint256(js_req["tx_hash"].asString());
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    std::string tx_hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    xdbg("xarc_query_manager::getTransaction account: %s, tx hash: %s, version: %s", account.c_str(), tx_hash_str.c_str(), version.c_str());

    // add top address check
    if (!account.empty()) {
        ADDRESS_CHECK_VALID(account)
    }

    std::shared_ptr<xtransaction_cache_data_t> cache_data_ptr = std::make_shared<xtransaction_cache_data_t>();
    if (m_txstore != nullptr && m_txstore->tx_cache_get(tx_hash_str, cache_data_ptr)) {
        const xrpc::xtx_exec_json_key jk(version);
        std::map<int, xJson::Value> map_jv = cache_data_ptr->jv;
        if (map_jv.find(base::enum_transaction_subtype_send) == map_jv.end()) {
            xdbg("not find tx:%s", tx_hash_str.c_str());

            xJson::Value result_json;
            xJson::Value jv;

            data::xtransaction_ptr_t tx_ptr = cache_data_ptr->tran;
            if (map_jv.find(base::enum_transaction_subtype_recv) != map_jv.end()) {
                jv[jk.m_recv] = map_jv[base::enum_transaction_subtype_recv];
                if (version == RPC_VERSION_V2)
                    jv[jk.m_recv]["account"] = tx_ptr->get_target_addr();
            }
            if (map_jv.find(base::enum_transaction_subtype_confirm) != map_jv.end()) {
                jv[jk.m_confirm] = map_jv[base::enum_transaction_subtype_confirm];
                if (version == RPC_VERSION_V2)
                    jv[jk.m_confirm]["account"] = tx_ptr->get_source_addr();
            }
            result_json["tx_consensus_state"] = jv;
            update_tx_state(result_json, jv, version);

            auto ori_tx_info = parse_tx(tx_ptr.get(), version);
            result_json["original_tx_info"] = ori_tx_info;
            js_rsp = result_json;
            return;
        }
    }

    xtransaction_t * tx_ptr = nullptr;
    xcons_transaction_ptr_t cons_tx_ptr = nullptr;
    if (m_txpool_service != nullptr) {
        cons_tx_ptr = m_txpool_service->query_tx(account, hash);
        if (cons_tx_ptr != nullptr) {
            tx_ptr = cons_tx_ptr->get_transaction();
        }
    }

    xJson::Value result_json;
    if (get_transaction_on_demand(account, tx_ptr, version, hash, result_json, strResult, nErrorCode) == 0)
        js_rsp = result_json;
}

int xrpc_query_manager::get_transaction_on_demand(const std::string & account,
                                                  xtransaction_t * tx_ptr,
                                                  const string & version,
                                                  const uint256_t & tx_hash,
                                                  xJson::Value & result_json,
                                                  std::string & strResult,
                                                  uint32_t & nErrorCode) {
    std::string strHash((char *)tx_hash.data(), tx_hash.size());
    if (!m_exchange_flag) {
        if (parse_tx(tx_hash, tx_ptr, version, result_json, strResult, nErrorCode) == 0) {  // find tx
            base::xvtransaction_store_ptr_t tx_store_ptr = m_block_store->query_tx(strHash, base::enum_transaction_subtype_all);
            if (tx_store_ptr != nullptr) {
                if (tx_store_ptr->get_raw_tx() != nullptr) {
                    auto tx = dynamic_cast<xtransaction_t *>(tx_store_ptr->get_raw_tx());
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
                        result_json["original_tx_info"]["sender_action_param"] = jsa;
                        result_json["original_tx_info"]["receiver_action_param"] = jta;
                    } else {
                        result_json["original_tx_info"]["tx_action"]["sender_action"]["action_param"] = jsa;
                        result_json["original_tx_info"]["tx_action"]["receiver_action"]["action_param"] = jta;
                    }
                }
            }
            return 0;
        }
        strResult = "account address or transaction hash error/does not exist";
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        return -1;
    }

    if (parse_tx(tx_hash, tx_ptr, version, result_json, strResult, nErrorCode) != 0) {  // not find tx
        mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_by_hash_t>(account, strHash, "unit_lack");
        base::xvchain_t::instance().get_xevmbus()->push_event(ev);
        strResult = "account address or transaction hash error/does not exist";
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_shard_exec_error;
        return -1;
    }

    base::xvtransaction_store_ptr_t tx_store_ptr = m_block_store->query_tx(strHash, base::enum_transaction_subtype_all);
    if (tx_store_ptr != nullptr) {
        if (tx_store_ptr->get_raw_tx() != nullptr) {
            auto tx = dynamic_cast<xtransaction_t *>(tx_store_ptr->get_raw_tx());
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
                result_json["original_tx_info"]["sender_action_param"] = jsa;
                result_json["original_tx_info"]["receiver_action_param"] = jta;
            } else {
                result_json["original_tx_info"]["tx_action"]["sender_action"]["action_param"] = jsa;
                result_json["original_tx_info"]["tx_action"]["receiver_action"]["action_param"] = jta;
            }
        }
    }

    if (result_json["tx_state"] != "pending" && result_json["tx_state"] != "queue") {
        return 0;
    }

    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    uint64_t tx_time = result_json["original_tx_info"]["send_timestamp"].asUInt64();
    xdbg("tx time: %" PRIu64 ",%" PRIu64, tx_time, (uint64_t)val.tv_sec);
    if (tx_time + 60 < (uint64_t)val.tv_sec) {  // 1 minute
        mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_by_hash_t>(account, strHash, "unit_lack");
        top::base::xvchain_t::instance().get_xevmbus()->push_event(ev);
    }
    return 0;
}

void xrpc_query_manager::getRecs(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    // get_nodes(sys_contract_rec_elect_rec_addr);
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_rec_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_0";
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (version == RPC_VERSION_V3) {
        auto tmp = j["root_beacon"];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"] = jv;
    } else {
        js_rsp["value"] = j["root_beacon"];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getZecs(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    // get_nodes(sys_contract_rec_elect_zec_addr);
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_zec_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_0";
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (version == RPC_VERSION_V3) {
        xJson::Value tmp = j["sub_beacon"];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"] = jv;
    } else {
        js_rsp["value"] = j["sub_beacon"];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getEdges(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_edge_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (version == RPC_VERSION_V3) {
        xJson::Value tmp = j["edge"];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"] = jv;
    } else {
        js_rsp["value"] = j["edge"];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getArcs(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xJson::Value j;
    std::string const addr = sys_contract_rec_elect_archive_addr;
    auto property_name = top::data::election::get_property_by_group_id(common::xarchive_group_id);
    m_xrpc_query_func.query_account_property(j, addr, property_name, xfull_node_compatible_mode_t::incompatible);

    if (version == RPC_VERSION_V3) {
        xJson::Value tmp = j[common::to_presentation_string(common::xnode_type_t::storage_archive)];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"] = jv;
    } else {
        js_rsp["value"] = j[common::to_presentation_string(common::xnode_type_t::storage_archive)];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getEVMs(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xJson::Value j;
    std::string const addr = sys_contract_zec_elect_eth_addr;
    auto property_name = top::data::election::get_property_by_group_id(common::xdefault_group_id);
    m_xrpc_query_func.query_account_property(j, addr, property_name, xfull_node_compatible_mode_t::incompatible);

    if (version == RPC_VERSION_V3) {
        xJson::Value tmp = j[common::to_presentation_string(common::xnode_type_t::evm_auditor)];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"]["evm_auditor"] = jv;

        tmp.clear();
        jv.clear();
        tmp = j[common::to_presentation_string(common::xnode_type_t::evm_validator)];
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"]["evm_validator"] = jv;
    } else {
        js_rsp["value"]["evm_auditor"] = j[common::to_presentation_string(common::xnode_type_t::evm_auditor)];
        js_rsp["value"]["evm_validator"] = j[common::to_presentation_string(common::xnode_type_t::evm_validator)];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getExchangeNodes(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xJson::Value j;

    std::string const addr = sys_contract_rec_elect_exchange_addr;
    auto const & property_name = top::data::election::get_property_by_group_id(common::xexchange_group_id);
    m_xrpc_query_func.query_account_property(j, addr, property_name, xfull_node_compatible_mode_t::incompatible);

    if (version == RPC_VERSION_V3) {
        xJson::Value tmp = j[common::to_presentation_string(common::xnode_type_t::storage_exchange)];
        xJson::Value jv;
        for (auto & i : tmp.getMemberNames()) {
            xJson::Value node = tmp[i][0];
            node["account_addr"] = i;
            jv.append(node);
        }
        js_rsp["value"] = jv;
    } else {
        js_rsp["value"] = j[common::to_presentation_string(common::xnode_type_t::storage_exchange)];
    }
    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getFullNodes2(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    std::string const addr = sys_contract_rec_elect_fullnode_addr;
    auto property_names = top::data::election::get_property_name_by_addr(common::xaccount_address_t{addr});
    assert(property_names.size() == 1);
    for (auto const & property : property_names) {
        xJson::Value j;
        m_xrpc_query_func.query_account_property(j, addr, property, xfull_node_compatible_mode_t::incompatible);
        if (version == RPC_VERSION_V3) {
            xJson::Value tmp = j[common::to_presentation_string(common::xnode_type_t::fullnode)];
            xJson::Value jv;
            for (auto & i : tmp.getMemberNames()) {
                xJson::Value node = tmp[i][0];
                node["account_addr"] = i;
                jv.append(node);
            }
            js_rsp["value"] = jv;
        } else {
            js_rsp["value"] = j[common::to_presentation_string(common::xnode_type_t::fullnode)];
        }
        js_rsp["chain_id"] = j["chain_id"];
    }
}

void xrpc_query_manager::getConsensus(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    std::string addr = sys_contract_zec_elect_consensus_addr;
    auto property_names = top::data::election::get_property_name_by_addr(common::xaccount_address_t{addr});
    for (auto const & property : property_names) {
        xJson::Value j;
        m_xrpc_query_func.query_account_property(j, addr, property, xfull_node_compatible_mode_t::incompatible);
        std::string cluster_name = "cluster" + property.substr(property.find('_') + 1);
        if (version == RPC_VERSION_V3) {
            xJson::Value tmp;
            tmp["chain_id"] = j["chain_id"];
            tmp["cluster_name"] = cluster_name;
            xJson::Value auditor = j["auditor"];
            xJson::Value array1;
            for (auto i : auditor.getMemberNames()) {
                xJson::Value node = auditor[i][0];
                node["account_addr"] = i;
                array1.append(node);
            }
            tmp["auditor"] = array1;
            xJson::Value array2;
            xJson::Value validator = j["validator"];
            for (auto i : validator.getMemberNames()) {
                xJson::Value node = validator[i][0];
                node["account_addr"] = i;
                array2.append(node);
            }
            tmp["validator"] = array2;
            js_rsp["value"].append(tmp);
        } else {
            js_rsp["value"][cluster_name] = j;
        }
    }
    // xJson::Value j1;
    // std::string addr = sys_contract_zec_elect_consensus_addr;
    // std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_1";
    // m_xrpc_query_func.query_account_property(j1, addr, prop_name);
    // js_rsp["value"]["cluster1"] = j1;

    // xJson::Value j2;
    // prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_2";
    // m_xrpc_query_func.query_account_property(j2, addr, prop_name);
    // js_rsp["value"]["cluster2"] = j2;
}

void xrpc_query_manager::getChainId(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    xJson::Value j;
    std::string addr = sys_contract_rec_elect_rec_addr;
    std::string prop_name = std::string(XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_0";
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);

    js_rsp["chain_id"] = j["chain_id"];
}

void xrpc_query_manager::getCrossReceiptIds(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {  
    xJson::Value jv_tables_height;
    base::xreceiptid_all_table_states all_states;
    std::vector<std::string> addrs = data::xblocktool_t::make_all_table_addresses();
    for (auto & addr : addrs) {
        common::xaccount_address_t table_addr(addr);
        xtablestate_ptr_t tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(table_addr);
        if (tablestate != nullptr) {
            base::xreceiptid_state_ptr_t receipt_state = tablestate->get_receiptid_state();
            if (nullptr != receipt_state) {
                all_states.add_table_receiptid_state(table_addr.vaccount().get_short_table_id(), receipt_state);
            }
            jv_tables_height[table_addr.to_string()] = static_cast<unsigned long long>(tablestate->height());
        } else {
            xerror("xrpc_query_manager::getCrossReceiptIds fail-get tablestate.%s", addr.c_str());
        }
    }
    
    xJson::Value jv_unfinish_info; 
    auto unfinish_infos = all_states.get_unfinish_info();
    for (auto & info : unfinish_infos) {
        xJson::Value v;
        v["src_tableid"]        = info.source_id;
        v["dst_tableid"]        = info.target_id;
        v["unrecv"]             = info.unrecv_num;
        v["unconfirm"]          = info.unconfirm_num;
        jv_unfinish_info.append(v);
    }

    js_rsp["tables_height"] = jv_tables_height;
    js_rsp["unfinish_infos"] = jv_unfinish_info;
}

void xrpc_query_manager::getStandbys(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string target = js_req["node_account_addr"].asString();
    xJson::Value j;
    std::string addr = sys_contract_rec_standby_pool_addr;
    std::string prop_name = XPROPERTY_CONTRACT_STANDBYS_KEY;
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (target == "") {
        js_rsp = j;
    } else {
        for (auto j1 : j.getMemberNames()) {
            auto j2 = j[j1];
            for (auto j3 : j2) {
                if (j3["node_id"].asString() == target) {
                    js_rsp = j3;
                }
            }
        }
    }
}

void xrpc_query_manager::queryAllNodeInfo(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_registration_addr;
    std::string prop_name = data::system_contract::XPORPERTY_CONTRACT_REG_KEY;

    m_xrpc_query_func.query_account_property(jv, contract_addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    js_rsp["value"] = jv[prop_name];
}

void xrpc_query_manager::set_sharding_vote_prop(xJson::Value & js_req, xJson::Value & js_rsp, std::string & prop_name, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    std::string target = js_req["node_account_addr"].asString();
    // add top address check
    ADDRESS_CHECK_VALID(owner)
    if (!target.empty()) {
        ADDRESS_CHECK_VALID(target)
    }

    xJson::Value jv;

    auto const & table_id = data::account_map_to_table_id(common::xaccount_address_t{target}).get_subaddr();
    auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_vote_addr}, table_id);
    xdbg("account: %s, target: %s, addr: %s, prop: %s", owner.c_str(), target.c_str(), shard_reward_addr.to_string().c_str(), prop_name.c_str());
    m_xrpc_query_func.query_account_property(jv, shard_reward_addr.to_string(), prop_name, xrpc::xfull_node_compatible_mode_t::incompatible);

    if (target == "") {
        js_rsp = jv[prop_name];
    } else {
        js_rsp = jv[prop_name][target];
    }
}

void xrpc_query_manager::set_sharding_reward_claiming_prop(xJson::Value & js_req, xJson::Value & js_rsp, std::string & prop_name, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    std::string target = js_req["node_account_addr"].asString();

    // add top address check
    ADDRESS_CHECK_VALID(owner)
    if (!target.empty()) {
        ADDRESS_CHECK_VALID(target)
    }
    std::string version = RPC_VERSION_V2;
    xJson::Value jv = parse_sharding_reward(target, prop_name, version);
    js_rsp = jv;
}

void xrpc_query_manager::queryNodeReward(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    xJson::Value jv;
    std::string prop_name = data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY;
    std::string target = js_req["node_account_addr"].asString();
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    // add top address check
    if (!target.empty()) {
        ADDRESS_CHECK_VALID(target)
    }

    js_rsp = parse_sharding_reward(target, prop_name, version);
}

xJson::Value xrpc_query_manager::parse_sharding_reward(const std::string & target, const std::string & prop_name, string & version) {
    xJson::Value jv;
    if (target == "") {
        if (prop_name == data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY) {
            for (size_t i = 0; i < enum_vbucket_has_tables_count; ++i) {
                auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, i);
                xdbg("target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.to_string().c_str(), prop_name.c_str());
                xJson::Value j;

                m_xrpc_query_func.query_account_property(j, shard_reward_addr.to_string(), prop_name, xfull_node_compatible_mode_t::incompatible);
                auto tmp = j[prop_name];
                for (auto i : tmp.getMemberNames()) {
                    if (version == RPC_VERSION_V3) {
                        tmp[i]["account_addr"] = i;
                        jv.append(tmp[i]);
                    } else {
                        jv[i] = tmp[i];
                    }
                }
            }
        } else {
            for (size_t i = 0; i < enum_vbucket_has_tables_count; ++i) {
                auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, i);
                xJson::Value j;
                for (int sub_map_no = 1; sub_map_no <= 4; sub_map_no++) {
                    std::string prop_name = std::string(data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE) + "-" + std::to_string(sub_map_no);
                    xdbg("[xrpc_query_manager::parse_sharding_reward] target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.to_string().c_str(), prop_name.c_str());
                    m_xrpc_query_func.query_account_property(j, shard_reward_addr.to_string(), prop_name, xfull_node_compatible_mode_t::incompatible);
                    auto tmp = j[prop_name];
                    for (auto i : tmp.getMemberNames()) {
                        xdbg("[xrpc_query_manager::parse_sharding_reward] --- %s", i.c_str());
                        jv[i] = tmp[i];
                    }
                }
            }
        }
    } else {
        auto const & table_id = data::account_map_to_table_id(common::xaccount_address_t{target}).get_subaddr();
        auto const & shard_reward_addr = contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, table_id);
        xdbg("[xrpc_query_manager::parse_sharding_reward] target: %s, addr: %s, prop: %s", target.c_str(), shard_reward_addr.to_string().c_str(), prop_name.c_str());
        m_xrpc_query_func.query_account_property(jv, shard_reward_addr.to_string(), prop_name, xfull_node_compatible_mode_t::incompatible);
        jv = jv[prop_name][target];
    }

    return jv;
}

void xrpc_query_manager::getLatestBlock(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    // add top address check
    ADDRESS_CHECK_VALID(owner)

    auto vblock = m_block_store->get_latest_committed_block(owner, metrics::blockstore_access_from_rpc_get_block_committed_block);
    data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
    if (owner == sys_contract_zec_slash_info_addr) {
        assert(false);
        std::error_code ec;
        xJson::Value value;
        top::contract::xcontract_manager_t::instance().get_contract_data(
            top::common::xaccount_address_t{owner}, bp->get_height(), top::contract::xjson_format_t::detail, value, ec);
        if (ec) {
            value["query_status"] = ec.message();
        }

        js_rsp["value"] = value;
    } else {
        auto value = get_block_json(bp);
        js_rsp["value"] = value;
    }
}

void xrpc_query_manager::getLatestFullBlock(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    // add top address check
    ADDRESS_CHECK_VALID(owner)
    auto vblock = m_block_store->get_latest_committed_full_block(owner, metrics::blockstore_access_from_rpc_get_block_full_block);
    data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
    if (bp) {
        xJson::Value jv;
        jv["height"] = static_cast<xJson::UInt64>(bp->get_height());
        if (bp->is_fulltable()) {
            xfull_tableblock_t * ftp = dynamic_cast<xfull_tableblock_t *>(bp);
            auto root_hash = ftp->get_fullstate_hash();
            jv["root_hash"] = to_hex_str(root_hash);
            data::xtablestate_ptr_t tablestate = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(bp);
            if (tablestate != nullptr) {
                jv["account_size"] = static_cast<xJson::UInt64>(tablestate->get_account_size());
            }
        }
        js_rsp["value"] = jv;
    }
}

void xrpc_query_manager::getBlockByHeight(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    // add top address check
    ADDRESS_CHECK_VALID(owner)
    uint64_t height = js_req["height"].asUInt64();
    if (owner == sys_contract_zec_slash_info_addr) {
        std::error_code ec;
        xJson::Value value;
        top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{owner}, height, top::contract::xjson_format_t::detail, value, ec);
        if (ec) {
            value["query_status"] = ec.message();
        }

        js_rsp["value"] = value;
    } else {
        auto vblock =
            m_block_store->load_block_object(base::xvaccount_t(owner), height, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_rpc_get_block_by_height);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        auto value = get_block_json(bp);
        js_rsp["value"] = value;
    }
}

void xrpc_query_manager::getBlocksByHeight(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string type = js_req["type"].asString();
    std::string owner = js_req["account_addr"].asString();
    std::string strHeight; 
    uint64_t height = 0;
    if (js_req["height"].isString()) {
        strHeight = js_req["height"].asString();
    } else {

        height = js_req["height"].asUInt64();
    }

    // add top address check
    ADDRESS_CHECK_VALID(owner)
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xJson::Value value;
    base::xvaccount_t _owner_vaddress(owner);
    if (type == "last" || strHeight == "latest") {
        uint64_t lastHeight = m_block_store->get_latest_cert_block_height(_owner_vaddress, metrics::blockstore_access_from_rpc_get_block_by_height);
        auto vblock_vector = m_block_store->load_block_object(_owner_vaddress, lastHeight, metrics::blockstore_access_from_rpc_get_block_by_height);
        auto vblocks = vblock_vector.get_vector();
        for (base::xvblock_t * vblock : vblocks) {
            data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock);
            value.append(get_blocks_json(bp, version));
        }
    } else {
        if (!strHeight.empty()) {
            height = std::stoull(strHeight);
        }
        auto vblock_vector = m_block_store->load_block_object(_owner_vaddress, height, metrics::blockstore_access_from_rpc_get_block_by_height);
        auto vblocks = vblock_vector.get_vector();
        for (base::xvblock_t * vblock : vblocks) {
            data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock);
            value.append(get_blocks_json(bp, version));
        }
    }
    js_rsp["value"] = value;
}

void xrpc_query_manager::getBlock(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string type = js_req["type"].asString();
    std::string owner = js_req["account_addr"].asString();
    std::string strHeight;
    uint64_t height = 0;
    if (js_req["height"].isString()) {
        strHeight = js_req["height"].asString();
    } else {
        height = js_req["height"].asUInt64();
    }
    // add top address check
    ADDRESS_CHECK_VALID(owner)
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    base::xvaccount_t _owner_vaddress(owner);

    xJson::Value value;
    if (type == "height") {
        auto vblock = m_block_store->load_block_object(_owner_vaddress, height, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_rpc_get_block_load_object);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        value = get_block_json(bp, version);

        if (owner == sys_contract_zec_slash_info_addr) {
            xJson::Value slash_prop;
            std::error_code ec;
            top::contract::xcontract_manager_t::instance().get_contract_data(top::common::xaccount_address_t{owner}, height, top::contract::xjson_format_t::detail, slash_prop, ec);
            value["property_info"] = slash_prop;
        } else if (owner.find(sys_contract_sharding_statistic_info_addr) != std::string::npos) {
            xJson::Value statistic_prop;
            std::error_code ec;
            top::contract::xcontract_manager_t::instance().get_contract_data(
                top::common::xaccount_address_t{owner}, height, top::contract::xjson_format_t::detail, statistic_prop, ec);
            value["statistic_info"] = statistic_prop;
        }
    } else if (type == "last" || strHeight == "latest") {
        auto vblock = m_block_store->get_latest_committed_block(_owner_vaddress, metrics::blockstore_access_from_rpc_get_block_committed_block);
        data::xblock_t * bp = dynamic_cast<data::xblock_t *>(vblock.get());
        value = get_block_json(bp, version);
    } else {
        if (js_req["height"].isString()) {
            height = std::stoull(strHeight);
        }
        xdbg("height: %llu", height);
        auto vb = m_block_store->load_block_object(_owner_vaddress, height, base::enum_xvblock_flag_committed, true, metrics::blockstore_access_from_rpc_get_block);
        xblock_t * bp = dynamic_cast<xblock_t *>(vb.get());
        value = get_block_json(bp);
    }

    js_rsp["value"] = value;
}

void xrpc_query_manager::getProperty(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string type = js_req["type"].asString();
    std::string owner = js_req["account_addr"].asString();
    // add top address check
    ADDRESS_CHECK_VALID(owner)
    base::xvaccount_t _owner_vaddress(owner);

    xJson::Value value;
    uint64_t height = 0;
    if (type == "last") {
        height = m_block_store->get_latest_committed_block_height(_owner_vaddress, metrics::blockstore_access_from_rpc_get_block_committed_height);
    } else if (type == "height") {
        height = js_req["height"].asUInt64();
    }

    std::string prop_name = js_req["prop"].asString();
    xJson::Value jv;
    m_xrpc_query_func.query_account_property(jv, owner, prop_name, height, xfull_node_compatible_mode_t::incompatible);
    js_rsp["value"] = jv;
}

void xrpc_query_manager::set_redeem_token_num(data::xunitstate_ptr_t ac, xJson::Value & value) {
    std::map<std::string, std::string> lock_txs = ac->map_get(XPROPERTY_LOCK_TOKEN_KEY);

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

void xrpc_query_manager::set_shared_info(xJson::Value & root, xblock_t * bp) {
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

void xrpc_query_manager::set_header_info(xJson::Value & header, xblock_t * bp) {
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

void xrpc_query_manager::set_accumulated_issuance_yearly(xJson::Value & j, const std::string & value) {
    xJson::Value jv;
    data::system_contract::xaccumulated_reward_record record;
    xstream_t stream(xcontext_t::instance(), (uint8_t *)value.c_str(), (uint32_t)value.size());
    record.serialize_from(stream);
    jv["last_issuance_time"] = (xJson::UInt64)record.last_issuance_time;
    jv["issued_until_last_year_end"] = (xJson::UInt64) static_cast<uint64_t>(record.issued_until_last_year_end / data::system_contract::REWARD_PRECISION);
    jv["issued_until_last_year_end_decimals"] = (xJson::UInt) static_cast<uint32_t>(record.issued_until_last_year_end % data::system_contract::REWARD_PRECISION);
    j[data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY] = jv;
}

void xrpc_query_manager::set_unqualified_node_map(xJson::Value & j, std::map<std::string, std::string> const & ms) {
    data::system_contract::xunqualified_node_info_v1_t summarize_info;
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
            jvn_auditor[v.first.to_string()] = auditor_info;
        }

        xJson::Value jvn_validator;
        for (auto const & v : summarize_info.validator_info) {
            xJson::Value validator_info;
            validator_info["vote_num"] = v.second.block_count;
            validator_info["subset_num"] = v.second.subset_count;
            jvn_validator[v.first.to_string()] = validator_info;
        }

        jvn["auditor"] = jvn_auditor;
        jvn["validator"] = jvn_validator;
        j["unqualified_node"] = jvn;
    }
}

void xrpc_query_manager::set_unqualified_slash_info_map(xJson::Value & j, std::map<std::string, std::string> const & ms) {
    data::system_contract::xslash_info s_info;
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

void xrpc_query_manager::set_proposal_map(xJson::Value & j, std::map<std::string, std::string> & ms) {
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

void xrpc_query_manager::set_property_info(xJson::Value & jph, const std::map<std::string, std::string> & mph) {
    for (auto & ph : mph) {
        // jph[ph.first] = ph.second; error non utf-8
        jph.append(ph.first);
    }
}

void xrpc_query_manager::set_addition_info(xJson::Value & body, xblock_t * bp) {
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(bp);
    if (nullptr == unitstate) {
        xwarn("xrpc_query_manager::set_addition_info get target state fail.block=%s", bp->dump().c_str());
        return;
    }
    std::string elect_data;
    auto block_owner = bp->get_block_owner();

    static std::set<std::string> sys_block_owner{sys_contract_rec_elect_edge_addr,
                                                 sys_contract_rec_elect_archive_addr,
                                                 sys_contract_rec_elect_exchange_addr,
                                                 sys_contract_rec_elect_rec_addr,
                                                 sys_contract_rec_elect_zec_addr,
                                                 sys_contract_zec_elect_consensus_addr,
                                                 sys_contract_rec_elect_fullnode_addr};
    if (sys_block_owner.find(block_owner) != std::end(sys_block_owner)) {
        using top::data::election::xelection_result_store_t;
        auto property_names = data::election::get_property_name_by_addr(common::xaccount_address_t{block_owner});
        xJson::Value jv;
        jv["round_no"] = static_cast<xJson::UInt64>(bp->get_height());
        jv["zone_id"] = common::xdefault_zone_id_value;
        for (auto const & property : property_names) {
            elect_data = unitstate->string_get(property);
            if (elect_data.empty()) {
                continue;
            }
            auto const & election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(elect_data), std::end(elect_data)});

            // depressed    jv["m_transaction_type"];
            common::xzone_id_t zid;
            if (block_owner == sys_contract_rec_elect_edge_addr) {
                zid = common::xedge_zone_id;
            } else if (block_owner == sys_contract_zec_elect_consensus_addr) {
                zid = common::xdefault_zone_id;
            } else if (block_owner == sys_contract_rec_elect_fullnode_addr) {
                zid = common::xfullnode_zone_id;
            } else if (block_owner == sys_contract_rec_elect_archive_addr || block_owner == sys_contract_rec_elect_exchange_addr) {
                zid = common::xstorage_zone_id;
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
                        auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(cluster_result_info);

                        for (auto const & group_result_info : cluster_result) {
                            auto const & group_id = top::get<common::xgroup_id_t const>(group_result_info);
                            auto const & group_result = top::get<data::election::xelection_group_result_t>(group_result_info);

                            for (auto const & node_info : group_result) {
                                auto const & node_id = top::get<data::election::xelection_info_bundle_t>(node_info).account_address();
                                if (node_id.empty()) {
                                    continue;
                                }
                                auto const & election_info = top::get<data::election::xelection_info_bundle_t>(node_info).election_info();
                                common::xip2_t xip2{
                                    network_id, zid, cluster_id, group_id, top::get<common::xslot_id_t const>(node_info), (uint16_t)group_result.size(), bp->get_height()};

                                xJson::Value j;
                                j["account"] = node_id.to_string();
                                j["public_key"] = to_hex_str(election_info.public_key().to_string());
                                j["group_id"] = xip2.group_id().value();
                                j["stake"] = static_cast<unsigned long long>(election_info.stake());
                                j["node_type"] = common::to_presentation_string(node_type);

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

void xrpc_query_manager::set_fullunit_state(xJson::Value & j_fu, data::xblock_t * bp) {
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(bp);
    if (nullptr == unitstate) {
        xwarn("xrpc_query_manager::set_fullunit_state get target state fail.block=%s", bp->dump().c_str());
        return;
    }

    j_fu["latest_send_trans_number"] = static_cast<unsigned long long>(unitstate->account_send_trans_number());
    j_fu["latest_send_trans_hash"] = to_hex_str(unitstate->account_send_trans_hash());
    j_fu["latest_recv_trans_number"] = static_cast<unsigned long long>(unitstate->account_recv_trans_number());
    j_fu["account_balance"] = static_cast<unsigned long long>(unitstate->balance());
    j_fu["burned_amount_change"] = static_cast<unsigned long long>(unitstate->burn_balance());
    j_fu["account_create_time"] = static_cast<unsigned long long>(unitstate->get_account_create_time());
}

void xrpc_query_manager::set_body_info(xJson::Value & body, xblock_t * bp, const std::string & rpc_version) {
    auto block_level = bp->get_block_level();

    base::xvaccount_t _vaccount(bp->get_account());
    base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, bp, metrics::blockstore_access_from_rpc_get_block_set_table);
    base::xvchain_t::instance().get_xblockstore()->load_block_output(_vaccount, bp, metrics::blockstore_access_from_rpc_get_block_set_table);

    bp->parse_to_json(body, rpc_version);
    if (block_level == base::enum_xvblock_level_unit) {
        set_addition_info(body, bp);
        auto block_class = bp->get_block_class();
        if (block_class == base::enum_xvblock_class_full) {
            set_fullunit_state(body["fullunit"], bp);
        }
    }
}

xJson::Value xrpc_query_manager::get_block_json(xblock_t * bp, const std::string & rpc_version) {
    xJson::Value root;
    if (bp == nullptr) {
        return root;
    }

    if (bp->is_genesis_block() && bp->get_block_class() == base::enum_xvblock_class_nil && false == bp->check_block_flag(base::enum_xvblock_flag_stored)) {
        // genesis empty non-stored block, should not return
        return root;
    }

    // load input for raw tx get
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(base::xvaccount_t(bp->get_account()), bp, metrics::blockstore_access_from_rpc_get_block_json)) {
        xassert(false);  // db block should always load input success
        return root;
    }
    // load output for accountindex
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(base::xvaccount_t(bp->get_account()), bp, metrics::blockstore_access_from_rpc_get_block_json)) {
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

xJson::Value xrpc_query_manager::get_blocks_json(xblock_t * bp, const std::string & rpc_version) {
    xJson::Value root;
    if (bp == nullptr) {
        return root;
    }

    if (bp->is_genesis_block() && bp->get_block_class() == base::enum_xvblock_class_nil && false == bp->check_block_flag(base::enum_xvblock_flag_stored)) {
        // genesis empty non-stored block, should not return
        return root;
    }

    // load input for raw tx get
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(base::xvaccount_t(bp->get_account()), bp, metrics::blockstore_access_from_rpc_get_block_json)) {
        xassert(false);  // db block should always load input success
        return root;
    }
    // load output for accountindex
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(base::xvaccount_t(bp->get_account()), bp, metrics::blockstore_access_from_rpc_get_block_json)) {
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
    if (bp->check_block_flag(base::enum_xvblock_flag_committed)) {
        root["status"] = "committed";
    } else if (bp->check_block_flag(base::enum_xvblock_flag_locked)) {
        root["status"] = "locked";
    } else if (bp->check_block_flag(base::enum_xvblock_flag_authenticated)) {
        root["status"] = "cert";
    } else {
        root["status"] = "unknown";
    }
    return root;
}

void xrpc_query_manager::getSyncNeighbors(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    auto roles = m_sync->get_neighbors();
    xJson::Value v;
    for (auto role : roles) {
        for (auto peer : role.second) {
            v[role.first].append(peer);
        }
    }
    js_rsp["value"] = v;
}

void xrpc_query_manager::get_property(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    const string & account = js_req["account_addr"].asString();
    const string & type = js_req["type"].asString();
    const std::string & prop_name = js_req["data"].asString();

    // add top address check
    ADDRESS_CHECK_VALID(account)

    xJson::Value result_json;
    m_xrpc_query_func.query_account_property(result_json, account, prop_name, xrpc::xfull_node_compatible_mode_t::incompatible);
    js_rsp = result_json;
}

void xrpc_query_manager::getChainInfo(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    xJson::Value jv;
    base::xvaccount_t _timer_vaddress(sys_contract_beacon_timer_addr);
    auto vb = m_block_store->load_block_object(_timer_vaddress, 0, 0, true, metrics::blockstore_access_from_rpc_get_chain_info);
    xblock_t * bp = static_cast<xblock_t *>(vb.get());
    if (bp != nullptr) {
        jv["first_timerblock_hash"] = bp->get_block_hash_hex_str();
        jv["first_timerblock_stamp"] = static_cast<xJson::UInt64>(bp->get_timestamp());
    }
    jv["initial_total_gas_deposit"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_gas_deposit));
    jv["total_gas_shard"] = static_cast<xJson::UInt64>(XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard));
    jv["validator_group_count"] = XGET_CONFIG(validator_group_count);
    auto onchain_total_lock_tgas_token = store::xtgas_singleton::get_instance().get_cache_total_lock_tgas_token();
    jv["token_price"] = xunit_bstate_t::get_token_price(onchain_total_lock_tgas_token);

    xJson::Value tj;
    m_xrpc_query_func.query_account_property(tj, sys_contract_rec_registration_addr, data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, xfull_node_compatible_mode_t::incompatible);
    jv["network_activate_time"] = tj[data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY]["activation_time"].asUInt64();

    js_rsp = jv;
}

void xrpc_query_manager::queryNodeInfo(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    std::string target = js_req["node_account_addr"].asString();
    std::string version = js_req["version"].asString();
    if (version.empty()) {
        version = RPC_VERSION_V1;
    }
    xdbg("account: %s, target: %s", owner.c_str(), target.c_str());

    // add top address check
    if (!owner.empty()) {
        ADDRESS_CHECK_VALID(owner)
    }
    if (!target.empty()) {
        ADDRESS_CHECK_VALID(target)
    }

    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_registration_addr;
    std::string prop_name = data::system_contract::XPORPERTY_CONTRACT_REG_KEY;
    m_xrpc_query_func.query_account_property(jv, contract_addr, prop_name, xfull_node_compatible_mode_t::incompatible);

    if (target == "") {
        if (version == RPC_VERSION_V3) {
            auto tmp = jv[prop_name];
            for (auto i : tmp.getMemberNames()) {
                js_rsp.append(tmp[i]);
            }
        } else {
            js_rsp = jv[prop_name];
        }
    } else {
        js_rsp = jv[prop_name][target];
    }
}

void xrpc_query_manager::getElectInfo(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    std::string target = js_req["node_account_addr"].asString();
    xdbg("account: %s, target: %s", owner.c_str(), target.c_str());

    // add top address check
    ADDRESS_CHECK_VALID(owner)
    if (!target.empty()) {
        ADDRESS_CHECK_VALID(target)
    }

    std::vector<std::string> ev;
    xJson::Value j;

    std::string addr = sys_contract_zec_elect_consensus_addr;
    auto property_names = top::data::election::get_property_name_by_addr(common::xaccount_address_t{addr});
    for (auto property : property_names) {
        m_xrpc_query_func.query_account_property(j, addr, property, xfull_node_compatible_mode_t::incompatible);
        if (j[common::to_presentation_string(common::xnode_type_t::consensus_auditor)].isMember(target)) {
            ev.push_back("auditor");
        }
        if (j[common::to_presentation_string(common::xnode_type_t::consensus_validator)].isMember(target)) {
            ev.push_back("validator");
        }
    }

    addr = sys_contract_rec_elect_archive_addr;
    std::string prop_name = data::election::get_property_by_group_id(common::xarchive_group_id);
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (j[common::to_presentation_string(common::xnode_type_t::storage_archive)].isMember(target)) {
        ev.push_back("archiver");
    }

    addr = sys_contract_rec_elect_exchange_addr;
    prop_name = data::election::get_property_by_group_id(common::xexchange_group_id);
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);

    if (j[common::to_presentation_string(common::xnode_type_t::storage_exchange)].isMember(target)) {
        ev.push_back("exchange");
    }

    addr = sys_contract_rec_elect_edge_addr;
    prop_name = data::election::get_property_by_group_id(common::xdefault_group_id);
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (j[common::to_presentation_string(common::xnode_type_t::edge)].isMember(target)) {
        ev.push_back("edger");
    }

    addr = sys_contract_rec_elect_fullnode_addr;
    prop_name = data::election::get_property_by_group_id(common::xdefault_group_id);
    m_xrpc_query_func.query_account_property(j, addr, prop_name, xfull_node_compatible_mode_t::incompatible);
    if (j[common::to_presentation_string(common::xnode_type_t::fullnode)].isMember(target)) {
        ev.push_back("fullnode");
    }

    std::string elect_info;
    if (ev.empty()) {
        elect_info = "Not elected to any node role.";
    } else {
        elect_info = "Elected as ";
        for (size_t i = 0; i < ev.size(); ++i) {
            elect_info = elect_info + ev[i];
            if (i == ev.size() - 1) {
                elect_info = elect_info + ".";
            } else {
                elect_info = elect_info + ", ";
            }
        }
    }
    js_rsp = elect_info;
}

void xrpc_query_manager::listVoteUsed(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string target = js_req["node_account_addr"].asString();

    // add top address check
    ADDRESS_CHECK_VALID(target)

    uint32_t sub_map_no = (utl::xxh32_t::digest(target) % 4) + 1;
    std::string prop_name;
    prop_name = prop_name + data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(sub_map_no);
    set_sharding_vote_prop(js_req, js_rsp, prop_name, strResult, nErrorCode);
}

void xrpc_query_manager::queryVoterDividend(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string target = js_req["node_account_addr"].asString();

    // add top address check
    ADDRESS_CHECK_VALID(target)

    uint32_t sub_map_no = (utl::xxh32_t::digest(target) % 4) + 1;
    std::string prop_name;
    prop_name = prop_name + data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);
    set_sharding_reward_claiming_prop(js_req, js_rsp, prop_name, strResult, nErrorCode);
}

void xrpc_query_manager::queryProposal(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    std::string target = js_req["proposal_id"].asString();
    std::string proposal_version = js_req["proposal_version"].asString();
    xdbg("account: %s, target: %s, proposal_version: %s", owner.c_str(), target.c_str(), proposal_version.c_str());

    xJson::Value jv;
    std::string contract_addr = sys_contract_rec_tcc_addr;
    std::string prop_name = PROPOSAL_MAP_ID;
    m_xrpc_query_func.query_account_property(jv, contract_addr, prop_name, xfull_node_compatible_mode_t::incompatible);

    if (target == "") {
        js_rsp = jv[prop_name];
    } else {
        target = "proposal_id_" + target;
        js_rsp = jv[prop_name][target];
    }
}

void xrpc_query_manager::getLatestTables(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    std::string owner = js_req["account_addr"].asString();
    xdbg("getLatestTables account: %s", owner.c_str());

    xJson::Value jv;
    for (auto i = 0; i < enum_vbucket_has_tables_count; ++i) {
        std::string addr = xblocktool_t::make_address_shard_table_account(i);
        auto vb = m_block_store->get_latest_committed_block(addr, metrics::blockstore_access_from_rpc_get_latest_tables);
        jv.append(static_cast<xJson::UInt64>(vb->get_height()));
        xdbg("getLatestTables addr %s, height %ull", addr.c_str(), vb->get_height());
    }

    js_rsp = jv;
}

}  // namespace chain_info
}  // namespace top
