#include "common.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xrole_xips_manager.h"

#include <gtest/gtest.h>

using namespace top::sync;
using namespace top::vnetwork;
using namespace top::data;

#if 0
TEST(xrole_xips_manager_t, add_role) {
    auto validators = get_validator_addresses(0, 0, 0, 5);
    auto auditors = get_auditor_addresses(0, 0, 0, 5);
    auto archives = get_archive_addresses(0, 0, 5);
    auto validator = validators[0];

    xdeceit_node_manager_t blacklist;
    blacklist.add_deceit_node(validators[1]);

    test_xrole_xips_manager_t role_mgr;

    role_mgr.add_role(validator, validators, auditors, archives);
    auto role_xips = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips.self_xip == validator);
    ASSERT_TRUE(role_xips.neighbour_xips->size() == 3);
    ASSERT_TRUE(role_xips.parent_xips->size() == 4);
    ASSERT_TRUE(role_mgr.m_archive_xips->size() == 4);

    // different version
    auto validators1 = get_validator_addresses(0, 1, 0, 5);
    auto auditors1 = get_auditor_addresses(0, 1, 0, 5);
    auto validator1 = validators1[0];

    role_mgr.add_role(validator1, validators1, auditors1, archives);
    auto role_xips1 = role_mgr.m_map[validator1];
    ASSERT_TRUE(role_mgr.m_map.size() == 1);
    ASSERT_TRUE(role_xips1.self_xip == validator1);
    ASSERT_TRUE(role_xips1.neighbour_xips->size() == 3);
    ASSERT_TRUE(role_xips1.parent_xips->size() == 4);

    // different cluster
    auto validators2 = get_validator_addresses(0, 1, 5, 5);
    auto auditors2 = get_auditor_addresses(0, 1, 5, 5);
    auto validator2 = validators2[0];

    role_mgr.add_role(validator2, validators2, auditors2, archives);
    auto role_xips2 = role_mgr.m_map[validator2];
    ASSERT_TRUE(role_mgr.m_map.size() == 2);
    ASSERT_TRUE(role_xips2.self_xip == validator2);
    ASSERT_TRUE(role_xips2.neighbour_xips->size() == 4);
    ASSERT_TRUE(role_xips2.parent_xips->size() == 5);
}

TEST(xrole_xips_manager_t, remove_role) {

    xdeceit_node_manager_t blacklist;

    auto validators = get_validator_addresses(0, 0, 0, 5);
    auto auditors = get_auditor_addresses(0, 0, 0, 5);
    auto archives = get_archive_addresses(0, 0, 5);
    auto validator = validators[0];
    auto validators2 = get_validator_addresses(0, 1, 5, 5);
    auto auditors2 = get_auditor_addresses(0, 1, 5, 5);
    auto validator2 = validators2[0];
    role_mgr.add_role(validator, validators, auditors, archives);
    role_mgr.add_role(validator2, validators2, auditors2, archives);
    ASSERT_TRUE(role_mgr.m_map.size() == 2);
    role_mgr.remove_role(validators[4]);
    ASSERT_TRUE(role_mgr.m_map.size() == 2);
    role_mgr.remove_role(validator);
    ASSERT_TRUE(role_mgr.m_map.size() == 1);
}

TEST(xrole_xips_manager_t, remove_xips_by_id) {

    xdeceit_node_manager_t blacklist;

    auto validators = get_validator_addresses(0, 0, 0, 5);
    auto auditors = get_auditor_addresses(0, 0, 0, 5);
    auto archives = get_archive_addresses(0, 0, 5);
    auto validator = get_validator_address(0, 0, "test10", 10);
    role_mgr.add_role(validator, validators, auditors, archives);
    auto role_xips = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips.self_xip == validator);
    ASSERT_TRUE(role_xips.neighbour_xips->size() == 5);
    ASSERT_TRUE(role_xips.parent_xips->size() == 5);
    ASSERT_TRUE(role_mgr.m_archive_xips->size() == 5);

    role_mgr.remove_xips_by_id(top::common::xnode_id_t{"test3"});

    auto role_xips1 = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips1.neighbour_xips->size() == 4);
    ASSERT_TRUE(role_xips1.parent_xips->size() == 4);
    ASSERT_TRUE(role_mgr.m_archive_xips->size() == 4);
}

TEST(xrole_xips_manager_t, get_rand_peers) {
    // convered by test_xtransaceiver::gossip
}

TEST(xrole_xips_manager_t, vrf_send_newblock) {
    // elect contract address

    top::uint256_t hash_uint256;
    std::string hash{(const char *)hash_uint256.data(), hash_uint256.size()};
    xvnode_address_t self_xip;
    std::vector<std::string> addrs = {sys_contract_rec_registration_addr, sys_contract_zec_elect_consensus_addr, sys_contract_sharding_vote_addr "-0001"};
    // don't have auditor / rec / zec
    auto validators = get_validator_addresses(0, 0, 0, 5);
    role_mgr.add_role(validators[0], validators, {}, {});
    for(auto& addr : addrs) {
        ASSERT_FALSE(role_mgr.vrf_send_newblock(addr, hash, 3, self_xip));
    }
    role_mgr.clear();

    // has auditor / rec / zec
    std::vector<top::common::xnode_type_t> types = {top::common::xnode_type_t::committee, top::common::xnode_type_t::zec, top::common::xnode_type_t::consensus_auditor};
    for(uint32_t i=0;i<types.size();i++) {
        // nodes <= 3
        role_mgr.clear();
        std::vector<top::vnetwork::xvnode_address_t> arr1, arr2;
        switch(types[i]) {
            case top::common::xnode_type_t::committee: arr1 = get_beacon_addresses(0, 0, 2);break;
            case top::common::xnode_type_t::zec: arr1 = get_zec_addresses(0, 0, 2);break;
            case top::common::xnode_type_t::consensus_auditor: arr1 = get_auditor_addresses(0, 0, 0, 2);break;
            default: break;
        }
        role_mgr.add_role(arr1[0], arr1, {}, {});
        ASSERT_TRUE(role_mgr.vrf_send_newblock(addrs[i], hash, 3, self_xip));

        // nodes > 3, and selected
        role_mgr.clear();
        switch(types[i]) {
            case top::common::xnode_type_t::committee: arr2 = get_beacon_addresses(0, 0, 6);break;
            case top::common::xnode_type_t::zec: arr2 = get_zec_addresses(0, 0, 6);break;
            case top::common::xnode_type_t::consensus_auditor: arr2 = get_auditor_addresses(0, 0, 0, 6);break;
            default: break;
        }
        role_mgr.add_role(arr2[0], arr2, {}, {});
        hash.data()[0] = 4;
        ASSERT_TRUE(role_mgr.vrf_send_newblock(addrs[i], hash, 3, self_xip));

        // nodes > 3, and not selected
        hash.data()[0] = 1;
        ASSERT_FALSE(role_mgr.vrf_send_newblock(addrs[i], hash, 3, self_xip));
    }
}
#endif
