#include "xchain_config/xconfig_center.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xbase/xutl.h"
NS_BEG2(top, config)
xconfig_center::xconfig_center() {
    init_xchain_config();
}
xconfig_center& xconfig_center::instance() {
    static xconfig_center config_center;
    return config_center;
}

std::set<std::string> xconfig_center::get_table_accounts() {
    std::set<std::string> tables;
    for (auto const & t : m_table_config) {
        for (uint32_t i = 0; i < t.second.m_used_number; i++) {
            tables.insert(data::make_address_by_prefix_and_subaddr(t.first, uint16_t(i)).value());
        }
    }
    return tables;
}

void xconfig_center::init_xchain_config() {
    // config beacon
    {
        xtable_config config;
        config.m_txpool_config[common::xnode_type_t::committee] = xtxpool_config(true, base::enum_chain_zone_beacon_index, common::xnode_type_t::committee);
        config.m_used_number = MAIN_CHAIN_REC_TABLE_USED_NUM;
        config.m_table_index = base::enum_chain_zone_beacon_index;
        m_table_config[sys_contract_beacon_table_block_addr] = config;
    }
    // config zec
    {
        xtable_config config;
        config.m_txpool_config[common::xnode_type_t::committee] = xtxpool_config(true, base::enum_chain_zone_zec_index, common::xnode_type_t::committee);
        config.m_used_number = MAIN_CHAIN_ZEC_TABLE_USED_NUM;
        config.m_table_index = base::enum_chain_zone_zec_index;
        xdbg("zec num: %d", config.m_used_number);
        m_table_config[sys_contract_zec_table_block_addr] = config;
    }
    // config sharding
    {
        xtable_config config;
        config.m_txpool_config[common::xnode_type_t::consensus_auditor] = xtxpool_config(true, base::enum_chain_zone_consensus_index, common::xnode_type_t::consensus_auditor);
        config.m_txpool_config[common::xnode_type_t::consensus_validator] = xtxpool_config(true, base::enum_chain_zone_consensus_index, common::xnode_type_t::consensus_validator);
        config.m_used_number = enum_vledger_const::enum_vbucket_has_tables_count;  //enum_vledger_const::enum_vbucket_has_books_count * enum_vledger_const::enum_vbook_has_tables_count;
        config.m_table_index = base::enum_chain_zone_consensus_index;
        m_table_config[sys_contract_sharding_table_block_addr] = config;
    }
    // config eth
    {
        xtable_config config;
        config.m_txpool_config[common::xnode_type_t::evm_auditor] = xtxpool_config(true, base::enum_chain_zone_evm_index, common::xnode_type_t::evm_auditor);
        config.m_txpool_config[common::xnode_type_t::evm_validator] = xtxpool_config(true, base::enum_chain_zone_evm_index, common::xnode_type_t::evm_validator);
        config.m_used_number = MAIN_CHAIN_EVM_TABLE_USED_NUM;
        config.m_table_index = base::enum_chain_zone_evm_index;
        m_table_config[sys_contract_eth_table_block_addr] = config;
    }
    // config relay
    {
        xtable_config config;
        config.m_txpool_config[common::xnode_type_t::relay] = xtxpool_config(true, base::enum_chain_zone_relay_index, common::xnode_type_t::relay);
        config.m_used_number = MAIN_CHAIN_RELAY_TABLE_USED_NUM;
        config.m_table_index = base::enum_chain_zone_relay_index;
        m_table_config[sys_contract_relay_table_block_base_addr] = config;
    }
    init_prune_table();
    init_prune_contract();
    init_sync_config();
}
void xconfig_center::init_sync_config() {
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::frozen, sys_contract_beacon_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_drand_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_drand_addr, sync::enum_chain_sync_policy_checkpoint, enum_address_chain});

    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::frozen, sys_contract_zec_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});

    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::consensus_auditor | common::xnode_type_t::consensus_validator,
                                            sys_contract_sharding_table_block_addr,
                                            sync::enum_chain_sync_policy_fast,
                                            enum_address_table});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_table});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_checkpoint, enum_address_table});

    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::evm_auditor | common::xnode_type_t::evm_validator,
                                            sys_contract_eth_table_block_addr_with_suffix,
                                            sync::enum_chain_sync_policy_full,
                                            enum_address_chain});
    m_sync_config.emplace_back(
        xsync_config{common::xnode_type_t::storage_archive, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(
        xsync_config{common::xnode_type_t::storage_exchange, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_address_chain});

    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
    m_sync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_address_chain});
}
void xconfig_center::init_prune_table() {
    m_table_config[sys_contract_beacon_table_block_addr].m_prune_type = enum_prune_table;
    m_table_config[sys_contract_beacon_table_block_addr].m_addr_type = enum_address_table;
    m_table_config[sys_contract_zec_table_block_addr].m_prune_type = enum_prune_table;
    m_table_config[sys_contract_zec_table_block_addr].m_addr_type = enum_address_table;
    m_table_config[sys_contract_sharding_table_block_addr].m_prune_type = enum_prune_table;
    m_table_config[sys_contract_sharding_table_block_addr].m_addr_type = enum_address_table;
    m_table_config[sys_contract_eth_table_block_addr].m_prune_type = enum_prune_none;
    m_table_config[sys_contract_eth_table_block_addr].m_addr_type = enum_address_table;
    m_table_config[sys_contract_relay_table_block_base_addr].m_prune_type = enum_prune_none;
    m_table_config[sys_contract_relay_table_block_base_addr].m_addr_type = enum_address_table;
}
void xconfig_center::init_prune_contract() {
    m_contract_config[sys_contract_rec_registration_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_edge_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_fullnode_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_archive_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_exchange_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_rec_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_elect_zec_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_tcc_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_rec_standby_pool_addr] = store::enum_prune_none;

    m_contract_config[sys_contract_zec_workload_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_zec_vote_addr] = store::enum_prune_fullunit;
    m_contract_config[sys_contract_zec_reward_addr] = store::enum_prune_fullunit;
    m_contract_config[sys_contract_zec_slash_info_addr] = store::enum_prune_fullunit;
    m_contract_config[sys_contract_zec_elect_consensus_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_zec_standby_pool_addr] = store::enum_prune_none;
    m_contract_config[sys_contract_zec_group_assoc_addr] = store::enum_prune_none;

    for (auto index = 0; index < enum_vledger_const::enum_vbucket_has_tables_count; ++index) {
        std::string addr;
        addr = std::string(sys_contract_sharding_vote_addr) + "@" + std::to_string(index);
        m_contract_config[addr] = store::enum_prune_fullunit;
        addr = std::string(sys_contract_sharding_reward_claiming_addr) + "@" + std::to_string(index);
        m_contract_config[addr] = store::enum_prune_fullunit;
        addr = std::string(sys_contract_sharding_statistic_info_addr) + "@" + std::to_string(index);
        m_contract_config[addr] = store::enum_prune_fullunit;
    }
}
NS_END2