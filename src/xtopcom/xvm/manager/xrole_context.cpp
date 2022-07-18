// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/manager/xrole_context.h"

#include "xchain_fork/xchain_upgrade_center.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtx_factory.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xevent_timer.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/manager/xmessage_ids.h"
#include "xvm/xvm_service.h"

#include <cinttypes>
#include <cmath>

NS_BEG2(top, contract)
using base::xstring_utl;
const uint16_t EXPIRE_DURATION = 300;
xrole_context_t::xrole_context_t(const observer_ptr<xstore_face_t> & store,
                                 const observer_ptr<store::xsyncvstore_t> & syncstore,
                                 const std::shared_ptr<xtxpool_service_v2::xrequest_tx_receiver_face> & unit_service,
                                 const std::shared_ptr<xvnetwork_driver_face_t> & driver,
                                 xcontract_info_t * info)
  : m_store(store), m_syncstore(syncstore), m_unit_service(unit_service), m_driver(driver), m_contract_info(info) {
    XMETRICS_COUNTER_INCREMENT("xvm_contract_role_context_counter", 1);
  }

xrole_context_t::~xrole_context_t() {
    XMETRICS_COUNTER_INCREMENT("xvm_contract_role_context_counter", -1);
    delete m_contract_info;
}

void xrole_context_t::on_block_to_db(const xblock_ptr_t & block, bool & event_broadcasted) {
    if (!m_contract_info->has_monitors()) {
        return;
    }

    // process block event
    if (m_contract_info->has_block_monitors()) {
        auto block_owner = block->get_block_owner();
        // table fulltable block process
        bool is_sharding_statistic =
            (m_contract_info->address == sharding_statistic_info_contract_address) && (block_owner.find(sys_contract_sharding_table_block_addr) != std::string::npos);
        bool is_eth_statistic = (m_contract_info->address == eth_statistic_info_contract_address) && (block_owner.find(sys_contract_eth_table_block_addr) != std::string::npos);

        if ((is_sharding_statistic || is_eth_statistic) && block->is_fulltable()) {
            auto block_height = block->get_height();
            xdbg("xrole_context_t::on_block_to_db fullblock process, owner: %s, height: %" PRIu64, block->get_block_owner().c_str(), block_height);
            base::xauto_ptr<base::xvblock_t> full_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(base::xvaccount_t{block_owner}, block_height, base::enum_xvblock_flag_committed, true);

            auto const * full_tableblock = dynamic_cast<xfull_tableblock_t*>(full_block.get());
            auto node_service = contract::xcontract_manager_t::instance().get_node_service();
            auto const fulltable_statisitc_data = full_tableblock->get_table_statistics();
            auto const statistic_accounts = fulltableblock_statistic_accounts(fulltable_statisitc_data, node_service);

            base::xstream_t stream(base::xcontext_t::instance());
            stream << fulltable_statisitc_data;
            stream << statistic_accounts;
            stream << block_height;
            stream << block->get_pledge_balance_change_tgas();
            std::string action_params = std::string((char *)stream.data(), stream.size());

            xblock_monitor_info_t * info = m_contract_info->find(m_contract_info->address);
            uint32_t table_id = 0;
            auto result = xdatautil::extract_table_id_from_address(block_owner, table_id);
            assert(result);
            XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_table_fullblock_event, 1);
            on_fulltableblock_event(m_contract_info->address, "on_collect_statistic_info", action_params, block->get_timestamp(), (uint16_t)table_id);
        }
    }

    // process broadcasts
    if (m_contract_info->has_broadcasts()) {
        if (event_broadcasted) {  // only select one node to broadcast
            return;
        }

        xassert(block->get_block_level() == base::enum_xvblock_level_table);

        auto address = common::xaccount_address_t{block->get_block_owner()};
        if (common::xaccount_address_t{data::account_address_to_block_address(m_contract_info->address)} == address) {
            xdbg("xrole_context_t::on_block_to_db block=%s", block->dump().c_str());

            // check leader
            common::xip_t validator_xip{block->get_cert()->get_validator().low_addr};  // TODO (payton)
            if (validator_xip.slot_id() == m_driver->address().slot_id()) {
                event_broadcasted = true;

                // load full block input and output
                base::xvaccount_t _vaccount(block->get_account());
                if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, block.get())
                    || false == base::xvchain_t::instance().get_xblockstore()->load_block_output(_vaccount, block.get())) {
                    xerror("xrole_context_t::on_block_to_db fail-load block input output, block=%s", block->dump().c_str());
                    return;
                }

                switch (m_contract_info->broadcast_policy) {
                case enum_broadcast_policy_t::normal:
                    xinfo("xrole_context_t::on_block::broadcast, normal, block=%s", block->dump().c_str());
                    // broadcast(((xevent_store_block_to_db_t *)e.get())->block, m_contract_info->broadcast_types);
                    broadcast(block, m_contract_info->broadcast_types);
                    break;
                case enum_broadcast_policy_t::fullunit:
                    xinfo("xrole_context_t::on_block::broadcast, fullunit, block=%s", block->dump().c_str());
                    if (block->is_fullunit()) {
                        assert(false);
                        // broadcast(((xevent_store_block_to_db_t *)e.get())->block, m_contract_info->broadcast_types);
                        broadcast(block, m_contract_info->broadcast_types);
                    }
                    break;
                default:
                    // no broadcast
                    break;
                }
            }
        }
    }
}

