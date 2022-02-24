// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"

#include "secp256k1/secp256k1.h"
#include "secp256k1/secp256k1_recovery.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xcommon/xrole_type.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xproperty.h"
#include "xdata/xrootblock.h"
#include "xdata/xslash.h"
#include "xmetrics/xmetrics.h"
#include "xstore/xstore_error.h"

using top::base::xcontext_t;
using top::base::xstream_t;
using namespace top::data;

#if !defined (XREC_MODULE)
#    define XREC_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "registration_"

#define XREG_CONTRACT XREC_MODULE XCONTRACT_PREFIX

NS_BEG2(top, xstake)

#define TIMER_ADJUST_DENOMINATOR 10

xrec_registration_contract::xrec_registration_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xrec_registration_contract::setup() {
    MAP_CREATE(XPORPERTY_CONTRACT_REG_KEY);
    MAP_CREATE(XPORPERTY_CONTRACT_TICKETS_KEY);
    MAP_CREATE(XPORPERTY_CONTRACT_REFUND_KEY);
    STRING_CREATE(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
    MAP_CREATE(XPROPERTY_CONTRACT_SLASH_INFO_KEY);

    std::vector<std::pair<std::string, std::string>> db_kv_101;
    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, XPORPERTY_CONTRACT_REG_KEY, db_kv_101);
    for (auto const & _p : db_kv_101) {
        auto const & node_info_serialized = _p.second;

        base::xstream_t stream{ base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(node_info_serialized.data())), static_cast<uint32_t>(node_info_serialized.size()) };
        xreg_node_info node_info;
        node_info.serialize_from(stream);

        auto const old_role = static_cast<common::xlegacy_miner_type_t>(node_info.miner_type());
        switch (old_role) {
        case common::xlegacy_miner_type_t::advance:
            node_info.miner_type(common::xminer_type_t::advance);
            break;

        case common::xlegacy_miner_type_t::consensus:
            node_info.miner_type(common::xminer_type_t::validator);
            break;

        case common::xlegacy_miner_type_t::archive:
            node_info.miner_type(common::xminer_type_t::archive);
            break;

        case common::xlegacy_miner_type_t::edge:
            node_info.miner_type(common::xminer_type_t::edge);
            break;

        default:
            break;
        }
        stream.reset();
        node_info.serialize_to(stream);

        MAP_SET(XPORPERTY_CONTRACT_REG_KEY, _p.first, { reinterpret_cast<char *>(stream.data()), static_cast<size_t>(stream.size()) });
    }

    std::vector<std::pair<std::string, std::string>> db_kv_128;
    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, XPORPERTY_CONTRACT_REFUND_KEY, db_kv_128);
    for (auto const & _p : db_kv_128) {
        MAP_SET(XPORPERTY_CONTRACT_REFUND_KEY, _p.first, _p.second);
    }

    std::vector<std::pair<std::string, std::string>> db_kv_132;
    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, XPROPERTY_CONTRACT_SLASH_INFO_KEY, db_kv_132);
    for (auto const & _p : db_kv_132) {
        MAP_SET(XPROPERTY_CONTRACT_SLASH_INFO_KEY, _p.first, _p.second);
    }

    std::string db_kv_129;
    chain_data::xchain_data_processor_t::get_stake_string_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, db_kv_129);
    if (!db_kv_129.empty()) {
        STRING_SET(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, db_kv_129);
    }

    const uint32_t old_tables_count = 256;
    for (auto table = 0; table < enum_vledger_const::enum_vbucket_has_tables_count; table++) {
        std::string table_address{std::string{sys_contract_sharding_vote_addr} + "@" + std::to_string(table)};
        std::map<std::string, uint64_t> adv_get_votes_detail;
        for (auto i = 1; i <= xstake::XPROPERTY_SPLITED_NUM; i++) {
            std::string property;
            property = property + XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(i);
            {
                std::map<std::string, std::map<std::string, uint64_t>> votes_detail;
                for (uint32_t j = 0; j < old_tables_count; j++) {
                    auto table_addr = std::string{sys_contract_sharding_vote_addr} + "@" + base::xstring_utl::tostring(j);
                    std::vector<std::pair<std::string, std::string>> db_kv_112;
                    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{table_addr}, property, db_kv_112);
                    for (auto const & _p : db_kv_112) {
                        base::xvaccount_t vaccount{_p.first};
                        auto account_table_id = vaccount.get_ledger_subaddr();
                        if (static_cast<uint16_t>(account_table_id) != static_cast<uint16_t>(table)) {
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
                }
            }
        }
        {
            if(adv_get_votes_detail.empty()) {
                continue;
            }
            std::map<std::string, std::string> adv_get_votes_str_detail;
            for (auto const & adv_get_votes : adv_get_votes_detail) {
                adv_get_votes_str_detail.insert(std::make_pair(adv_get_votes.first, base::xstring_utl::tostring(adv_get_votes.second)));
            }
            xstream_t stream(xcontext_t::instance());
            stream << adv_get_votes_str_detail;
            std::string adv_get_votes_str{std::string((const char*)stream.data(), stream.size())};
            MAP_SET(XPORPERTY_CONTRACT_TICKETS_KEY, table_address, adv_get_votes_str);
        }
    }

    top::chain_data::data_processor_t data;
    top::chain_data::xtop_chain_data_processor::get_contract_data(common::xlegacy_account_address_t{SELF_ADDRESS()}, data);
    TOP_TOKEN_INCREASE(data.top_balance);

    auto const & source_address = SOURCE_ADDRESS();
    MAP_CREATE(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY);
    MAP_SET(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, source_address, base::xstring_utl::tostring(0));

#ifdef MAINNET_ACTIVATED
    xactivation_record record;
    record.activated = 1;
    record.activation_time = 1;

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    STRING_SET(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, value_str);
#endif

    xdbg("[xrec_registration_contract::setup] pid:%d", getpid());
    xreg_node_info node_info;
    {
        common::xnetwork_id_t network_id{top::config::to_chainid(XGET_CONFIG(chain_name))};
        const std::vector<node_info_t> & seed_nodes = data::xrootblock_t::get_seed_nodes();
        for (size_t i = 0; i < seed_nodes.size(); i++) {
            node_info_t node_data = seed_nodes[i];

            if (MAP_FIELD_EXIST(XPORPERTY_CONTRACT_REG_KEY, node_data.m_account.value())) {
                continue;
            }
            node_info.m_account             = node_data.m_account;
            node_info.m_account_mortgage    = 0;
            node_info.m_genesis_node        = true;
#if defined(XBUILD_DEV)
            node_info.miner_type(common::xminer_type_t::edge | common::xminer_type_t::advance | common::xminer_type_t::validator | common::xminer_type_t::archive);
#else
            node_info.miner_type(common::xminer_type_t::edge | common::xminer_type_t::advance | common::xminer_type_t::validator);
#endif
            node_info.m_network_ids.insert(network_id);
            node_info.nickname              = std::string("bootnode") + std::to_string(i + 1);
            node_info.consensus_public_key  = xpublic_key_t{node_data.m_publickey};
            //xdbg("[xrec_registration_contract::setup] pid:%d,node account: %s, public key: %s\n", getpid(), node_data.m_account.c_str(), node_data.m_publickey.c_str());
            update_node_info(node_info);
        }
    }
}

