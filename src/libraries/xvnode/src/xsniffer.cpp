// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xcomponents/xblock_sniffing/xsniffer.h"

#include "xbasic/xutility.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtx_factory.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvnode/xcomponents/xblock_process/xfulltableblock_process.h"
#include "xvnode/xcomponents/xblock_sniffing/xsniffer_action.h"
#include "xstatestore/xstatestore_face.h"
#include <cinttypes>

NS_BEG4(top, vnode, components, sniffing)

xtop_sniffer::xtop_sniffer(observer_ptr<base::xvnodesrv_t> const & nodesrv,
                           observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & manager,
                           observer_ptr<xvnode_face_t> const & vnode)
  : m_nodesvr(nodesrv), m_system_contract_manager(manager), m_vnode(vnode) {
    sniff_set();
}

void xtop_sniffer::sniff_set() {
    common::xnode_type_t type{m_vnode->vnetwork_driver()->type()};
    bool disable_broadcasts{false};
    xdbg("[xtop_vnode::get_roles_data] node type : %s", common::to_string(type).c_str());

    if (common::has<common::xnode_type_t::consensus_auditor>(type)) {
        xdbg("[xtop_vnode::get_roles_data] add all sharding contracts' rcs");
        type = common::xnode_type_t::consensus_validator;
        disable_broadcasts = true;
    }

    auto const & system_contract_deployment_data = m_system_contract_manager->deployment_data();
    for (auto const & contract_data_pair : system_contract_deployment_data) {
        auto const & contract_address = top::get<common::xaccount_address_t const>(contract_data_pair);
        auto const & contract_data = top::get<contract_runtime::system::xcontract_deployment_data_t>(contract_data_pair);

        if (!common::has(type, contract_data.node_type)) {
            continue;
        }
        if (!m_config_map.count(contract_address)) {
            m_config_map.insert(std::make_pair(contract_address, xrole_config_t{contract_data, std::map<common::xaccount_address_t, uint64_t>()}));
        }
        if (disable_broadcasts) {
            m_config_map[contract_address].role_data.broadcast_config.zone = contract_runtime::xsniff_broadcast_type_t::invalid;
        }
    }
}

xsniffer_config_t xtop_sniffer::sniff_config() const {
    xsniffer_config_t config;
    for (auto const & data_pair : m_config_map) {
        auto const & data = top::get<xrole_config_t>(data_pair);
        auto const & sniff_type = data.role_data.sniff_type;
        if (contract_runtime::has<contract_runtime::xsniff_type_t::broadcast>(sniff_type)) {
            auto sniff_block_type = data.role_data.broadcast_config.type;
            assert(sniff_block_type != contract_runtime::xsniff_block_type_t::invalid);
            config.insert(std::make_pair(xsniffer_event_type_t::broadcast, xsniffer_event_config_t{sniff_block_type, std::bind(&xtop_sniffer::sniff_broadcast, this, std::placeholders::_1)}));
        }
        if (contract_runtime::has<contract_runtime::xsniff_type_t::timer>(sniff_type)) {
            config.insert(std::make_pair(
                xsniffer_event_type_t::timer,
                xsniffer_event_config_t{xsniffer_block_type_t::all_block, std::bind(&xtop_sniffer::sniff_timer, this, std::placeholders::_1)}));
        }
        if (contract_runtime::has<contract_runtime::xsniff_type_t::block>(sniff_type)) {
            auto sniff_block_type = data.role_data.block_config.type;
            assert(sniff_block_type != contract_runtime::xsniff_block_type_t::invalid);
            config.insert(std::make_pair(xsniffer_event_type_t::block, xsniffer_event_config_t{sniff_block_type, std::bind(&xtop_sniffer::sniff_block, this, std::placeholders::_1)}));
        }
    }

    return config;
}