void xrole_context_t::on_block_timer(const xevent_ptr_t & e) {
    if (!m_contract_info->has_monitors()) {
        return;
    }
    if (m_contract_info->has_block_monitors()) {
        auto event = (const mbus::xevent_chain_timer_ptr_t&) e;
        event->time_block->add_ref();
        xblock_ptr_t block{};
        block.attach((xblock_t*) event->time_block);
        xdbg("[xrole_context_t::on_block_timer] %s, %" PRIu64, block->get_account().c_str(), block->get_height());

        // on_relay_election_data_update(block->get_height(),block->get_timestamp());

        auto address = common::xaccount_address_t{block->get_account()};

        xblock_monitor_info_t * info = m_contract_info->find(address);
        if (info == nullptr) {
            for (auto & pair : m_contract_info->monitor_map) {
                if (xcontract_address_map_t::match(address, pair.first)) {
                    info = pair.second;
                    break;
                }
            }
        }

        if (info != nullptr) {
            bool do_call{false};
            uint64_t block_timestamp{0};
            uint64_t onchain_timer_round{0};
            if (info->type == enum_monitor_type_t::timer) {
                xdbg("==== timer monitor");

                xtimer_block_monitor_info_t * timer_info = dynamic_cast<xtimer_block_monitor_info_t *>(info);
                assert(timer_info);
                auto time_interval = timer_info->get_interval();

                if (address == timer_system_address) {
                    xdbg("[xrole_context_t::on_block] get timer block at %" PRIu64, block->get_height());
                    onchain_timer_round = block->get_height();
                    block_timestamp = block->get_timestamp();


                    if ((m_contract_info->address == sharding_statistic_info_contract_address) && valid_call(onchain_timer_round)) {

                        int table_num = m_driver->table_ids().size();
                        if (table_num == 0) {
                            xwarn("xrole_context_t::on_block_timer: table_ids empty\n");
                            return;
                        }

                        int clock_interval = 1;
                        clock_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);

                        if (m_table_contract_schedule.find(m_contract_info->address) != m_table_contract_schedule.end()) {
                            auto& schedule_info = m_table_contract_schedule[m_contract_info->address];
                            schedule_info.target_interval = clock_interval;
                            schedule_info.cur_interval++;
                            if (schedule_info.cur_interval == schedule_info.target_interval) {
                                schedule_info.cur_table = m_driver->table_ids().at(0) +  static_cast<uint16_t>((onchain_timer_round / clock_interval) % table_num);
                                xinfo("xrole_context_t::on_block_timer: table contract schedule, contract address %s, timer %" PRIu64 ", schedule info:[%hu, %hu, %hu %hu]",
                                    m_contract_info->address.value().c_str(), onchain_timer_round, schedule_info.cur_interval, schedule_info.target_interval, schedule_info.clock_or_table, schedule_info.cur_table);
                                call_contract(onchain_timer_round, info, block_timestamp, schedule_info.cur_table);
                                schedule_info.cur_interval = 0;
                            }
                        } else { // have not schedule yet
                            xtable_schedule_info_t schedule_info(clock_interval, m_driver->table_ids().at(0) + static_cast<uint16_t>((onchain_timer_round / clock_interval) % table_num));
                            xinfo("xrole_context_t::on_block_timer: table contract schedule initial, contract address %s, timer %" PRIu64 ", schedule info:[%hu, %hu, %hu %hu]",
                                    m_contract_info->address.value().c_str(), onchain_timer_round, schedule_info.cur_interval, schedule_info.target_interval, schedule_info.clock_or_table, schedule_info.cur_table);
                            call_contract(onchain_timer_round, info, block_timestamp, schedule_info.cur_table);
                            m_table_contract_schedule[m_contract_info->address] = schedule_info;
                        }

                        return;
                    } else if ((m_contract_info->address == eth_statistic_info_contract_address) && valid_call(onchain_timer_round)) {
                        // default size = 1
                        auto call_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_statistic_report_schedule_interval);
                        auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
                        if (top::chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.eth_fork_point, onchain_timer_round)) {
                            if (call_interval != 0 && onchain_timer_round != 0 && onchain_timer_round % call_interval == 0) {
                                xinfo("xrole_context_t::on_block_timer table contract schedule, contract address %s, timer %" PRIu64 ", interval: %lu",
                                    m_contract_info->address.value().c_str(),
                                    onchain_timer_round,
                                    call_interval);
                                call_contract(onchain_timer_round, info, block_timestamp, 0);
                            }
                        }
                        return;
                    }

                    bool first_blk = runtime_stand_alone(onchain_timer_round, m_contract_info->address);
                    if (time_interval != 0 && onchain_timer_round != 0 && ((first_blk && (onchain_timer_round % 3) == 0) || (!first_blk && (onchain_timer_round % time_interval) == 0))) {
                        do_call = true;
                    }
                    // if (time_interval == 0 ||
                    //     round == 0         ||
                    //     ((round % time_interval) != 0 && !runtime_stand_alone(onchain_timer_round, m_contract_info->address))) {
                    //     do_call = false;
                    // }
                    xdbg("==== get timer2 transaction %s, %llu, %u, %d", m_contract_info->address.value().c_str(), onchain_timer_round, time_interval, do_call);
                } else {
                    assert(0);
                }
            }
            if (do_call && valid_call(onchain_timer_round)) {
                xinfo("xrole_context_t::on_block_timer call_contract : %s , %llu ,%d", m_contract_info->address.value().c_str(), onchain_timer_round, static_cast<int32_t>(info->type));
                call_contract(onchain_timer_round, info, block_timestamp);
            }
        }
    }
}

