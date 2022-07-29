#include "xconfig/xconfig_center.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
NS_BEG2(top, config)
xconfig_center& xconfig_center::instance() {
    static xconfig_center config_center;
    return config_center;
}

std::vector<std::string> xconfig_center::get_table_accounts() {
    std::vector<std::string> v;
/*    const std::vector<std::pair<std::string, int>> table = {
        std::make_pair(std::string{sys_contract_sharding_table_block_addr}, enum_vledger_const::enum_vbucket_has_tables_count),
        std::make_pair(std::string{sys_contract_zec_table_block_addr}, MAIN_CHAIN_ZEC_TABLE_USED_NUM),
        std::make_pair(std::string{sys_contract_beacon_table_block_addr}, MAIN_CHAIN_REC_TABLE_USED_NUM),
        std::make_pair(std::string{sys_contract_eth_table_block_addr}, MAIN_CHAIN_EVM_TABLE_USED_NUM),
        std::make_pair(std::string{sys_contract_relay_table_block_base_addr}, MAIN_CHAIN_RELAY_TABLE_USED_NUM),
    };*/
    for (auto const & t : m_chain_config) {
        for (uint32_t i = 0; i < t.second.m_used_number; i++) {
            v.emplace_back(data::make_address_by_prefix_and_subaddr(t.first, uint16_t(i)).value());
        }
    }
    return v;
}

std::vector<std::string> xconfig_center::get_system_contract_accounts() {
    std::vector<std::string> v;
/*    const std::vector<std::string> unit = {
        sys_contract_rec_registration_addr,
        sys_contract_rec_elect_edge_addr,
        sys_contract_rec_elect_archive_addr,
        sys_contract_rec_elect_exchange_addr,
        sys_contract_rec_elect_rec_addr,
        sys_contract_rec_elect_zec_addr,
        sys_contract_rec_elect_fullnode_addr,
        sys_contract_rec_tcc_addr,
        sys_contract_rec_standby_pool_addr,
        sys_contract_zec_workload_addr,
        sys_contract_zec_vote_addr,
        sys_contract_zec_reward_addr,
        sys_contract_zec_slash_info_addr,
        sys_contract_zec_elect_consensus_addr,
        sys_contract_zec_standby_pool_addr,
        sys_contract_zec_group_assoc_addr,
    };
    const std::vector<std::string> table = {
        sys_contract_sharding_vote_addr,
        sys_contract_sharding_reward_claiming_addr,
        sys_contract_sharding_statistic_info_addr,
    };
    for (auto const & u : unit) {
        v.emplace_back(u);
    }
    for (auto const & t : table) {
        for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; i++) {
            v.emplace_back(data::make_address_by_prefix_and_subaddr(t, uint16_t(i)).value());
        }
    }*/
    for (auto const & u : m_contract_config) {
        if (u.second.m_used_number <= 1) {
            v.emplace_back(u.first);
            continue;
        }
        for (uint32_t i = 0; i < u.second.m_used_number; i++) {
            v.emplace_back(data::make_address_by_prefix_and_subaddr(u.first, uint16_t(i)).value());
        }
    }

    return v;
}

void xconfig_center::init_xchain_config() {
    // config beacon
    {
        xchain_config config;
        config.m_sync_config.emplace_back(
            xsync_config{common::xnode_type_t::frozen, sys_contract_beacon_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
        config.m_txpool_config[common::xnode_type_t::committee] = xtxpool_config(true, base::enum_chain_zone_beacon_index, common::xnode_type_t::committee);
        config.m_used_number = MAIN_CHAIN_ZEC_TABLE_USED_NUM;
        m_chain_config[sys_contract_beacon_table_block_addr] = config;
    }
    // config zec
    {
        xchain_config config;
        config.m_sync_config.emplace_back(
            xsync_config{common::xnode_type_t::frozen, sys_contract_zec_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
        config.m_txpool_config[common::xnode_type_t::committee] = xtxpool_config(true, base::enum_chain_zone_zec_index, common::xnode_type_t::committee);
        config.m_used_number = MAIN_CHAIN_REC_TABLE_USED_NUM;
        m_chain_config[sys_contract_beacon_table_block_addr] = config;
    }
    // config sharding
    {
        xchain_config config;
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::consensus_auditor | common::xnode_type_t::consensus_validator,
                                                       sys_contract_sharding_table_block_addr,
                                                       sync::enum_chain_sync_policy_fast,
                                                       enum_address_table});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_checkpoint, enum_address_table});
        config.m_txpool_config[common::xnode_type_t::consensus_auditor] = xtxpool_config(true, base::enum_chain_zone_consensus_index, common::xnode_type_t::consensus_auditor);
        config.m_txpool_config[common::xnode_type_t::consensus_validator] = xtxpool_config(true, base::enum_chain_zone_consensus_index, common::xnode_type_t::consensus_validator);
        config.m_used_number = enum_vledger_const::enum_vbucket_has_tables_count;
        m_chain_config[sys_contract_sharding_table_block_addr] = config;
    }
    // config eth
    {
        xchain_config config;
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::evm_auditor | common::xnode_type_t::evm_validator, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_txpool_config[common::xnode_type_t::evm_auditor] = xtxpool_config(true, base::enum_chain_zone_evm_index, common::xnode_type_t::evm_auditor);
        config.m_txpool_config[common::xnode_type_t::evm_validator] = xtxpool_config(true, base::enum_chain_zone_evm_index, common::xnode_type_t::evm_validator);
        config.m_used_number = MAIN_CHAIN_EVM_TABLE_USED_NUM;
        m_chain_config[sys_contract_eth_table_block_addr] = config;
    }
    // config relay
    {
        xchain_config config;
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
        config.m_txpool_config[common::xnode_type_t::relay] = xtxpool_config(true, base::enum_chain_zone_relay_index, common::xnode_type_t::relay);
        config.m_used_number = MAIN_CHAIN_RELAY_TABLE_USED_NUM;
        m_chain_config[sys_contract_sharding_table_block_addr] = config;
    }
    init_prune_table();
}

void xconfig_center::init_prune_table() {
    m_chain_config[sys_contract_beacon_table_block_addr].m_prune_type = store::enum_prune_table;
    m_chain_config[sys_contract_beacon_table_block_addr].m_addr_type = enum_address_table;
    m_chain_config[sys_contract_zec_table_block_addr].m_prune_type = store::enum_prune_table;
    m_chain_config[sys_contract_zec_table_block_addr].m_addr_type = enum_address_table;
    m_chain_config[sys_contract_sharding_table_block_addr].m_prune_type = store::enum_prune_table;
    m_chain_config[sys_contract_sharding_table_block_addr].m_addr_type = enum_address_table;
    m_chain_config[sys_contract_eth_table_block_addr].m_prune_type = store::enum_prune_none;
    m_chain_config[sys_contract_eth_table_block_addr].m_addr_type = enum_address_table;
    m_chain_config[sys_contract_sharding_table_block_addr].m_prune_type = store::enum_prune_none;
    m_chain_config[sys_contract_sharding_table_block_addr].m_addr_type = enum_address_table;
}
NS_END2