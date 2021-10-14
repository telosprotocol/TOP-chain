// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xcomponents/xblock_sniffing/xvnode_sniff.h"

#include "xdata/xfull_tableblock.h"
#include "xdata/xtransaction_v2.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvnode/xcomponents/xblock_process/xfulltableblock_process.h"

NS_BEG4(top, vnode, components, sniffing)

xtop_vnode_sniff::xtop_vnode_sniff(observer_ptr<store::xstore_face_t> const & store,
                                   observer_ptr<base::xvnodesrv_t> const& nodesrv,
                                   observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & manager,
                                   observer_ptr<vnetwork::xvnetwork_driver_face_t> const & driver,
                                   observer_ptr<xtxpool_service_v2::xtxpool_proxy_face> const & txpool)
  : m_store(store), m_nodesvr(nodesrv), m_system_contract_manager(manager), m_the_binding_driver(driver), m_txpool_face(txpool) {

    sniff_set();
}

void xtop_vnode_sniff::sniff_set() {
    // common::xnode_type_t type = e->driver->type();
    // common::xnode_type_t type = m_the_binding_driver->type();
    common::xnode_type_t type{m_the_binding_driver->type()};
    bool disable_broadcasts{false};
    xdbg("[xtop_vnode::get_roles_data] node type : %s", common::to_string(type).c_str());

    if (common::has<common::xnode_type_t::consensus_auditor>(type)) {
        xdbg("[xtop_vnode::get_roles_data] add all sharding contracts' rcs");
        type = common::xnode_type_t::consensus_validator;
        disable_broadcasts = true;
    }

    auto const & system_contract_deployment_data = m_system_contract_manager->deployment_data();
    for (auto const & contract_data_pair : system_contract_deployment_data) {
        auto const & contract_address = contract_data_pair.first;
        auto const & contract_data = contract_data_pair.second;

        if (!common::has(type, contract_data.node_type)) {
            continue;
        }
        if (!m_config_map.count(contract_address)) {
            m_config_map.insert(std::make_pair(contract_address, xrole_config_t{contract_data, std::map<common::xaccount_address_t, uint64_t>()}));
        }
        if (disable_broadcasts) {
            m_config_map[contract_address].role_data.broadcast_config.type = contract_runtime::xsniff_broadcast_type_t::invalid;
        }
    }
#if defined(DEBUG)
    for (auto const & data_pair : m_config_map) {
        xdbg("address: %s, driver type: %d", data_pair.first.c_str(), m_the_binding_driver->type());
        auto const & data = data_pair.second;
        xdbg("contract: %p, node type: %d, sniff type: %d, broadcast: %d, %d, timer: %d, action: %s",
             &data.role_data.system_contract,
             data.role_data.node_type,
             data.role_data.sniff_type,
             data.role_data.broadcast_config.type,
             data.role_data.broadcast_config.policy,
             data.role_data.timer_config.interval,
             data.role_data.timer_config.action.c_str());
    }
#endif
}

xvnode_sniff_config_t xtop_vnode_sniff::sniff_config() const {
    xvnode_sniff_config_t config;
    for (auto const & data_pair : m_config_map) {
        auto const & data = data_pair.second;
        auto const & sniff_type = data.role_data.sniff_type;
        if (static_cast<uint32_t>(contract_runtime::xsniff_type_t::broadcast) & static_cast<uint32_t>(sniff_type)) {
            config.insert(std::make_pair(xvnode_sniff_event_type_t::broadcast,
                                         xvnode_sniff_event_config_t{xvnode_sniff_block_type_t::full_block, std::bind(&xtop_vnode_sniff::sniff_broadcast, this, std::placeholders::_1)}));
        } else if (static_cast<uint32_t>(contract_runtime::xsniff_type_t::timer) & static_cast<uint32_t>(sniff_type)) {
            config.insert(std::make_pair(xvnode_sniff_event_type_t::timer,
                                         xvnode_sniff_event_config_t{xvnode_sniff_block_type_t::all, std::bind(&xtop_vnode_sniff::sniff_timer, this, std::placeholders::_1)}));
        } else if (static_cast<uint32_t>(contract_runtime::xsniff_type_t::block) & static_cast<uint32_t>(sniff_type)) {
            config.insert(std::make_pair(xvnode_sniff_event_type_t::block,
                                         xvnode_sniff_event_config_t{xvnode_sniff_block_type_t::full_block, std::bind(&xtop_vnode_sniff::sniff_block, this, std::placeholders::_1)}));
        }
    }

    return config;
}