bool xrole_context_t::runtime_stand_alone(const uint64_t timer_round, common::xaccount_address_t const & sys_addr) const {
    const static std::vector<common::xaccount_address_t> sys_addr_list{rec_elect_edge_contract_address,
                                                                       rec_elect_archive_contract_address,
                                                                       rec_elect_zec_contract_address,
                                                                       zec_elect_consensus_contract_address,
                                                                       rec_elect_fullnode_contract_address,
                                                                       zec_elect_eth_contract_address};

    if (std::find(std::begin(sys_addr_list), std::end(sys_addr_list), sys_addr) == std::end(sys_addr_list)) {
        return false;
    }

    auto account = m_store->query_account(sys_addr.value());
    if (nullptr == account) {
        xerror("xrole_context_t::runtime_stand_alone fail-query account.address=%s", sys_addr.value().c_str());
        xassert(nullptr != account);
        return false;
    }
    return 0 == account->get_chain_height();
}

bool xrole_context_t::valid_call(const uint64_t onchain_timer_round) {
    auto iter = m_address_round_map.find(m_contract_info->address);
    if (iter == m_address_round_map.end() || (iter != m_address_round_map.end() && iter->second < onchain_timer_round)) {
        m_address_round_map[m_contract_info->address] = onchain_timer_round;
        return true;
    } else {
        xinfo("not valid call %llu", onchain_timer_round);
        return false;
    }
}