void xrec_registration_contract::registerNode(const std::string & miner_type_name,
                                              const std::string & nickname,
                                              const std::string & signing_key,
                                              const uint32_t dividend_rate
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                            , common::xaccount_address_t const & registration_account
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
) {
    std::set<common::xnetwork_id_t> network_ids;
    common::xnetwork_id_t nid{top::config::to_chainid(XGET_CONFIG(chain_name))};
    network_ids.insert(nid);

#if defined(XENABLE_MOCK_ZEC_STAKE)
    registerNode2(miner_type_name, nickname, signing_key, dividend_rate, network_ids, registration_account);
#else   // #if defined(XENABLE_MOCK_ZEC_STAKE)
    registerNode2(miner_type_name, nickname, signing_key, dividend_rate, network_ids);
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
}

void xrec_registration_contract::registerNode2(const std::string & miner_type_name,
                                               const std::string & nickname,
                                               const std::string & signing_key,
                                               const uint32_t dividend_rate,
                                               const std::set<common::xnetwork_id_t> & network_ids
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                               ,
                                               common::xaccount_address_t const & registration_account
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "registerNode_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "registerNode_ExecutionTime");

    auto const miner_type = common::to_miner_type(miner_type_name);
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "xrec_registration_contract::registerNode2: invalid node_type!");

#if defined(XENABLE_MOCK_ZEC_STAKE)
    auto const & account = registration_account;
#else   // #if defined(XENABLE_MOCK_ZEC_STAKE)
    auto const & account = common::xaccount_address_t{ SOURCE_ADDRESS() };
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
    xdbg("[xrec_registration_contract::registerNode2] call xregistration_contract registerNode() pid:%d, balance: %lld, account: %s, node_types: %s, signing_key: %s, dividend_rate: %u\n",
         getpid(),
         GET_BALANCE(),
         account.c_str(),
         miner_type_name.c_str(),
         signing_key.c_str(),
         dividend_rate);

    xreg_node_info node_info;
    auto ret = get_node_info(account.value(), node_info);
    XCONTRACT_ENSURE(ret != 0, "xrec_registration_contract::registerNode2: node exist!");
    XCONTRACT_ENSURE(is_valid_name(nickname), "xrec_registration_contract::registerNode: invalid nickname");
    XCONTRACT_ENSURE(dividend_rate >= 0 && dividend_rate <= 100, "xrec_registration_contract::registerNode: dividend_rate must be >=0 and be <= 100");

    const xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(!account.empty(),
                     "xrec_registration_contract::registerNode: account must be not empty");

    xstream_t stream(xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(trans_ptr->get_source_action_para().data())), static_cast<uint32_t>(trans_ptr->get_source_action_para().size()));

    data::xproperty_asset asset_out{0};
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;

    node_info.m_account = account;
    node_info.miner_type(miner_type);
#if defined(XENABLE_MOCK_ZEC_STAKE)
    node_info.m_account_mortgage = 100000000000000;