bool xtop_sniffer::sniff_broadcast(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    xassert(vblock->get_block_level() == base::enum_xvblock_level_table);
    auto const & block_address = vblock->get_account();
    auto const height = vblock->get_height();
    for (auto const & role_data_pair : m_config_map) {
        auto const & contract_address = top::get<common::xaccount_address_t const>(role_data_pair);
        auto const & config = top::get<xrole_config_t>(role_data_pair).role_data;
        if (!contract_runtime::has<contract_runtime::xsniff_type_t::broadcast>(config.sniff_type)) {
            continue;
        }
        if (data::account_address_to_block_address(contract_address) != block_address) {
            continue;
        }
        common::xip_t validator_xip{vblock->get_cert()->get_validator().low_addr};
        if (validator_xip.slot_id() == m_vnode->vnetwork_driver()->address().slot_id()) {
            // load full block input and output
            base::xvaccount_t _vaccount(block_address);
            if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, vblock.get()) ||
                false == base::xvchain_t::instance().get_xblockstore()->load_block_output(_vaccount, vblock.get())) {
                xerror("[xtop_vnode::sniff_broadcast] fail-load block input output, block=%s", vblock->dump().c_str());
                return false;
            }
            vblock->add_ref();
            data::xblock_ptr_t block{};
            block.attach(static_cast<data::xblock_t *>(vblock.get()));
            xsniffer_action_t::broadcast(m_vnode, block, static_cast<common::xnode_type_t>(config.broadcast_config.zone));
            xinfo("[xtop_vnode::sniff_broadcast] contract: %s, block: %s, height: %llu, broadcast success, block=%s!",
                  contract_address.to_string().c_str(),
                  block_address.c_str(),
                  height,
                  vblock->dump().c_str());
            return true;
        }
    }

    return false;
}

bool xtop_sniffer::sniff_timer(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    assert(common::xaccount_address_t{vblock->get_account()} == common::xaccount_address_t{sys_contract_beacon_timer_addr});
    auto const logic_time = vblock->get_height();
    auto const timestamp = vblock->get_cert()->get_gmtime();

    for (auto & role_data_pair : m_config_map) {
        auto const & contract_address = top::get<common::xaccount_address_t const>(role_data_pair);
        auto const & config = top::get<xrole_config_t>(role_data_pair).role_data;

        if (!contract_runtime::has<contract_runtime::xsniff_type_t::timer>(config.sniff_type)) {
            xdbg("xtop_sniffer::sniff_timer: logic time: %" PRIu64 " sees %s but contract not sniff timer", logic_time, contract_address.to_string().c_str());
            continue;
        }

        xdbg("xtop_sniffer::sniff_timer: logic time: %" PRIu64 " sees %s", logic_time, contract_address.to_string().c_str());
        auto const valid = is_valid_timer_call(contract_address, top::get<xrole_config_t>(role_data_pair), logic_time);
        // xdbg("xtop_sniffer::sniff_timer: contract address %s, interval: %u, valid: %d", contract_address.c_str(), config.timer_config.timer_config_data.get_timer_interval(), valid);
        if (!valid) {
            continue;
        }

        base::xstream_t stream(base::xcontext_t::instance());
        stream << logic_time;
        std::string action_params = std::string(reinterpret_cast<char *>(stream.data()), stream.size());
        xdbg("[xtop_sniffer::sniff_timer] make tx, action: %s, params size: %zu", config.timer_config.action.c_str(), action_params.size());

        switch (config.timer_config.strategy) {
            case top::contract_runtime::xtimer_strategy_type_t::normal:
                normal_timer_func(contract_address, config.timer_config, action_params, timestamp);
                break;


            case top::contract_runtime::xtimer_strategy_type_t::table:
                table_timer_func(contract_address, config.timer_config, action_params, timestamp, logic_time);
                break;

            default:
                assert(false); // invalid
                break;
        }

    }
    return true;
}

bool xtop_sniffer::sniff_block(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    auto const & block_address = vblock->get_account();
    auto const height = vblock->get_height();
    xdbg("[xtop_vnode::sniff_block begin]  block: %s, height: %llu",  block_address.c_str(), height);

    for (auto & role_data_pair : m_config_map) {
        auto const & contract_address = role_data_pair.first;
        auto const & config = role_data_pair.second.role_data;
        if (!contract_runtime::has<contract_runtime::xsniff_type_t::block>(config.sniff_type)) {
            continue;
        }
        xdbg("[xtop_vnode::sniff_block] contract: %s, block: %s, height: %llu", contract_address.to_string().c_str(), block_address.c_str(), height);

        // table upload contract sniff sharding table addr
        if ((block_address.find(config.block_config.sniff_address.to_string()) != std::string::npos) && (contract_address == config.block_config.action_address)) {
            xdbg("[xtop_vnode::sniff_block] sniff block match, contract: %s, block: %s, height: %llu", contract_address.to_string().c_str(), block_address.c_str(), height);
            auto const full_tableblock = (dynamic_cast<data::xfull_tableblock_t *>(vblock.get()));
            auto const fulltable_statisitc_data = full_tableblock->get_table_statistics();
            auto const statistic_accounts = components::xfulltableblock_process_t::fulltableblock_statistic_accounts(fulltable_statisitc_data, m_nodesvr.get());


            base::xstream_t stream(base::xcontext_t::instance());
            stream << fulltable_statisitc_data;
            stream << statistic_accounts;
            stream << height;
            stream << full_tableblock->get_pledge_balance_change_tgas();
            std::string action_params = std::string((char *)stream.data(), stream.size());
            uint32_t table_id = 0;
#if !defined(NDEBUG)
            auto result =
#endif
            data::xdatautil::extract_table_id_from_address(block_address, table_id);
            assert(result);
            {
                // table id check
                auto const & driver_ids = m_vnode->vnetwork_driver()->table_ids();
                auto result = find(driver_ids.begin(), driver_ids.end(), table_id);
                if (result == driver_ids.end()) {
                    return false;
                }
            }
            auto const & table_address = contract::xcontract_address_map_t::calc_cluster_address(contract_address, table_id);

            XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_table_fullblock_event, 1);
            call(table_address, config.block_config.action, action_params, vblock->get_cert()->get_gmtime());
        }
    }
    return true;
}