data::xfulltableblock_statistic_accounts xrole_context_t::fulltableblock_statistic_accounts(data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service) {
    using namespace top::data;

    xfulltableblock_statistic_accounts res;

    // process one full tableblock statistic data
    for (auto const & statistic_item: block_statistic_data.detail) {
        auto elect_statistic = statistic_item.second;
        xfulltableblock_group_data_t res_group_data;
        for (auto const & group_item: elect_statistic.group_statistics_data) {
            xgroup_related_statistics_data_t const& group_account_data = group_item.second;
            common::xgroup_address_t const& group_addr = group_item.first;
            xvip2_t const& group_xvip2 = top::common::xip2_t{
                group_addr.network_id(),
                group_addr.zone_id(),
                group_addr.cluster_id(),
                group_addr.group_id(),
                (uint16_t)group_account_data.account_statistics_data.size(),
                statistic_item.first
            };

            xfulltableblock_account_data_t res_account_data;
            for (std::size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                auto const& account_group = node_service->get_group(group_xvip2);
                // if empty, then just return current data
                if (!account_group) {
                    XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_table_statistic_empty_ptr, 1);
                    xerror("[xfulltableblock_statistic_accounts xrole_context_t::fulltableblock_statistic_accounts] data miss, statistic accounts not the same");
                    return res;
                }
                auto account_addr = account_group->get_node(slotid)->get_account();
                res_account_data.account_data.emplace_back(std::move(account_addr));
            }

            res_group_data.group_data[group_addr] = res_account_data;
        }

        res.accounts_detail[statistic_item.first] = res_group_data;
    }

    return res;
}

void xrole_context_t::call_contract(const uint64_t onchain_timer_round, xblock_monitor_info_t * info, const uint64_t block_timestamp) {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << onchain_timer_round;
    std::string action_params = std::string((char *)stream.data(), stream.size());

    call_contract(action_params, block_timestamp, info);
}

void xrole_context_t:: call_contract(const uint64_t onchain_timer_round, xblock_monitor_info_t * info, const uint64_t block_timestamp, uint16_t table_id) {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << onchain_timer_round;
    std::string action_params = std::string((char *)stream.data(), stream.size());

    call_contract(action_params, block_timestamp, info, table_id);
}

bool xrole_context_t::is_timer_unorder(common::xaccount_address_t const & address, uint64_t timestamp) {
    if (address == timer_system_address) {
        auto block = m_syncstore->get_vblockstore()->get_latest_committed_block(address.value());
        if (abs((int64_t)(((xblock_t *)block.get())->get_timestamp() - timestamp)) <= 3) {
            return true;
        }
    }
    return false;
}

void xrole_context_t::call_contract(const std::string & action_params, uint64_t timestamp, xblock_monitor_info_t * info) {
    std::vector<common::xaccount_address_t> addresses;
    if (is_sys_sharding_contract_address(m_contract_info->address)) {
        for (auto & tid : m_driver->table_ids()) {
            addresses.push_back(xcontract_address_map_t::calc_cluster_address(m_contract_info->address, tid));
        }
    } else {
        addresses.push_back(m_contract_info->address);
    }
    xproperty_asset asset_out{0};
    for (auto & address : addresses) {
        if (is_timer_unorder(address, timestamp)) {
            xinfo("[xrole_context_t] call_contract in consensus mode, address timer unorder, not create tx", address.value().c_str());
            continue;
        }

        xaccount_ptr_t account = m_store->query_account(address.value());
        if (nullptr == account) {
            xerror("xrole_context_t::call_contract fail-query account.address=%s", address.value().c_str());
            xassert(nullptr != account);
            return;
        }
        xtransaction_ptr_t tx = xtx_factory::create_sys_contract_call_self_tx(address.value(),
                                                         account->account_send_trans_number(), account->account_send_trans_hash(),
                                                         info->action, action_params, timestamp, EXPIRE_DURATION);

        if (info->call_way == enum_call_action_way_t::consensus) {
            int32_t r = m_unit_service->request_transaction_consensus(tx, true);
            xinfo("[xrole_context_t] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
                  r,
                  tx->get_digest_hex_str().c_str(),
                  address.value().c_str(),
                  data::to_hex_str(account->account_send_trans_hash()).c_str(),
                  account->account_send_trans_number(),
                  timestamp);
        } else {
            // TODO(jimmy) now support
            xassert(false);
            // xvm::xvm_service s;
            // xaccount_context_t ac(address.value(), m_store.get());
            // auto trace = s.deal_transaction(tx, &ac);
            // xinfo("[xrole_context_t] call_contract in no_consensus mode with return code : %d", (int)trace->m_errno);
        }
    }
}