#else   // #if defined(XENABLE_MOCK_ZEC_STAKE)
    node_info.m_account_mortgage += asset_out.m_amount;
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
    node_info.nickname                  = nickname;
    node_info.consensus_public_key      = xpublic_key_t{signing_key};
    node_info.m_support_ratio_numerator = dividend_rate;
    if (network_ids.empty()) {
        xdbg("[xrec_registration_contract::registerNode2] network_ids empty");
        common::xnetwork_id_t nid{top::config::to_chainid(XGET_CONFIG(chain_name))};
        node_info.m_network_ids.insert(nid);
    } else {
        std::string network_ids_str;
        for (auto const & net_id : network_ids) {
            network_ids_str += net_id.to_string() + ' ';
        }
        xdbg("[xrec_registration_contract::registerNode2] network_ids %s", network_ids_str.c_str());
        node_info.m_network_ids = network_ids;
    }

    auto const& fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool isforked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.node_initial_credit_fork_point, TIME());
    init_node_credit(node_info, isforked);

    uint64_t const min_deposit = node_info.get_required_min_deposit();
    xdbg("[xrec_registration_contract::registerNode2] call xregistration_contract registerNode() pid:%d, transaction_type:%d, source action type: %d, m_deposit: %" PRIu64
         ", min_deposit: %" PRIu64 ", account: %s",
         getpid(),
         trans_ptr->get_tx_type(),
         trans_ptr->get_source_action_type(),
         asset_out.m_amount,
         min_deposit,
         account.c_str());
    //XCONTRACT_ENSURE(asset_out.m_amount >= min_deposit, "xrec_registration_contract::registerNode2: mortgage must be greater than minimum deposit");
    XCONTRACT_ENSURE(node_info.m_account_mortgage >= min_deposit, "xrec_registration_contract::registerNode2: mortgage must be greater than minimum deposit");

    update_node_info(node_info);
    check_and_set_genesis_stage();

    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "registerNode_Executed", 1);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "registeredUserCnt", 1);
}

void xrec_registration_contract::unregisterNode() {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "unregisterNode_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "unregisterNode_ExecutionTime");
    uint64_t cur_time = TIME();
    std::string const& account = SOURCE_ADDRESS();
    xdbg("[xrec_registration_contract::unregisterNode] call xregistration_contract unregisterNode(), balance: %lld, account: %s", GET_BALANCE(), account.c_str());

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::unregisterNode: node not exist");

    xslash_info s_info;
    if (get_slash_info(account, s_info) == 0 && s_info.m_staking_lock_time > 0) {
        XCONTRACT_ENSURE(cur_time - s_info.m_punish_time >= s_info.m_staking_lock_time, "[xrec_registration_contract::unregisterNode]: has punish time, cannot deregister now");
    }

    xdbg("[xrec_registration_contract::unregisterNode] call xregistration_contract unregisterNode() pid:%d, balance:%lld, account: %s, refund: %lld",
    getpid(), GET_BALANCE(), account.c_str(), node_info.m_account_mortgage);
    // refund
    // TRANSFER(account, node_info.m_account_mortgage);

    ins_refund(account, node_info.m_account_mortgage);

    delete_node_info(account);

    xdbg("[xrec_registration_contract::unregisterNode] finish call xregistration_contract unregisterNode() pid:%d", getpid());

    XMETRICS_COUNTER_DECREMENT(XREG_CONTRACT "registeredUserCnt", 1);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "unregisterNode_Executed", 1);
}

void xrec_registration_contract::updateNodeInfo(const std::string & nickname, const int updateDepositType, const uint64_t deposit, const uint32_t dividend_rate, const std::string & node_types, const std::string & node_sign_key) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeInfo_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "updateNodeInfo_ExecutionTime");

    std::string const & account = SOURCE_ADDRESS();

    xdbg("[xrec_registration_contract::updateNodeInfo] call xregistration_contract updateNodeInfo() pid:%d, balance: %lld, account: %s, nickname: %s, updateDepositType: %u, deposit: %llu, dividend_rate: %u, node_types: %s, node_sign_key: %s",
         getpid(),
         GET_BALANCE(),
         account.c_str(),
         nickname.c_str(),
         updateDepositType,
         deposit,
         dividend_rate,
         node_types.c_str(),
         node_sign_key.c_str());

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::updateNodeInfo: node does not exist!");
    XCONTRACT_ENSURE(is_valid_name(nickname) == true, "xrec_registration_contract::updateNodeInfo: invalid nickname");
    XCONTRACT_ENSURE(updateDepositType == 1 || updateDepositType == 2, "xrec_registration_contract::updateNodeInfo: invalid updateDepositType");
    XCONTRACT_ENSURE(dividend_rate >= 0 && dividend_rate <= 100, "xrec_registration_contract::updateNodeInfo: dividend_rate must be greater than or be equal to zero");
    auto const miner_type = common::to_miner_type(node_types);
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "xrec_registration_contract::updateNodeInfo: invalid node_type!");

    node_info.nickname          = nickname;
    node_info.miner_type(miner_type);

    uint64_t const min_deposit = node_info.get_required_min_deposit();
    if (updateDepositType == 1) { // stake deposit
        const xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
        XCONTRACT_ENSURE(!account.empty(),
                         "xrec_registration_contract::updateNodeInfo: account must be not empty");
        if (trans_ptr->get_source_action_para().size() > 0) {
            xstream_t stream(xcontext_t::instance(), (uint8_t *)trans_ptr->get_source_action_para().data(), trans_ptr->get_source_action_para().size());
            data::xproperty_asset asset_out{0};
            stream >> asset_out.m_token_name;
            stream >> asset_out.m_amount;
            XCONTRACT_ENSURE(asset_out.m_amount == deposit, "xrec_registration_contract::updateNodeInfo: invalid deposit!");
            node_info.m_account_mortgage += asset_out.m_amount;
        }

        XCONTRACT_ENSURE(node_info.m_account_mortgage >= min_deposit, "xrec_registration_contract::updateNodeInfo: deposit not enough");
    } else {

        XCONTRACT_ENSURE(deposit <= node_info.m_account_mortgage && node_info.m_account_mortgage - deposit >= min_deposit, "xrec_registration_contract::updateNodeInfo: unstake deposit too big");
        if (deposit > 0) {
            TRANSFER(account, deposit);
        }
        node_info.m_account_mortgage -= deposit;
    }

    if (node_info.m_support_ratio_numerator != dividend_rate) {
        uint64_t cur_time = TIME();
        auto SDR_INTERVAL = XGET_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval);
        XCONTRACT_ENSURE(node_info.m_last_update_time == 0 || cur_time - node_info.m_last_update_time >= SDR_INTERVAL,
                     "xrec_registration_contract::updateNodeInfo: set must be longer than or equal to SDR_INTERVAL");

        node_info.m_support_ratio_numerator = dividend_rate;
        node_info.m_last_update_time = cur_time;
    }
    node_info.consensus_public_key = xpublic_key_t{node_sign_key};
    auto const& fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool isforked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.node_initial_credit_fork_point, TIME());
    init_node_credit(node_info, isforked);

    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeInfo_Executed", 1);
}