bool xtop_vnode_sniff::sniff_broadcast(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    return true;
}

bool xtop_vnode_sniff::sniff_timer(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    assert(common::xaccount_address_t{vblock->get_account()} == common::xaccount_address_t{sys_contract_beacon_timer_addr});
    auto const height = vblock->get_height();
    for (auto & role_data_pair : m_config_map) {
        auto const & contract_address = role_data_pair.first;
        auto const & config = role_data_pair.second.role_data;
        if ((static_cast<uint32_t>(contract_runtime::xsniff_type_t::timer) & static_cast<uint32_t>(config.sniff_type)) == 0) {
            continue;
        }
        xdbg("[xtop_vnode_sniff::sniff_timer] block address: %s, height: %lu", vblock->get_account().c_str(), vblock->get_height());
        auto valid = is_valid_timer_call(contract_address, role_data_pair.second, height);
        xdbg("[xtop_vnode_sniff::sniff_timer] contract address %s, interval: %u, valid: %d", contract_address.c_str(), config.timer_config.interval, valid);
        if (!valid) {
            return false;
        }
        base::xstream_t stream(base::xcontext_t::instance());
        stream << height;
        std::string action_params = std::string((char *)stream.data(), stream.size());
        xdbg("[xtop_vnode_sniff::sniff_timer] make tx, action: %s, params: %s", config.timer_config.action.c_str(), action_params.c_str());
        call(contract_address, config.timer_config.action, action_params, vblock->get_cert()->get_gmtime());
    }
    return true;
}

bool xtop_vnode_sniff::sniff_block(xobject_ptr_t<base::xvblock_t> const & vblock) const {
    auto const & block_address = vblock->get_account();
    auto const height = vblock->get_height();
    for (auto & role_data_pair : m_config_map) {
        auto const & contract_address = role_data_pair.first;
        auto const & config = role_data_pair.second.role_data;
        if ((static_cast<uint32_t>(contract_runtime::xsniff_type_t::block) & static_cast<uint32_t>(config.sniff_type)) == 0) {
            continue;
        }

        // table upload contract sniff sharding table addr
        if ((block_address.find(sys_contract_sharding_table_block_addr) != std::string::npos) && (contract_address.value() == sys_contract_sharding_statistic_info_addr)) {
            xdbg("[xtop_vnode::sniff_block] sniff block match, contract: %s, block: %s, height: %llu", contract_address.c_str(), block_address.c_str(), height);
            auto const full_tableblock = (dynamic_cast<xfull_tableblock_t *>(vblock.get()));
            auto const fulltable_statisitc_data = full_tableblock->get_table_statistics();
            auto const statistic_accounts = components::xfulltableblock_process_t::fulltableblock_statistic_accounts(fulltable_statisitc_data, m_nodesvr.get());


            base::xstream_t stream(base::xcontext_t::instance());
            stream << fulltable_statisitc_data;
            stream << statistic_accounts;
            stream << height;
            stream << full_tableblock->get_pledge_balance_change_tgas();
            std::string action_params = std::string((char *)stream.data(), stream.size());
            uint32_t table_id = 0;
            auto result = xdatautil::extract_table_id_from_address(block_address, table_id);
            assert(result);
            {
                // table id check
                auto const & driver_ids = m_the_binding_driver->table_ids();
                auto result = find(driver_ids.begin(), driver_ids.end(), table_id);
                if (result == driver_ids.end()) {
                    return false;
                }
            }
            auto const & table_address = contract::xcontract_address_map_t::calc_cluster_address(contract_address, table_id);

            XMETRICS_GAUGE(metrics::xmetircs_tag_t::contract_table_fullblock_event, 1);
            call(table_address, config.block_config.action, action_params, vblock->get_cert()->get_gmtime());
        }
    }
    return true;
}