void xrole_context_t::call_contract(const std::string & action_params, uint64_t timestamp, xblock_monitor_info_t * info, uint16_t table_id) {
    auto const address = xcontract_address_map_t::calc_cluster_address(m_contract_info->address, table_id);

    if (is_timer_unorder(address, timestamp)) {
        xinfo("[xrole_context_t] call_contract in consensus mode, address timer unorder, not create tx", address.c_str());
        return;
    }

    xaccount_ptr_t account = m_store->query_account(address.value());
    if (nullptr == account) {
        xerror("xrole_context_t::call_contract fail-query account.address=%s", address.value().c_str());
        xassert(nullptr != account);
        return;
    }
    xtransaction_ptr_t tx = xtx_factory::create_sys_contract_call_self_tx(address.value(),
                                                     account->account_send_trans_number(), account->account_send_trans_hash(),
                                                     info->action, action_params, timestamp, EXPIRE_DURATION);

    if (info->call_way == enum_call_action_way_t::consensus) {
        int32_t r = m_unit_service->request_transaction_consensus(tx, true);
        xinfo("[xrole_context_t] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
              r,
              tx->get_digest_hex_str().c_str(),
              address.c_str(),
              data::to_hex_str(account->account_send_trans_hash()).c_str(),
              account->account_send_trans_number(),
              timestamp);
    } else {
        // TODO(jimmy) now support
        xassert(false);
        // xvm::xvm_service s;
        // xaccount_context_t ac(address.value(), m_store.get());
        // auto trace = s.deal_transaction(tx, &ac);
        // xinfo("[xrole_context_t] call_contract in no_consensus mode with return code : %d", (int)trace->m_errno);
    }
}

void xrole_context_t::on_fulltableblock_event(common::xaccount_address_t const& contract_name, std::string const& action_name, std::string const& action_params, uint64_t timestamp, uint16_t table_id) {
    auto const address = xcontract_address_map_t::calc_cluster_address(contract_name, table_id);
    xaccount_ptr_t account = m_store->query_account(address.value());
    if (nullptr == account) {
        xerror("xrole_context_t::on_fulltableblock_event fail-query account.address=%s", address.c_str());
        xassert(nullptr != account);
        return;
    }
    xtransaction_ptr_t tx = xtx_factory::create_sys_contract_call_self_tx(
        address.value(), account->account_send_trans_number(), account->account_send_trans_hash(), action_name, action_params, timestamp, EXPIRE_DURATION);

    auto const & driver_ids = m_driver->table_ids();
    auto result = find(driver_ids.begin(), driver_ids.end(), table_id);
    if (result == driver_ids.end( )) {
        return;
    }
    int32_t r = m_unit_service->request_transaction_consensus(tx, true);
    xinfo("[xrole_context_t::fulltableblock_event] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
            r,
            tx->get_digest_hex_str().c_str(),
            address.c_str(),
            data::to_hex_str(account->account_send_trans_hash()).c_str(),
            account->account_send_trans_number(),
            timestamp);
}

// void xrole_context_t::on_relay_election_data_update(uint64_t block_height, uint64_t block_timestamp){
//     auto const relay_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(relay_election_interval);
//     if (block_height == 0 || (block_height % (relay_election_interval / 4) != 0)){
//         return;
//     }

//     if (m_contract_info->address != relay_repackage_election_data_contract_address) {
//         return;
//     }

//     // re-package the relay shard election data into the relay chain:
//     //  1. get the relay shard election data
//     //  2. read the re-package contract data
//     //  3. compare the read relay shard election data epoch and the contract data epoch
//     //  4. epoch of relay election data is newer, generate transaction calling the re-package contract

//     xobject_ptr_t<xvblock_t> zec_elect_relay_blk =
//         xblocktool_t::get_latest_connectted_state_changed_block(xvchain_t::instance().get_xblockstore(), zec_elect_relay_contract_address.vaccount());
//     // zec_elect_relay_blk->get_parent_block_height()
//     xobject_ptr_t<xvblock_t> package_relay_election_data_blk =
//         xblocktool_t::get_latest_connectted_state_changed_block(xvchain_t::instance().get_xblockstore(), relay_repackage_election_data_contract_address.vaccount());