void xrec_registration_contract::redeemNodeDeposit() {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "redeemNodeDeposit_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "redeemNodeDeposit_ExecutionTime");
    uint64_t cur_time = TIME();
    std::string const & account = SOURCE_ADDRESS();
    xdbg("[xrec_registration_contract::redeemNodeDeposit] pid:%d, balance: %lld, account: %s\n", getpid(), GET_BALANCE(), account.c_str());

    xrefund_info refund;
    auto ret = get_refund(account, refund);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::redeemNodeDeposit: refund not exist");
    XCONTRACT_ENSURE(cur_time - refund.create_time >= REDEEM_INTERVAL, "xrec_registration_contract::redeemNodeDeposit: interval must be greater than or equal to REDEEM_INTERVAL");

    xdbg("[xrec_registration_contract::redeemNodeDeposit] pid:%d, balance:%llu, account: %s, refund amount: %llu\n", getpid(), GET_BALANCE(), account.c_str(), refund.refund_amount);
    // refund
    TRANSFER(account, refund.refund_amount);

    del_refund(account);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "redeemNodeDeposit_Executed", 1);
}

void xrec_registration_contract::setDividendRatio(uint32_t dividend_rate) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "setDividendRatio_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "setDividendRatio_ExecutionTime");
    std::string const & account = SOURCE_ADDRESS();
    uint64_t cur_time = TIME();

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);

    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::setDividendRatio: node not exist");

    xdbg("[xrec_registration_contract::setDividendRatio] pid:%d, balance: %lld, account: %s, dividend_rate: %u, cur_time: %llu, last_update_time: %llu\n",
         getpid(),
         GET_BALANCE(),
         account.c_str(),
         dividend_rate,
         cur_time,
         node_info.m_last_update_time);

    auto SDR_INTERVAL = XGET_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval);
    XCONTRACT_ENSURE(node_info.m_last_update_time == 0 || cur_time - node_info.m_last_update_time >= SDR_INTERVAL,
                     "xrec_registration_contract::setDividendRatio: set must be longer than or equal to SDR_INTERVAL");
    XCONTRACT_ENSURE(dividend_rate >= 0 && dividend_rate <= 100, "xrec_registration_contract::setDividendRatio: dividend_rate must be >=0 and be <= 100");

    xdbg("[xrec_registration_contract::setDividendRatio] call xregistration_contract registerNode() pid:%d, balance:%lld, account: %s\n",
         getpid(),
         GET_BALANCE(),
         account.c_str());
    node_info.m_support_ratio_numerator = dividend_rate;
    node_info.m_last_update_time = cur_time;

    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "setDividendRatio_Executed", 1);
}

void xrec_registration_contract::setNodeName(const std::string & nickname) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "setNodeName_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "setNodeName_ExecutionTime");
    std::string const & account = SOURCE_ADDRESS();

    XCONTRACT_ENSURE(is_valid_name(nickname) == true, "xrec_registration_contract::setNodeName: invalid nickname");

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::setNodeName: node not exist");

    xdbg("[xrec_registration_contract::setNodeName] pid:%d, balance: %lld, account: %s, nickname: %s\n", getpid(), GET_BALANCE(), account.c_str(), nickname.c_str());
    XCONTRACT_ENSURE(node_info.nickname != nickname, "xrec_registration_contract::setNodeName: nickname can not be same");

    node_info.nickname = nickname;
    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "setNodeName_Executed", 1);
}

void xrec_registration_contract::updateNodeSignKey(const std::string & node_sign_key) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeSignKey_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "updateNodeSignKey_ExecutionTime");
    std::string const & account = SOURCE_ADDRESS();

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::updateNodeSignKey: node not exist");

    xdbg("[xrec_registration_contract::updateNodeSignKey] pid:%d, balance: %lld, account: %s, node_sign_key: %s\n", getpid(), GET_BALANCE(), account.c_str(), node_sign_key.c_str());
    XCONTRACT_ENSURE(node_info.consensus_public_key.to_string() != node_sign_key, "xrec_registration_contract::updateNodeSignKey: node_sign_key can not be same");

    node_info.consensus_public_key = xpublic_key_t{node_sign_key};
    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeSignKey_Executed", 1);
}

void xrec_registration_contract::update_node_info(xreg_node_info const & node_info) {
    base::xstream_t stream(base::xcontext_t::instance());
    node_info.serialize_to(stream);

    std::string stream_str = std::string((char *)stream.data(), stream.size());

    XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_SetExecutionTime");
    MAP_SET(XPORPERTY_CONTRACT_REG_KEY, node_info.m_account.value(), stream_str);
}