bool xtop_sniffer::is_valid_timer_call(common::xaccount_address_t const & address, xrole_config_t & data, const uint64_t height) const {
    static std::vector<common::xaccount_address_t> const sys_addr_list{common::xaccount_address_t{sys_contract_rec_elect_edge_addr},
                                                                       common::xaccount_address_t{sys_contract_rec_elect_archive_addr},
                                                                       common::xaccount_address_t{sys_contract_rec_elect_exchange_addr},
                                                                       // common::xaccount_address_t{ sys_contract_zec_elect_edge_addr },
                                                                       // common::xaccount_address_t{ sys_contract_zec_elect_archive_addr },
                                                                       common::xaccount_address_t{sys_contract_rec_elect_zec_addr},
                                                                       common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}};
    bool is_first_block{false};
    if (std::find(std::begin(sys_addr_list), std::end(sys_addr_list), address) != std::end(sys_addr_list)) {
        if (statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(address)->height() == 0) {
            is_first_block = true;
        }
    }

    auto const interval = data.role_data.timer_config.timer_config_data.get_timer_interval();
    assert(interval > 0);
    if (interval != 0 && height != 0 && ((is_first_block && (height % 3) == 0) || (!is_first_block && (height % interval) == 0))) {
        xdbg("[xtop_sniffer::is_valid_timer_call] param check pass, interval: %u, height: %llu, first_block: %d", interval, height, is_first_block);
    } else {
        xdbg("[xtop_sniffer::is_valid_timer_call] param check not pass, interval: %u, height: %llu, first_block: %d", interval, height, is_first_block);
        return false;
    }

    auto & round = data.address_round;
    auto iter = round.find(address);
    if (iter == round.end() || (iter != round.end() && iter->second < height)) {
        round[address] = height;
        return true;
    } else {
        xwarn("[xtop_sniffer::is_valid_timer_call] address %s height check error, last height: %llu, this height : %llu", address.to_string().c_str(), round[address], height);
        return false;
    }
}

bool xtop_sniffer::trigger_first_timer_call(common::xaccount_address_t const & address) const {
    static std::vector<common::xaccount_address_t> const sys_addr_list{common::xaccount_address_t{sys_contract_rec_elect_edge_addr},
                                                                       common::xaccount_address_t{sys_contract_rec_elect_archive_addr},
                                                                       common::xaccount_address_t{sys_contract_rec_elect_exchange_addr},
                                                                       // common::xaccount_address_t{ sys_contract_zec_elect_edge_addr },
                                                                       // common::xaccount_address_t{ sys_contract_zec_elect_archive_addr },
                                                                       common::xaccount_address_t{sys_contract_rec_elect_zec_addr},
                                                                       common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}};

    if (std::find(std::begin(sys_addr_list), std::end(sys_addr_list), address) == std::end(sys_addr_list)) {
        xdbg("xtop_vnode_sniff::trigger_first_timer_call: not monitored address %s", address.to_string().c_str());
        return false;
    }

    auto const account = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(address);
    xdbg("xtop_vnode_sniff::trigger_first_timer_call: monitored address %s height %" PRIu64, address.to_string().c_str(), account->height());
    return 0 == account->height();
}