//     xobject_ptr_t<xvbstate_t> const zec_elect_relay_state =
//         xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(zec_elect_relay_blk.get(), metrics::statestore_access_from_contract_framework);
//     xobject_ptr_t<xvbstate_t> const package_relay_election_data_state =
//         xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(package_relay_election_data_blk.get(), metrics::statestore_access_from_contract_framework);

//     if (nullptr == zec_elect_relay_state || nullptr == package_relay_election_data_state) {
//         xwarn("role_context fail to load relay election contract state");
//         return;
//     }

//     xunit_bstate_t const zec_elect_relay_unit_state{zec_elect_relay_state.get()};
//     xunit_bstate_t const relay_elect_relay_unit_state{package_relay_election_data_state.get()};

//     auto const & property_names = election::get_property_name_by_addr(zec_elect_relay_contract_address);
//     if (property_names.size() != 1) {
//         xerror("role_context: zec_elect_relay_contract property count unexpected");
//         return;
//     }

//     auto const & property_name = property_names.front();
//     std::string result;
//     zec_elect_relay_unit_state.string_get(property_name, result);
//     if (result.empty()) {
//         xerror("role_context: relay election data empty. property=%s,block=%s", property_name.c_str(), zec_elect_relay_blk->dump().c_str());
//         return;
//     }

//     xdbg("role_context: process relay election data %s, %" PRIu64 " from block: %" PRIu64 " %" PRIu64,
//          zec_elect_relay_contract_address.c_str(),
//          zec_elect_relay_blk->get_height(),
//          block_height,
//          block_timestamp);

//     using top::data::election::xelection_result_store_t;

//     std::string current_result;
//     relay_elect_relay_unit_state.string_get(property_name, current_result);
//     if (current_result.empty()) {
//         // even genesis is not empty.
//         xerror("role_context: local packaged relay election data empty. property=%s,block=%s", property_name.c_str(), package_relay_election_data_blk->dump().c_str());
//         return;
//     }
//     auto const & current_election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(current_result), std::end(current_result)});

//     auto const & coming_election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});

//     auto const relay_group_address = common::build_relay_group_address(common::network_id());
//     try {
//         auto const & coming_group_result = coming_election_result_store.result_of(relay_group_address.network_id())
//                                                .result_of(common::xnode_type_t::relay)
//                                                .result_of(relay_group_address.cluster_id())
//                                                .result_of(relay_group_address.group_id());

//         auto const & current_group_result = current_election_result_store.result_of(relay_group_address.network_id())
//                                                 .result_of(common::xnode_type_t::relay)
//                                                 .result_of(relay_group_address.cluster_id())
//                                                 .result_of(relay_group_address.group_id());

//         if (coming_group_result.group_epoch() <= current_group_result.group_epoch()) {
//             xwarn("coming epoch is older, no need to update, %" PRIu64 " <= % " PRIu64, coming_group_result.group_epoch().value(), current_group_result.group_epoch().value());
//             return;
//         }

//         // create tx to call contract xtop_relay_process_election_data_contract on_recv_election_data
//         xaccount_ptr_t account = m_store->query_account(relay_repackage_election_data_contract_address.value());
//         if (nullptr == account) {
//             xerror("xrole_context_t relay election data fail-query account.address=%s", relay_repackage_election_data_contract_address.c_str());
//             xassert(nullptr != account);
//             return;
//         }

//         base::xstream_t stream(base::xcontext_t::instance());
//         stream << top::to_bytes(result);
//         std::string action_params = std::string((char *)stream.data(), stream.size());

//         xtransaction_ptr_t tx = xtx_factory::create_sys_contract_call_self_tx(relay_repackage_election_data_contract_address.value(),
//                                                                               account->account_send_trans_number(),
//                                                                               account->account_send_trans_hash(),
//                                                                               "on_recv_election_data",
//                                                                               action_params,
//                                                                               block_timestamp,
//                                                                               EXPIRE_DURATION);

//         int32_t r = m_unit_service->request_transaction_consensus(tx, true);
//         xinfo("xrole_context_t relay election data call_contract in consensus mode with return code : %d, %s, %s %s %" PRIu64 " %" PRIu64,
//               r,
//               tx->get_digest_hex_str().c_str(),
//               relay_repackage_election_data_contract_address.c_str(),
//               data::to_hex_str(account->account_send_trans_hash()).c_str(),
//               account->account_send_trans_number(),
//               block_timestamp);