void xrec_registration_contract::delete_node_info(std::string const & account) {
    XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_RemoveExecutionTime");
    REMOVE(enum_type_t::map, XPORPERTY_CONTRACT_REG_KEY, account);
}

int32_t xrec_registration_contract::get_node_info(const std::string & account, xreg_node_info & node_info) {
    std::string value_str;
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_GetExecutionTime");
        MAP_GET2(XPORPERTY_CONTRACT_REG_KEY, account, value_str);
    }

    if (value_str.empty()) {
        xdbg("[xrec_registration_contract] account(%s) not exist pid:%d\n", account.c_str(), getpid());
        return xaccount_property_not_exist;
    }

    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());

    node_info.serialize_from(stream);

    return 0;
}

bool xrec_registration_contract::check_if_signing_key_exist(const std::string & signing_key) {
    std::map<std::string, std::string> map_nodes;

    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_REG_KEY, map_nodes);
    }

    for (auto const & it : map_nodes) {
        xstake::xreg_node_info reg_node_info;
        xstream_t stream(xcontext_t::instance(), (uint8_t *)it.second.c_str(), it.second.size());
        reg_node_info.serialize_from(stream);
        if (reg_node_info.consensus_public_key.to_string() == signing_key) return true;
    }

    return false;
}

int32_t xrec_registration_contract::ins_refund(const std::string & account, uint64_t const & refund_amount) {
    xrefund_info refund;
    get_refund(account, refund);

    refund.refund_amount += refund_amount;
    refund.create_time   = TIME();

    base::xstream_t stream(base::xcontext_t::instance());

    refund.serialize_to(stream);
    std::string stream_str = std::string((char *)stream.data(), stream.size());

    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REFUND_KEY_SetExecutionTime");
        MAP_SET(XPORPERTY_CONTRACT_REFUND_KEY, account, stream_str);
    }

    return 0;
}

int32_t xrec_registration_contract::del_refund(const std::string & account) {
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REFUND_KEY_RemoveExecutionTime");
        REMOVE(enum_type_t::map, XPORPERTY_CONTRACT_REFUND_KEY, account);
    }

    return 0;
}

int32_t xrec_registration_contract::get_refund(const std::string & account, xrefund_info & refund) {
    std::string value_str;
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REFUND_KEY_GetExecutionTime");
        MAP_GET2(XPORPERTY_CONTRACT_REFUND_KEY, account, value_str);
    }

    if (value_str.empty()) {
        xdbg("[xrec_registration_contract::get_refund] account(%s) not exist pid:%d\n", account.c_str(), getpid());
        return xaccount_property_not_exist;
    }

    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());

    refund.serialize_from(stream);

    return 0;
}

void xrec_registration_contract::update_batch_stake(std::map<std::string, std::string> const & contract_adv_votes) {
    XMETRICS_TIME_RECORD(XREG_CONTRACT "update_batch_stake_ExecutionTime");
    auto const & source_address = SOURCE_ADDRESS();
    xdbg("[xrec_registration_contract::update_batch_stake] src_addr: %s, pid:%d, size: %d\n", source_address.c_str(), getpid(), contract_adv_votes.size());

    std::string base_addr;
    uint32_t    table_id;
    if (!data::xdatautil::extract_parts(source_address, base_addr, table_id) || sys_contract_sharding_vote_addr != base_addr) {
        xwarn("[xrec_registration_contract::update_batch_stake] invalid call from %s", source_address.c_str());
        return;
    }

    xstream_t stream(xcontext_t::instance());
    stream << contract_adv_votes;
    std::string contract_adv_votes_str = std::string((const char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_TICKETS_KEY_SetExecutionTime");
        MAP_SET(XPORPERTY_CONTRACT_TICKETS_KEY, source_address, contract_adv_votes_str);
    }

    std::map<std::string, std::string> votes_table;
    try {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_TICKETS_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_TICKETS_KEY, votes_table);
    } catch (std::runtime_error & e) {
        xdbg("[xrec_registration_contract::update_batch_stake] MAP COPY GET error:%s", e.what());
    }

    std::map<std::string, std::string> map_nodes;
    try {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_REG_KEY, map_nodes);
    } catch (std::runtime_error & e) {
        xdbg("[xrec_registration_contract::update_batch_stake] MAP COPY GET error:%s", e.what());
    }

    auto update_adv_votes = [&](std::string adv_account, std::map<std::string, std::string> votes_table) {
        uint64_t total_votes = 0;
        for (auto const & vote : votes_table) {
            auto const & vote_str = vote.second;
            std::map<std::string, std::string> contract_votes;
            if (!vote_str.empty()) {
                base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_str.c_str(), (uint32_t)vote_str.size());
                stream >> contract_votes;
            }

            auto iter = contract_votes.find(adv_account);
            if (iter != contract_votes.end()) {
                total_votes += base::xstring_utl::touint64(iter->second);
            }
        }

        return total_votes;
    };

    for (auto const & entity : map_nodes) {
        auto const & account = entity.first;
        auto const & reg_node_str = entity.second;
        if (reg_node_str.empty()) {
            continue;
        }
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)reg_node_str.data(), reg_node_str.size());
        xreg_node_info reg_node_info;
        reg_node_info.serialize_from(stream);

        if (!reg_node_info.could_be_auditor()) {
            continue;
        }

        reg_node_info.m_vote_amount = update_adv_votes(account, votes_table);
        //reg_node_info.calc_stake();
        update_node_info(reg_node_info);
    }

    check_and_set_genesis_stage();
}