bool xtop_vnode_sniff::is_valid_timer_call(common::xaccount_address_t const & address, xrole_config_t & data, const uint64_t height) const {
    static std::vector<common::xaccount_address_t> sys_addr_list{common::xaccount_address_t{sys_contract_rec_elect_edge_addr},
                                                                 common::xaccount_address_t{sys_contract_rec_elect_archive_addr},
                                                                 // common::xaccount_address_t{ sys_contract_zec_elect_edge_addr },
                                                                 // common::xaccount_address_t{ sys_contract_zec_elect_archive_addr },
                                                                 common::xaccount_address_t{sys_contract_rec_elect_zec_addr},
                                                                 common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}};
    bool is_first_block{false};
    if (std::find(std::begin(sys_addr_list), std::end(sys_addr_list), address) != std::end(sys_addr_list)) {
        if (m_store->query_account(address.value())->get_chain_height() == 0) {
            is_first_block = true;
        }
    }

    auto const interval = data.role_data.timer_config.interval;
    assert(interval > 0);
    if (interval != 0 && height != 0 && ((is_first_block && (height % 3) == 0) || (!is_first_block && (height % interval) == 0))) {
        xdbg("[xtop_vnode_sniff::is_valid_timer_call] param check pass, interval: %u, height: %llu, first_block: %d", interval, height, is_first_block);
    } else {
        xdbg("[xtop_vnode_sniff::is_valid_timer_call] param check not pass, interval: %u, height: %llu, first_block: %d", interval, height, is_first_block);
        return false;
    }

    auto & round = data.address_round;
    auto iter = round.find(address);
    if (iter == round.end() || (iter != round.end() && iter->second < height)) {
        round[address] = height;
        return true;
    } else {
        xwarn("[xtop_vnode_sniff::is_valid_timer_call] address %s height check error, last height: %llu, this height : %llu", address.c_str(), round[address], height);
        return false;
    }
}

void xtop_vnode_sniff::call(common::xaccount_address_t const & address, std::string const & action_name, std::string const & action_params, const uint64_t timestamp) const {
    xproperty_asset asset_out{0};
    auto tx = make_object_ptr<data::xtransaction_v2_t>();

    tx->make_tx_run_contract(asset_out, action_name, action_params);
    tx->set_same_source_target_address(address.value());
    xaccount_ptr_t account = m_store->query_account(address.value());
    assert(account != nullptr);
    tx->set_last_trans_hash_and_nonce(account->account_send_trans_hash(), account->account_send_trans_number());
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();

    int32_t r = m_txpool_face->request_transaction_consensus(tx, true);
    xinfo("[xrole_context_t] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
          r,
          tx->get_digest_hex_str().c_str(),
          address.value().c_str(),
          data::to_hex_str(account->account_send_trans_hash()).c_str(),
          account->account_send_trans_number(),
          timestamp);
}

void xtop_vnode_sniff::call(common::xaccount_address_t const & source_address,
                            common::xaccount_address_t const & target_address,
                            std::string const & action_name,
                            std::string const & action_params,
                            uint64_t timestamp) const {
    auto tx = make_object_ptr<xtransaction_v2_t>();
    tx->make_tx_run_contract(action_name, action_params);
    tx->set_different_source_target_address(source_address.value(), target_address.value());
    xaccount_ptr_t account = m_store->query_account(source_address.value());
    assert(account != nullptr);
    tx->set_last_trans_hash_and_nonce(account->account_send_trans_hash(), account->account_send_trans_number());
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();

    int32_t r = m_txpool_face->request_transaction_consensus(tx, true);
    xinfo("[xrole_context_t::fulltableblock_event] call_contract in consensus mode with return code : %d, %s, %s %s %ld, %lld",
            r,
            tx->get_digest_hex_str().c_str(),
            source_address.c_str(),
            data::to_hex_str(account->account_send_trans_hash()).c_str(),
            account->account_send_trans_number(),
            timestamp);
}

NS_END4