//     } catch (std::exception const & eh) {
//         xwarn("fail to get zec elect relay election data. exception msg: %s", eh.what());
//         return;
//     }
// }

// todo(nathan):relay make contract already do this

void xrole_context_t::broadcast(const xblock_ptr_t & block_ptr, common::xnode_type_t types) {
    assert(block_ptr != nullptr);
    base::xstream_t stream(base::xcontext_t::instance());
    block_ptr->full_block_serialize_to(stream);
    auto message = xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_block_broadcast_id);

    if (common::has<common::xnode_type_t::real_part_mask>(types)) {
        common::xnode_address_t dest{common::xcluster_address_t{m_driver->network_id()}};
        std::error_code ec;
        m_driver->broadcast(common::xip2_t{m_driver->network_id()}, message, ec);
        if (ec) {
            xerror("[xrole_context_t] broadcast to ALL failed. block owner %s height %" PRIu64, block_ptr->get_block_owner().c_str(), block_ptr->get_height());
        } else {
            xdbg("[xrole_context_t] broadcast to ALL. block owner %s height %" PRIu64, block_ptr->get_block_owner().c_str(), block_ptr->get_height());
        }
    } else {
        if (common::has<common::xnode_type_t::committee>(types)) {
            common::xnode_address_t dest{common::build_committee_sharding_address(m_driver->network_id())};
            //m_driver->forward_broadcast_message(message, dest);
            std::error_code ec;
            m_driver->broadcast(dest.xip2(), message, ec);
            if (ec) {
                xerror("[xrole_context_t] broadcast to rec failed. block owner %s", block_ptr->get_block_owner().c_str());
            } else {
                xdbg("[xrole_context_t] broadcast to beacon. block owner %s", block_ptr->get_block_owner().c_str());
            }
        }

        if (common::has<common::xnode_type_t::zec>(types)) {
            common::xnode_address_t dest{common::build_zec_sharding_address(m_driver->network_id())};
            //if (m_driver->address().cluster_address() == dest.cluster_address()) {
            //    m_driver->broadcast(message);
            //} else {
            //    m_driver->forward_broadcast_message(message, dest);
            //}
            std::error_code ec;
            m_driver->broadcast(dest.xip2(), message, ec);
            if (ec) {
                xerror("[xrole_context_t] broadcast to zec failed. block owner %s", block_ptr->get_block_owner().c_str());
            } else {
                xdbg("[xrole_context_t] broadcast to zec. block owner %s", block_ptr->get_block_owner().c_str());
            }
        }

        if (common::has<common::xnode_type_t::storage>(types)) {
            {
                common::xnode_address_t archive_dest{common::build_archive_sharding_address(common::xarchive_group_id, m_driver->network_id())};
                std::error_code ec;
                m_driver->broadcast(archive_dest.xip2(), message, ec);
                if (ec) {
                    xerror("[xrole_context_t] broadcast to archive failed. block owner %s", block_ptr->get_block_owner().c_str());
                } else {
                    xdbg("[xrole_context_t] broadcast to archive. block owner %s", block_ptr->get_block_owner().c_str());
                }
            }
            {
                // todo(next version fork)
                // if (forked standalone_exchange_point) {
                    common::xnode_address_t exchange_dest{common::build_exchange_sharding_address(m_driver->network_id())};
                // } else {
                    // common::xnode_address_t exchange_dest{common::build_legacy_exchange_sharding_address(common::xlegacy_exchange_group_id, m_driver->network_id())};
                // }
                std::error_code ec;
                m_driver->broadcast(exchange_dest.xip2(), message, ec);
                if (ec) {
                    xerror("[xrole_context_t] broadcast to exchange failed. block owner %s", block_ptr->get_block_owner().c_str());
                } else {
                    xdbg("[xrole_context_t] broadcast to exchange. block owner %s", block_ptr->get_block_owner().c_str());
                }
            }
            //xdbg("[xrole_context_t] broadcast to archive. block owner %s", block_ptr->get_block_owner().c_str());
        }
    }
}

NS_END2