bool xrec_registration_contract::handle_receive_shard_votes(uint64_t report_time, uint64_t last_report_time, std::map<std::string, std::string> const & contract_adv_votes, std::map<std::string, std::string> & merge_contract_adv_votes) {
    xdbg("[xrec_registration_contract::handle_receive_shard_votes] report vote table size: %d, original vote table size: %d",
            contract_adv_votes.size(), merge_contract_adv_votes.size());
    if (report_time < last_report_time) {
        return false;
    }
    if (report_time == last_report_time) {
        merge_contract_adv_votes.insert(contract_adv_votes.begin(), contract_adv_votes.end());
        xdbg("[xrec_registration_contract::handle_receive_shard_votes] same batch of vote report, report vote table size: %d, total size: %d",
            contract_adv_votes.size(), merge_contract_adv_votes.size());
    } else {
        merge_contract_adv_votes = contract_adv_votes;
    }
    return true;
}

void xrec_registration_contract::update_batch_stake_v2(uint64_t report_time, std::map<std::string, std::string> const & contract_adv_votes) {
    XMETRICS_TIME_RECORD(XREG_CONTRACT "update_batch_stake_ExecutionTime");
    auto const & source_address = SOURCE_ADDRESS();
    xdbg("[xrec_registration_contract::update_batch_stake_v2] src_addr: %s, report_time: %llu, pid:%d, contract_adv_votes size: %d\n",
        source_address.c_str(), report_time, getpid(), contract_adv_votes.size());

    std::string base_addr;
    uint32_t    table_id;
    if (!data::xdatautil::extract_parts(source_address, base_addr, table_id) || sys_contract_sharding_vote_addr != base_addr) {
        xwarn("[xrec_registration_contract::update_batch_stake_v2] invalid call from %s", source_address.c_str());
        return;
    }

    // if (!MAP_PROPERTY_EXIST(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY)) {
    //     MAP_CREATE(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY);
    //     MAP_SET(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, source_address, base::xstring_utl::tostring(0));
    // }
    bool replace = true;
    std::string value_str;
    uint64_t last_report_time = 0;
    MAP_GET2(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, source_address, value_str);
    if (!value_str.empty()) {
        last_report_time = base::xstring_utl::touint64(value_str);
        xdbg("[xrec_registration_contract::update_batch_stake_v2] last_report_time: %llu", last_report_time);
    }
    MAP_SET(XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, source_address, base::xstring_utl::tostring(report_time));

    {
        std::map<std::string, std::string> auditor_votes;
        std::string auditor_votes_str;
        MAP_GET2(XPORPERTY_CONTRACT_TICKETS_KEY, source_address, auditor_votes_str);
        if (!auditor_votes_str.empty()) {
            base::xstream_t votes_stream(base::xcontext_t::instance(), (uint8_t *)auditor_votes_str.c_str(), (uint32_t)auditor_votes_str.size());
            votes_stream >> auditor_votes;
        }
        if ( !handle_receive_shard_votes(report_time, last_report_time, contract_adv_votes, auditor_votes) ) {
            XCONTRACT_ENSURE(false, "[xrec_registration_contract::on_receive_shard_votes_v2] handle_receive_shard_votes fail");
        }

        xstream_t stream(xcontext_t::instance());
        stream << auditor_votes;
        std::string contract_adv_votes_str = std::string((const char *)stream.data(), stream.size());
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_TICKETS_KEY_SetExecutionTime");
        MAP_SET(XPORPERTY_CONTRACT_TICKETS_KEY, source_address, contract_adv_votes_str);
    }

    std::map<std::string, std::string> votes_table;
    try {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_TICKETS_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_TICKETS_KEY, votes_table);
    } catch (std::runtime_error & e) {
        xdbg("[xrec_registration_contract::update_batch_stake_v2] MAP COPY GET error:%s", e.what());
    }

    std::map<std::string, std::string> map_nodes;
    try {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_REG_KEY, map_nodes);
    } catch (std::runtime_error & e) {
        xdbg("[xrec_registration_contract::update_batch_stake_v2] MAP COPY GET error:%s", e.what());
    }

    auto update_adv_votes = [&](std::string adv_account, std::map<std::string, std::string> votes_table) {
        uint64_t total_votes = 0;
        for (auto const & vote : votes_table) {
            auto const & vote_str = vote.second;
            std::map<std::string, std::string> contract_votes;
            if (!vote_str.empty()) {
                base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_str.c_str(), (uint32_t)vote_str.size());
                stream >> contract_votes;
            }

            auto iter = contract_votes.find(adv_account);
            if (iter != contract_votes.end()) {
                total_votes += base::xstring_utl::touint64(iter->second);
            }
        }

        return total_votes;
    };

    for (auto const & entity : map_nodes) {
        auto const & account = entity.first;
        auto const & reg_node_str = entity.second;
        if (reg_node_str.empty()) {
            continue;
        }
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)reg_node_str.data(), reg_node_str.size());
        xreg_node_info reg_node_info;
        reg_node_info.serialize_from(stream);

        reg_node_info.m_vote_amount = update_adv_votes(account, votes_table);
        update_node_info(reg_node_info);
    }

    check_and_set_genesis_stage();
}