void xtop_sniffer::table_timer_func(common::xaccount_address_t const& contract_address, top::contract_runtime::xsniff_timer_config_t const& timer_config,
                                            std::string const& action_params, uint64_t timestamp, uint64_t height) const {
    int table_num = m_vnode->vnetwork_driver()->table_ids().size();
    if (table_num == 0) {
        xwarn("[xtop_vnode_sniff::table_timer_func] table_ids empty");
        return;
    }

    int clock_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);

    if (m_table_contract_schedule.find(contract_address) != m_table_contract_schedule.end()) {
        auto& schedule_info = m_table_contract_schedule[contract_address];
        schedule_info.cur_table = m_vnode->vnetwork_driver()->table_ids().at(0) + static_cast<uint16_t>((height / clock_interval) % table_num);
        xinfo("[xtop_vnode_sniff::table_timer_func] table contract schedule, contract address %s, timer %lu, schedule info:[%hu, %hu, %hu %hu]",
              contract_address.to_string().c_str(),
              height,
              schedule_info.cur_interval,
              schedule_info.target_interval,
              schedule_info.clock_or_table,
              schedule_info.cur_table);
        call(contract_address, timer_config.action, action_params, timestamp, schedule_info.cur_table);

    } else { // have not schedule yet
        xtable_schedule_info_t schedule_info(clock_interval, m_vnode->vnetwork_driver()->table_ids().at(0) + static_cast<uint16_t>((height / clock_interval) % table_num));
        xinfo("[xtop_vnode_sniff::table_timer_func] table contract schedule initial, contract address %s, timer %lu, schedule info:[%hu, %hu, %hu %hu]",
              contract_address.to_string().c_str(),
              height,
              schedule_info.cur_interval,
              schedule_info.target_interval,
              schedule_info.clock_or_table,
              schedule_info.cur_table);
        call(contract_address, timer_config.action, action_params, timestamp, schedule_info.cur_table);
        m_table_contract_schedule[contract_address] = schedule_info;
    }
}

void xtop_sniffer::normal_timer_func(common::xaccount_address_t const& contract_address, top::contract_runtime::xsniff_timer_config_t const& timer_config,
                                             std::string const& action_params, uint64_t timestamp) const {
    xdbg("xtop_vnode_sniff::normal_timer_func: make tx, action: %s, params: %s", timer_config.action.c_str(), action_params.c_str());
    call(contract_address, timer_config.action, action_params, timestamp);
}

void xtop_sniffer::call(common::xaccount_address_t const & address, std::string const & action_name, std::string const & action_params, const uint64_t timestamp) const {
    //base::xaccount_index_t accountindex;
    //// int32_t ret_pushtx = xsuccess;
    //bool ret = statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, address, accountindex);
    //if (ret) {
    //    auto tx = data::xtx_factory::create_v2_run_contract_tx(address,
    //                                                    accountindex.get_latest_tx_nonce(),
    //                                                    action_name,
    //                                                    action_params,
    //                                                    timestamp);

    //    int32_t r = m_vnode->txpool_proxy()->request_transaction_consensus(tx, true);
    //    xinfo("[xtop_sniffer] call_contract in consensus mode with return code : %d, %s, %s %ld, %lld, %s",
    //        r,
    //        tx->get_digest_hex_str().c_str(),
    //        address.to_string().c_str(),
    //        accountindex.get_latest_tx_nonce(),
    //        timestamp,
    //        action_name.c_str());
    //}
}

void xtop_sniffer::call(common::xaccount_address_t const & address,
                        std::string const & action_name,
                        std::string const & action_params,
                        uint64_t timestamp,
                        uint64_t table_id) const {
    auto const specific_addr = data::make_address_by_prefix_and_subaddr(address.to_string(), table_id);
    call(specific_addr, action_name, action_params, timestamp);
}

void xtop_sniffer::call(common::xaccount_address_t const & source_address,
                            common::xaccount_address_t const & target_address,
                            std::string const & action_name,
                            std::string const & action_params,
                            uint64_t timestamp) const {
    //base::xaccount_index_t accountindex;
    //// int32_t ret_pushtx = xsuccess;
    //bool ret = statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, source_address, accountindex);
    //if (ret) {
    //    auto tx = data::xtx_factory::create_v2_run_contract_tx(source_address,
    //                                                    target_address,
    //                                                    accountindex.get_latest_tx_nonce(),
    //                                                    action_name,
    //                                                    action_params,
    //                                                    timestamp);

    //    int32_t r = m_vnode->txpool_proxy()->request_transaction_consensus(tx, true);
    //    xinfo("[xrole_context_t::fulltableblock_event] call_contract in consensus mode with return code : %d, %s, %s %ld, %lld, %s",
    //            r,
    //            tx->get_digest_hex_str().c_str(),
    //            source_address.to_string().c_str(),
    //            accountindex.get_latest_tx_nonce(),
    //            timestamp,
    //            action_name.c_str());
    //}
}

NS_END4
