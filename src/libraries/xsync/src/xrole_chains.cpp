#include "xsync/xrole_chains.h"

#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace data;

using nt = common::xnode_type_t;

void xchains_wrapper_t::add(const std::string & id, const xchain_info_t & info) {
    auto it = m_chains.find(id);
    if (it != m_chains.end()) {
        it->second.merge(info);
    } else {
        m_chains.insert(std::make_pair(id, info));
    }
}

void xchains_wrapper_t::merge(const xchains_wrapper_t & other) {
    const map_chain_info_t & chains = other.get_chains();

    for (auto & it : chains) {
        add(it.first, it.second);
    }
}

////////////

xrole_chains_t::xrole_chains_t(const vnetwork::xvnode_address_t & role, const std::set<uint16_t> & table_ids) : m_role(role), m_table_ids(table_ids) {
    m_type = real_part_type(role.type());
    init_chains();
}

void xrole_chains_t::init_chains() {

    add_tables(nt::frozen, common::rec_table_base_address.to_string(), enum_chain_sync_policy_full);
    add_tables(nt::frozen, common::zec_table_base_address.to_string(), enum_chain_sync_policy_full);
    //add_chain(nt::frozen, std::string(sys_contract_eth_table_block_addr)+"@0", enum_chain_sync_policy_full);
    //add_chain(nt::frozen, sys_contract_relay_table_block_addr, enum_chain_sync_policy_full);

    add_tables(nt::consensus_auditor | nt::consensus_validator, common::con_table_base_address.to_string(), enum_chain_sync_policy_fast);
    add_chain(nt::evm_auditor | nt::evm_validator, sys_contract_eth_table_block_addr_with_suffix, enum_chain_sync_policy_fast);

    add_tables(nt::storage_archive, common::con_table_base_address.to_string(), enum_chain_sync_policy_full);
    add_chain(nt::storage_archive, sys_contract_relay_table_block_addr, enum_chain_sync_policy_full);
    add_chain(nt::storage_archive, sys_contract_eth_table_block_addr_with_suffix, enum_chain_sync_policy_full);
    add_chain(nt::storage_archive, sys_drand_addr, enum_chain_sync_policy_full);

    add_tables(nt::storage_exchange, common::con_table_base_address.to_string(), enum_chain_sync_policy_full);
    add_chain(nt::storage_exchange, sys_contract_relay_table_block_addr, enum_chain_sync_policy_full);
    add_chain(nt::storage_exchange, sys_contract_eth_table_block_addr_with_suffix, enum_chain_sync_policy_full);
    add_chain(nt::storage, sys_contract_relay_table_block_addr, enum_chain_sync_policy_full);
    add_chain(nt::storage, sys_contract_eth_table_block_addr_with_suffix, enum_chain_sync_policy_full);
    
    add_chain(nt::fullnode, sys_drand_addr, enum_chain_sync_policy_checkpoint);
    add_tables(nt::fullnode, common::con_table_base_address.to_string(), enum_chain_sync_policy_checkpoint);
    add_chain(nt::fullnode, sys_contract_relay_table_block_addr, enum_chain_sync_policy_full);
    add_chain(nt::fullnode, sys_contract_eth_table_block_addr_with_suffix, enum_chain_sync_policy_full);
}

void xrole_chains_t::add_chain(common::xnode_type_t allow_types,
                               const std::string & address, enum_chain_sync_policy sync_policy) {
    if ((m_type & allow_types) == m_type) {
        auto info = xchain_info_t(address, sync_policy);
        m_chains_wrapper.add(address, info);
    }
}

void xrole_chains_t::add_tables(common::xnode_type_t allow_types,
                                const std::string & address, enum_chain_sync_policy sync_policy) {
    if ((m_type & allow_types) == m_type) {
        if (m_type == nt::frozen) {
            add_rec_or_zec(allow_types, address, sync_policy);
            return;
        } else if (common::has<common::xnode_type_t::storage>(m_type) || common::has<common::xnode_type_t::fullnode>(m_type)) {
            if (address == common::rec_table_base_address.to_string() || address == common::zec_table_base_address.to_string()) {
                add_rec_or_zec(allow_types, address, sync_policy);
                return;
            }
        }

        for (auto table_id : m_table_ids) {
            std::string owner = xdatautil::serialize_owner_str(address, table_id);
            add_chain(allow_types, owner, sync_policy);
        }
    }
}

void xrole_chains_t::add_rec_or_zec(common::xnode_type_t allow_types, const std::string &address, enum_chain_sync_policy sync_policy) {

    std::set<uint16_t> table_ids;

    if (address == common::rec_table_base_address.to_string()) {
        for (uint16_t i = 0; i < MAIN_CHAIN_REC_TABLE_USED_NUM; i++) {
            table_ids.insert(i);
        }
    } else if (address == common::zec_table_base_address.to_string()) {
        for (uint16_t i = 0; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; i++) {
            table_ids.insert(i);
        }
    }

    for (auto table_id : table_ids) {
        std::string owner = xdatautil::serialize_owner_str(address, table_id);
        add_chain(allow_types, owner, sync_policy);
    }
}

NS_END2