void xrec_registration_contract::check_and_set_genesis_stage() {
    std::string value_str;
    xactivation_record record;

    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_GENESIS_STAGE_KEY_GetExecutionTime");
        value_str = STRING_GET(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
    }

    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
    }
    if (record.activated) {
        xinfo("[xrec_registration_contract::check_and_set_genesis_stage] activated: %d, activation_time: %llu, pid:%d\n", record.activated, record.activation_time, getpid());
        return;
    }

    std::map<std::string, std::string> map_nodes;
    try {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_REG_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(XPORPERTY_CONTRACT_REG_KEY, map_nodes);
    } catch (std::runtime_error & e) {
        xdbg("[xrec_registration_contract::check_and_set_genesis_stage] MAP COPY GET error:%s", e.what());
    }

    auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    auto const fullnode_enabled = chain_fork::xchain_fork_config_center_t::is_forked(fork_config.enable_fullnode_election_fork_point, TIME());
    bool active = check_registered_nodes_active(map_nodes, fullnode_enabled);
    if (active) {
        record.activated = 1;
        record.activation_time = TIME();
    }

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    value_str = std::string((char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPORPERTY_CONTRACT_GENESIS_STAGE_KEY_SetExecutionTime");
        STRING_SET(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, value_str);
    }
}

void xrec_registration_contract::updateNodeType(const std::string & node_types) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeType_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "updateNodeType_ExecutionTime");
    std::string const& account = SOURCE_ADDRESS();

    xdbg("[xrec_registration_contract::updateNodeType] pid: %d, balance: %lld, account: %s, node_types: %s", getpid(), GET_BALANCE(), account.c_str(), node_types.c_str());

    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::updateNodeType: node not exist");

    auto const miner_type = common::to_miner_type(node_types);
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "xrec_registration_contract::updateNodeType: invalid node_type!");
    XCONTRACT_ENSURE(miner_type != node_info.miner_type(), "xrec_registration_contract::updateNodeType: node_types can not be same!");

    const xtransaction_ptr_t trans_ptr = GET_TRANSACTION();
    XCONTRACT_ENSURE(!account.empty(), "xrec_registration_contract::updateNodeType: account must be not empty");

    if (trans_ptr->get_source_action_para().size() > 0) {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)trans_ptr->get_source_action_para().data(), trans_ptr->get_source_action_para().size());

        data::xproperty_asset asset_out{0};
        stream >> asset_out.m_token_name;
        stream >> asset_out.m_amount;

        node_info.m_account_mortgage += asset_out.m_amount;
    }

    node_info.miner_type(miner_type);
    uint64_t min_deposit = node_info.get_required_min_deposit();
    xdbg(("[xrec_registration_contract::updateNodeType] min_deposit: %" PRIu64 ", account: %s, account morgage: %llu\n"),
        min_deposit, account.c_str(), node_info.m_account_mortgage);
    XCONTRACT_ENSURE(node_info.m_account_mortgage >= min_deposit, "xrec_registration_contract::updateNodeType: deposit not enough");

    auto const& fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool isforked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.node_initial_credit_fork_point, TIME());
    init_node_credit(node_info, isforked);
    update_node_info(node_info);
    check_and_set_genesis_stage();

    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "updateNodeType_Executed", 1);
}

void xrec_registration_contract::stakeDeposit() {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "stakeDeposit_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "stakeDeposit_ExecutionTime");
    std::string const& account = SOURCE_ADDRESS();
    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::stakeDeposit: node not exist");

    const xtransaction_ptr_t trans_ptr = GET_TRANSACTION();

    xstream_t stream(xcontext_t::instance(), (uint8_t*)trans_ptr->get_source_action_para().data(), trans_ptr->get_source_action_para().size());

    data::xproperty_asset asset_out{0};
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;

    xdbg("[xrec_registration_contract::stakeDeposit] pid: %d, balance: %lld, account: %s, stake deposit: %llu\n", getpid(), GET_BALANCE(), account.c_str(), asset_out.m_amount);
    XCONTRACT_ENSURE(asset_out.m_amount != 0, "xrec_registration_contract::stakeDeposit: stake deposit can not be zero");

    node_info.m_account_mortgage += asset_out.m_amount;
    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "stakeDeposit_Executed", 1);
}

void xrec_registration_contract::unstakeDeposit(uint64_t unstake_deposit) {
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "unstakeDeposit_Called", 1);
    XMETRICS_TIME_RECORD(XREG_CONTRACT "unstakeDeposit_ExecutionTime");
    std::string const& account = SOURCE_ADDRESS();
    xreg_node_info node_info;
    auto ret = get_node_info(account, node_info);
    XCONTRACT_ENSURE(ret == 0, "xrec_registration_contract::unstakeDeposit: node not exist");

    xdbg("[xrec_registration_contract::unstakeDeposit] pid: %d, balance: %lld, account: %s, unstake deposit: %llu, account morgage: %llu\n",
        getpid(), GET_BALANCE(), account.c_str(), unstake_deposit, node_info.m_account_mortgage);
    XCONTRACT_ENSURE(unstake_deposit != 0, "xrec_registration_contract::unstakeDeposit: unstake deposit can not be zero");

    uint64_t min_deposit = node_info.get_required_min_deposit();
    XCONTRACT_ENSURE(unstake_deposit <= node_info.m_account_mortgage && node_info.m_account_mortgage - unstake_deposit >= min_deposit, "xrec_registration_contract::unstakeDeposit: unstake deposit too big");
    TRANSFER(account, unstake_deposit);

    node_info.m_account_mortgage -= unstake_deposit;
    update_node_info(node_info);
    XMETRICS_COUNTER_INCREMENT(XREG_CONTRACT "unstakeDeposit_Executed", 1);
}

