#include "xcommon/xaddress.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xrouter/xrouter.h"
#include "xsync/xgossip_message.h"
#include "xsync/xrole_chains.h"
#include "xvnetwork/xaddress.h"
#include "tests/xsync/common.h"

#include <gtest/gtest.h>

using namespace top;
using namespace top::sync;
using namespace top::data;

#if 0
// old test contain unit
TEST(xrole_chains, archive) {
    top::common::xnode_address_t addr = create_archive_addr(1, "aa", 1);

    std::vector<std::uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.push_back(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // beacon table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // zec table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // fixed system contract
    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };
    for (auto & address : fixed_accounts) {
        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // sharding system contract
    std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

    for (auto & contract : sharding_contract) {
        for (auto table_id : table_ids) {
            std::string address = xdatautil::serialize_owner_str(contract, table_id);

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            chains.erase(it);
        }
    }

    // elect block
    std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_edge_addr,
                                                  sys_contract_rec_elect_archive_addr,
                                                  sys_contract_beacon_timer_addr,
                                                  sys_contract_rec_elect_zec_addr,
                                                  sys_contract_zec_elect_consensus_addr,
                                                  sys_contract_rec_standby_pool_addr,
                                                  sys_contract_zec_group_assoc_addr};

    for (const auto & pair : elect_block_chain) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // root elect
    {
        const std::string address = sys_contract_rec_elect_rec_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> beacon_contract = {
        sys_contract_zec_workload_addr,
        sys_contract_zec_slash_info_addr,
    };

    for (const auto & pair : beacon_contract) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, beacon) {
    top::common::xnode_address_t addr = create_beacon_addr(1, "aa", 1);
    std::vector<std::uint16_t> table_ids = {0};

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // beacon table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };

    for (auto & address : fixed_accounts) {
        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // elect block
    std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_edge_addr,
                                                  sys_contract_rec_elect_archive_addr,
                                                  sys_contract_rec_elect_zec_addr,
                                                  sys_contract_zec_elect_consensus_addr,
                                                  sys_contract_zec_group_assoc_addr};

    for (const auto & pair : elect_block_chain) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // root elect
    {
        const std::string address = sys_contract_rec_elect_rec_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    {
        const std::string & address = sys_contract_beacon_timer_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    {
        const std::string & address = sys_contract_rec_standby_pool_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, zec) {
    top::common::xnode_address_t addr = create_zec_addr(1, "aa", 1);
    std::vector<std::uint16_t> table_ids = {0};

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // zec table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };

    for (auto & address : fixed_accounts) {
        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // elect block
    std::vector<std::string> elect_block_chain = {sys_contract_rec_elect_zec_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

    for (const auto & pair : elect_block_chain) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // root elect
    {
        const std::string address = sys_contract_rec_elect_rec_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    {
        const std::string & address = sys_contract_beacon_timer_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> beacon_contract = {
        sys_contract_zec_workload_addr,
        sys_contract_zec_slash_info_addr,
    };

    for (const auto & pair : beacon_contract) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, auditor) {
    top::common::xnode_address_t addr = create_auditor_addr(1, "aa", 1);

    std::vector<std::uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.push_back(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };

    for (auto & address : fixed_accounts) {
        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // sharding system contract
    std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

    for (auto & contract : sharding_contract) {
        for (auto table_id : table_ids) {
            std::string address = xdatautil::serialize_owner_str(contract, table_id);

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            chains.erase(it);
        }
    }

    // elect block
    std::vector<std::string> elect_block_chain = {
        sys_contract_rec_elect_edge_addr, sys_contract_rec_elect_archive_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

    for (const auto & pair : elect_block_chain) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // root elect
    {
        const std::string address = sys_contract_rec_elect_rec_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> beacon_contract = {
        sys_contract_beacon_timer_addr,
        sys_contract_rec_elect_zec_addr,
        sys_contract_zec_workload_addr,
    };

    for (const auto & pair : beacon_contract) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, validator) {
    top::common::xnode_address_t addr = create_validator_addr(1, "aa", 1);

    std::vector<std::uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.push_back(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> fixed_accounts = {
        sys_contract_rec_tcc_addr,
        sys_contract_rec_registration_addr,
    };

    for (auto & address : fixed_accounts) {
        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // sharding system contract
    std::vector<std::string> sharding_contract = {sys_contract_sharding_vote_addr};

    for (auto & contract : sharding_contract) {
        for (auto table_id : table_ids) {
            std::string address = xdatautil::serialize_owner_str(contract, table_id);

            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());

            ASSERT_EQ(it->second.address, address);
            chains.erase(it);
        }
    }

    // elect block
    std::vector<std::string> elect_block_chain = {
        sys_contract_rec_elect_edge_addr, sys_contract_rec_elect_archive_addr, sys_contract_zec_elect_consensus_addr, sys_contract_zec_group_assoc_addr};

    for (const auto & pair : elect_block_chain) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    // root elect
    {
        const std::string address = sys_contract_rec_elect_rec_addr;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    std::vector<std::string> beacon_contract = {
        sys_contract_beacon_timer_addr,
        sys_contract_rec_elect_zec_addr,
        sys_contract_zec_workload_addr,
    };

    for (const auto & pair : beacon_contract) {
        const std::string & address = pair;

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}
#endif


TEST(xrole_chains, archive) {
    top::common::xnode_address_t addr = create_archive_addr(1, "aa", 1);

    std::set<uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.insert(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // rec
    {
        std::set<uint16_t> table_ids;

        for (uint16_t i = 0; i < MAIN_CHAIN_REC_TABLE_USED_NUM; i++) {
            table_ids.insert(i);
        }

        for (auto &table_id: table_ids) {
            std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), table_id);
            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());
            ASSERT_EQ(it->second.address, address);
            chains.erase(it);
        }
    }

    // zec
    {
        std::set<uint16_t> table_ids;

        for (uint16_t i = 0; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; i++) {
            table_ids.insert(i);
        }

        for (auto &table_id: table_ids) {
            std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, table_id);
            auto it = chains.find(address);
            ASSERT_NE(it, chains.end());
            ASSERT_EQ(it->second.address, address);
            chains.erase(it);
        }
    }

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, beacon) {
    top::common::xnode_address_t addr = create_beacon_addr(1, "aa", 1);
    std::set<uint16_t> table_ids;
    table_ids.insert(0);

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // beacon table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_beacon_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, zec) {
    top::common::xnode_address_t addr = create_zec_addr(1, "aa", 1);
    std::set<uint16_t> table_ids;
    table_ids.insert(0);

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // zec table
    {
        std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, 0);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, auditor) {
    top::common::xnode_address_t addr = create_auditor_addr(1, "aa", 1);

    std::set<uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.insert(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, validator) {
    top::common::xnode_address_t addr = create_validator_addr(1, "aa", 1);

    std::set<uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.insert(i);
    }

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // shard table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}

TEST(xrole_chains, frozen) {
    common::xnode_address_t addr(common::build_frozen_sharding_address());

    std::set<uint16_t> table_ids;
    table_ids.insert(0);

    xrole_chains_t role_chains(addr, table_ids);
    const map_chain_info_t & tmp_chains = role_chains.get_chains_wrapper().get_chains();
    map_chain_info_t chains = tmp_chains;

    // beacon table
    for (auto table_id : table_ids) {
        std::string address = xdatautil::serialize_owner_str(common::rec_table_base_address.to_string(), table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    for (uint16_t table_id=0; table_id<MAIN_CHAIN_ZEC_TABLE_USED_NUM; table_id++) {
        std::string address = xdatautil::serialize_owner_str(sys_contract_zec_table_block_addr, table_id);

        auto it = chains.find(address);
        ASSERT_NE(it, chains.end());

        ASSERT_EQ(it->second.address, address);
        chains.erase(it);
    }

    ASSERT_EQ(chains.size(), 0);
}


TEST(xrole_chains, height_message) {
    top::common::xnode_address_t addr = create_archive_addr(1, "aa", 1);

    std::set<uint16_t> table_ids;
    for (uint32_t i = 0; i < 256; i++) {
        table_ids.insert(i);
    }

    xrole_chains_t role_chains(addr, table_ids);

    const map_chain_info_t & chains = role_chains.get_chains_wrapper().get_chains();
    std::vector<xgossip_chain_info_ptr_t> info_list_request;
    xbyte_buffer_t bloom_data(32, 0);
    std::map<std::string, uint64_t> map_chains;

    // send
    for (const auto & it : chains) {
        const xchain_info_t & chain = it.second;
        xgossip_chain_info_ptr_t info = std::make_shared<xgossip_chain_info_t>();

        info->owner = chain.address;
        info->max_height = 0;
        info_list_request.push_back(info);

        map_chains[chain.address] = 0;
    }

    xgossip_message_t msg;
    xbyte_buffer_t buffer = msg.create_payload(info_list_request, bloom_data, false);

    // response
    std::vector<xgossip_chain_info_ptr_t> info_list_response;
    msg.parse_payload(buffer, info_list_response, bloom_data, false);
    ASSERT_EQ(map_chains.size(), info_list_response.size());

    for (auto & info : info_list_response) {
        ASSERT_EQ(info->max_height, 0);

        auto it = map_chains.find(info->owner);
        ASSERT_NE(it, map_chains.end());
        map_chains.erase(it);
    }

    ASSERT_EQ(map_chains.size(), 0);
}
