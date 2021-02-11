#include "xsync/xrole_chains.h"

#include "xdata/xgenesis_data.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace data;

using nt = common::xnode_type_t;

#define TIME_WINDOWS 24 * 360

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

xrole_chains_t::xrole_chains_t(const vnetwork::xvnode_address_t & role, const std::vector<std::uint16_t> & table_ids) : m_role(role), m_table_ids(table_ids) {
    m_type = real_part_type(role.type());
    init_chains();
}

void xrole_chains_t::init_chains() {
    // first policy
#ifdef SYNC_UNIT
    // second policy
    // auto active_policy_24h = xsync_latest_active_policy::time_policy(TIME_WINDOWS);

    // anchor policy
    auto anchor_policy_enable = xsync_latest_active_policy::anchor_policy();
    xsync_latest_active_policy anchor_policy_none;
#endif

#ifdef SYNC_UNIT
    // elect chains
    add_chain(nt::frozen, sys_contract_rec_elect_rec_addr, empty_policy, empty_policy, empty_policy, history_policy_full);

    add_chain(nt::rec | nt::consensus_auditor | nt::consensus_validator | nt::archive,
              sys_contract_rec_elect_edge_addr,
              empty_policy,
              empty_policy,
              anchor_policy_enable,
              history_policy_full);
    add_chain(nt::rec | nt::consensus_auditor | nt::consensus_validator | nt::archive,
              sys_contract_rec_elect_archive_addr,
              empty_policy,
              empty_policy,
              anchor_policy_enable,
              history_policy_full);
    add_chain(
        nt::rec | nt::zec | nt::consensus_auditor | nt::consensus_validator, sys_contract_beacon_timer_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_chain(nt::archive, sys_contract_beacon_timer_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_chain(nt::consensus_auditor | nt::consensus_validator, sys_contract_rec_elect_zec_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_chain(nt::rec | nt::zec | nt::archive, sys_contract_rec_elect_zec_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_chain(nt::rec | nt::zec | nt::consensus_auditor | nt::consensus_validator | nt::archive,
              sys_contract_zec_elect_consensus_addr,
              empty_policy,
              empty_policy,
              anchor_policy_enable,
              history_policy_full);
    add_chain(nt::rec, sys_contract_rec_standby_pool_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_chain(nt::archive, sys_contract_rec_standby_pool_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_chain(nt::rec | nt::zec | nt::consensus_auditor | nt::consensus_validator | nt::archive,
              sys_contract_zec_group_assoc_addr,
              empty_policy,
              empty_policy,
              anchor_policy_enable,
              history_policy_full);

    // beacon contract
    add_chain(nt::zec | nt::consensus_auditor | nt::consensus_validator, sys_contract_zec_workload_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_chain(nt::zec, sys_contract_zec_slash_info_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_chain(nt::archive, sys_contract_zec_workload_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_chain(nt::archive, sys_contract_zec_slash_info_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);

    // tables
    add_tables(nt::rec, sys_contract_beacon_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_none);
    add_tables(nt::zec, sys_contract_zec_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_none);
    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_none);
    add_tables(nt::archive, sys_contract_beacon_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_full);
    add_tables(nt::archive, sys_contract_zec_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_full);
    add_tables(nt::archive, sys_contract_sharding_table_block_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_full);

    // fix account
    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };
    add_chains(
        fixed_accounts, nt::rec | nt::zec | nt::consensus_auditor | nt::consensus_validator | nt::archive, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);

    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_vote_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_reward_claiming_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_slash_info_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);
    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_workload_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_none);

    add_tables(nt::archive, sys_contract_sharding_vote_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_tables(nt::archive, sys_contract_sharding_reward_claiming_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_tables(nt::archive, sys_contract_sharding_slash_info_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
    add_tables(nt::archive, sys_contract_sharding_workload_addr, empty_policy, empty_policy, anchor_policy_enable, history_policy_full);
#else
    add_tables(nt::frozen, sys_contract_beacon_table_block_addr);
    add_tables(nt::frozen, sys_contract_zec_table_block_addr);
    //add_chain(nt::frozen, sys_contract_beacon_timer_addr, latest_policy_24h, empty_policy, empty_policy, history_policy_full);

    add_tables(nt::rec, sys_contract_beacon_table_block_addr);
    add_tables(nt::zec, sys_contract_zec_table_block_addr);

    // add_tables(nt::edge                                                                       , sys_contract_zec_table_block_addr,       latest_policy_24h,   empty_policy,
    // empty_policy, history_policy_full); add_tables(             nt::zec                                                             , sys_contract_zec_table_block_addr,
    // latest_policy_24h,   empty_policy, empty_policy, history_policy_full); add_tables(                       nt::consensus_auditor | nt::consensus_validator            ,
    // sys_contract_zec_table_block_addr,      latest_policy_24h,   empty_policy, empty_policy, history_policy_full);
    add_tables(nt::consensus_auditor | nt::consensus_validator, sys_contract_sharding_table_block_addr);
    // add_tables(                                                                      nt::archive, sys_contract_zec_table_block_addr,      latest_policy_24h,   empty_policy,
    // empty_policy, history_policy_full);
    add_tables(nt::archive, sys_contract_sharding_table_block_addr);

    // add_chain (nt::rec | nt::zec | nt::consensus_auditor | nt::consensus_validator | nt::archive , sys_contract_beacon_timer_addr, latest_policy_24h, empty_policy, empty_policy,
    // history_policy_full);
#endif
}

void xrole_chains_t::add_chain(common::xnode_type_t allow_types,
                               const std::string & address) {
    if ((m_type & allow_types) == m_type) {
        auto info = xchain_info_t(address);
        m_chains_wrapper.add(address, info);
    }
}

void xrole_chains_t::add_chains(const std::vector<std::string> & addrs,
                                common::xnode_type_t allow_types) {
    if ((m_type & allow_types) == m_type) {
        for (auto & addr : addrs) {
            add_chain(allow_types, addr);
        }
    }
}

void xrole_chains_t::add_tables(common::xnode_type_t allow_types,
                                const std::string & address) {
    if ((m_type & allow_types) == m_type) {
        if (m_type == nt::frozen) {
            std::vector<std::uint16_t> table_ids;

            if (address == sys_contract_beacon_table_block_addr) {
                for (uint16_t i = 0; i < MAIN_CHAIN_REC_TABLE_USED_NUM; i++) {
                    table_ids.push_back(i);
                }
            } else if (address == sys_contract_zec_table_block_addr) {
                for (uint16_t i = 0; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; i++) {
                    table_ids.push_back(i);
                }
            }

            for (auto table_id : table_ids) {
                std::string owner = xdatautil::serialize_owner_str(address, table_id);
                add_chain(allow_types, owner);
            }
        } else {
            for (auto table_id : m_table_ids) {
                std::string owner = xdatautil::serialize_owner_str(address, table_id);
                add_chain(allow_types, owner);
            }
        }
    }
}

NS_END2