int32_t xrec_registration_contract::get_slash_info(std::string const & account, xslash_info & node_slash_info) {
    std::string value_str;
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPROPERTY_CONTRACT_SLASH_INFO_KEY_GetExecutionTime");
        MAP_GET2(XPROPERTY_CONTRACT_SLASH_INFO_KEY, account, value_str);
    }
    if (value_str.empty()) {
        xdbg("[xrec_registration_contract][get_slash_info] account(%s) not exist,  pid:%d\n", account.c_str(), getpid());
        return xaccount_property_not_exist;
    }

    xstream_t stream(xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
    node_slash_info.serialize_from(stream);

    return 0;
}

void xrec_registration_contract::slash_staking_time(std::string const & node_addr) {
    auto current_time = TIME();
    xslash_info s_info;
    get_slash_info(node_addr, s_info);
    if (0 != s_info.m_punish_time) {  // already has slahs info
        auto minus_time = current_time - s_info.m_punish_time;
        if (minus_time > s_info.m_staking_lock_time) {
            s_info.m_staking_lock_time = 0;
        } else {
            s_info.m_staking_lock_time -= minus_time;
        }
    }

    auto lock_time_inc = XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_nodedeposit_lock_duration_increment);
    auto lock_time_max = XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_max_nodedeposit_lock_duration);
    s_info.m_punish_time = current_time;
    s_info.m_staking_lock_time += lock_time_inc;
    if (s_info.m_staking_lock_time > lock_time_max) {
        s_info.m_staking_lock_time = lock_time_max;
        s_info.m_punish_staking++;
    }

    xstream_t stream(xcontext_t::instance());
    s_info.serialize_to(stream);
    {
        XMETRICS_TIME_RECORD(XREG_CONTRACT "XPROPERTY_CONTRACT_SLASH_INFO_KEY_SetExecutionTime");
        MAP_SET(XPROPERTY_CONTRACT_SLASH_INFO_KEY, node_addr, std::string((char *)stream.data(), stream.size()));
    }
}

void xrec_registration_contract::slash_unqualified_node(std::string const & punish_node_str) {
    XMETRICS_TIME_RECORD(XREG_CONTRACT "slash_unqualified_node_ExecutionTime");
    auto const & account = SELF_ADDRESS();
    auto const & source_addr = SOURCE_ADDRESS();

    std::string base_addr = "";
    uint32_t table_id = 0;
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xdbg("[xrec_registration_contract][slash_unqualified_node] self_account %s, source_addr %s, base_addr %s\n", account.c_str(), source_addr.c_str(), base_addr.c_str());

    XCONTRACT_ENSURE(source_addr == top::sys_contract_zec_slash_info_addr, "invalid source addr's call!");

    std::vector<xaction_node_info_t> node_slash_info;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)punish_node_str.data(), punish_node_str.size());
    VECTOR_OBJECT_DESERIALZE2(stream, node_slash_info);
    xinfo("[xrec_registration_contract][slash_unqualified_node] do slash unqualified node, size: %zu", node_slash_info.size());

    for (auto const & value : node_slash_info) {
        std::string const & addr = value.node_id.value();
        xdbg("[xrec_registration_contract][slash_unqualified_node] do slash unqualified node, addr: %s", addr.c_str());
        xreg_node_info reg_node;
        auto ret = get_node_info(addr, reg_node);
        if (ret != 0) {
            xwarn("[xrec_registration_contract][slash_staking_time] get reg node_info error, account: %s", addr.c_str());
            continue;
        }

        if (value.action_type) {  // punish
            xkinfo("[xrec_registration_contract][slash_unqualified_node] effective slash credit & staking time, node addr: %s", addr.c_str());
            XMETRICS_PACKET_INFO("sysContract_zecSlash", "effective slash credit & staking time, node addr", addr);
            reg_node.slash_credit_score(value.node_type);
            slash_staking_time(addr);
        } else {
            xkinfo("[xrec_registration_contract][slash_unqualified_node] effective award credit, node addr: %s", addr.c_str());
            XMETRICS_PACKET_INFO("sysContract_zecSlash", "effective award credit, node addr", addr)
            reg_node.award_credit_score(value.node_type);
        }

        update_node_info(reg_node);
    }
}

bool xrec_registration_contract::is_valid_name(const std::string & nickname) const {
    int len = nickname.length();
    if (len < 4 || len > 16) {
        return false;
    }

    return std::find_if(nickname.begin(), nickname.end(), [](unsigned char c) { return !std::isalnum(c) && c != '_'; }) == nickname.end();
};

void  xrec_registration_contract::init_node_credit(xreg_node_info& node_info, bool isforked) {
    if (node_info.could_be_validator()) {
        if (node_info.m_validator_credit_numerator == 0 ) {
            if (isforked) {
                node_info.m_validator_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_creditscore);
            } else {
                node_info.m_validator_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_creditscore);
            }
        }
    }
    if (node_info.could_be_auditor()) {
        if (node_info.m_auditor_credit_numerator == 0) {
            if (isforked) {
                node_info.m_auditor_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_creditscore);
            } else {
                node_info.m_auditor_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_creditscore);
            }
        }
    }

}

NS_END2

#undef XREG_CONTRACT
#undef XCONTRACT_PREFIX
#undef XZEC_MODULE
