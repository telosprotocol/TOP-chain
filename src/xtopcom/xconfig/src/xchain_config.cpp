#include "xconfig/xchain_config.h"
#include "xdata/xnative_contract_address.h"
NS_BEG2(top, config)
xchain_config& xchain_config::instance() {
    static xchain_config chain_config;
    return chain_config;
}

xchain_config::xchain_config() {
    init_xsync_config();
    init_prune_contract();
    init_prune_table();
    init_txpool_config();
}
void xchain_config::init_xsync_config() {
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::frozen, sys_contract_beacon_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_table});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::frozen, sys_contract_zec_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_table});

    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::consensus_auditor | common::xnode_type_t::consensus_validator, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_fast, enum_xsync_address_table});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::evm_auditor | common::xnode_type_t::evm_validator, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});

    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_table});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_archive, sys_drand_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});

    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_table});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage_exchange, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::storage, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});

    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_drand_addr, sync::enum_chain_sync_policy_checkpoint, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_sharding_table_block_addr, sync::enum_chain_sync_policy_checkpoint, enum_xsync_address_table});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_relay_table_block_addr, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
    m_xsync_config.emplace_back(xsync_config{common::xnode_type_t::fullnode, sys_contract_eth_table_block_addr_with_suffix, sync::enum_chain_sync_policy_full, enum_xsync_address_chain});
}
void xchain_config::init_prune_contract() {
    m_prune_contract.clear();
    m_prune_contract[sys_contract_rec_registration_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_edge_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_fullnode_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_archive_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_exchange_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_rec_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_elect_zec_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_tcc_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_rec_standby_pool_addr] = store::enum_prune_none;

    m_prune_contract[sys_contract_zec_workload_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_zec_vote_addr] = store::enum_prune_fullunit;
    m_prune_contract[sys_contract_zec_reward_addr] = store::enum_prune_fullunit;
    m_prune_contract[sys_contract_zec_slash_info_addr] = store::enum_prune_fullunit;
    m_prune_contract[sys_contract_zec_elect_consensus_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_zec_standby_pool_addr] = store::enum_prune_none;
    m_prune_contract[sys_contract_zec_group_assoc_addr] = store::enum_prune_none;

    for (auto index = 0; index < enum_vledger_const::enum_vbucket_has_tables_count; ++index) {
        std::string addr;
        addr = std::string(sys_contract_sharding_vote_addr) + "@" + std::to_string(index);
        m_prune_contract[addr] = store::enum_prune_fullunit;
        addr = std::string(sys_contract_sharding_reward_claiming_addr) + "@" + std::to_string(index);
        m_prune_contract[addr] = store::enum_prune_fullunit;
        addr = std::string(sys_contract_sharding_statistic_info_addr) + "@" + std::to_string(index);
        m_prune_contract[addr] = store::enum_prune_fullunit;
    }
}
void xchain_config::init_prune_table() {
    m_prune_table[sys_contract_beacon_table_block_addr] = store::enum_prune_table;
    m_prune_table[sys_contract_zec_table_block_addr] = store::enum_prune_table;
    m_prune_table[sys_contract_sharding_table_block_addr] = store::enum_prune_table;
    m_prune_table[sys_contract_eth_table_block_addr] = store::enum_prune_none;
    m_prune_table[sys_contract_relay_table_block_base_addr] = store::enum_prune_none;
}
void xchain_config::init_txpool_config() {
    m_txpool_config[common::xnode_type_t::committee] = xtxpool_config(true, base::enum_chain_zone_beacon_index, common::xnode_type_t::committee);
}
NS_END2