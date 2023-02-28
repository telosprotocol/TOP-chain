#include <gtest/gtest.h>
#include <sstream>
#define protected public
#define private   public
#include "../common.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xdeceit_node_manager.h"
#undef protected
#undef private


using namespace top::sync;
using namespace top::vnetwork;
using namespace top::data;
using namespace top;

TEST(xrole_xips_manager_t, add_role) {
    auto validators = get_validator_addresses(0, 0, 0, 5);
    auto auditors = get_auditor_addresses(0, 0, 0, 5);
    auto archives = get_archive_addresses(0, 0, 5);
    auto validator = validators[0];

    xdeceit_node_manager_t blacklist;
    blacklist.add_deceit_node(validators[1]);

    xrole_xips_manager_t role_mgr("");
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> network_driver;
    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<64; i++)
        table_ids.insert(i);

    role_mgr.add_role(validator, validators, auditors, network_driver, table_ids);
    auto role_xips = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips.self_xip == validator);
    std::cout<<"validators size:"<<validators.size()<<std::endl;
    std::cout<<"neighbour_xips size:"<<role_xips.neighbour_xips->size()<<std::endl;
    ASSERT_TRUE(role_xips.neighbour_xips->size() == 4);
    ASSERT_TRUE(role_xips.parent_xips->size() == 5);

    // different version
    auto validators1 = get_validator_addresses(0, 1, 0, 5);
    auto auditors1 = get_auditor_addresses(0, 1, 0, 5);
    auto validator1 = validators1[0];   
    role_mgr.add_role(validator1, validators1, auditors1, network_driver, table_ids);
    auto role_xips1 = role_mgr.m_map[validator1];
    ASSERT_TRUE(role_mgr.m_map.size() == 1);
    ASSERT_TRUE(role_xips1.self_xip == validator1);
    ASSERT_TRUE(role_xips1.neighbour_xips->size() == 4);
    ASSERT_TRUE(role_xips1.parent_xips->size() == 5);

    // different cluster
    auto validators2 = get_validator_addresses(0, 1, 5, 5);
    auto auditors2 = get_auditor_addresses(0, 1, 5, 5);
    auto validator2 = validators2[0];

    role_mgr.add_role(validator2, validators2, auditors2, network_driver, table_ids);
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

    xrole_xips_manager_t role_mgr("");
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> network_driver;
    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<64; i++)
        table_ids.insert(i);

    role_mgr.add_role(validator, validators, auditors, network_driver, table_ids);
    role_mgr.add_role(validator2, validators2, auditors2, network_driver, table_ids);
    ASSERT_TRUE(role_mgr.m_map.size() == 2);
    role_mgr.remove_role(validators[4]);
    ASSERT_TRUE(role_mgr.m_map.size() == 2);
    role_mgr.remove_role(validator);
    ASSERT_TRUE(role_mgr.m_map.size() == 1);
}

TEST(xrole_xips_manager_t, remove_xips_by_id) {

    xdeceit_node_manager_t blacklist;
    xrole_xips_manager_t role_mgr("");
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> network_driver;
    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<64; i++)
        table_ids.insert(i);

    auto validators = get_validator_addresses(0, 0, 0, 5);
    auto auditors = get_auditor_addresses(0, 0, 0, 5);
    auto archives = get_archive_addresses(0, 0, 5);
    auto validator = get_validator_address(0, 0, "T80000fffffffffffffffffffffffffffffffffffffff0", 10);
    role_mgr.add_role(validator, validators, auditors, network_driver, table_ids);
    auto role_xips = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips.self_xip == validator);
    ASSERT_TRUE(role_xips.neighbour_xips->size() == 5);
    ASSERT_TRUE(role_xips.parent_xips->size() == 5);
    //ASSERT_TRUE(role_mgr.m_archive_xips->size() == 5);

    role_mgr.remove_xips_by_id(top::common::xnode_id_t{"T80000fffffffffffffffffffffffffffffffffffffff0"});

    auto role_xips1 = role_mgr.m_map[validator];
    ASSERT_TRUE(role_xips1.neighbour_xips->size() == 4);
    ASSERT_TRUE(role_xips1.parent_xips->size() == 4);
    //ASSERT_TRUE(role_mgr.m_archive_xips->size() == 4);
}

TEST(xrole_xips_manager_t, get_rand_peers) {
    // convered by test_xtransaceiver::gossip
}

TEST(xrole_xips_manager_t, vrf_send_newblock) {
    // elect contract address

    top::uint256_t hash_uint256;
    std::string hash{(const char *)hash_uint256.data(), hash_uint256.size()};
    xvnode_address_t self_xip;
    std::vector<std::string> addrs = {sys_contract_rec_registration_addr, sys_contract_zec_elect_consensus_addr, sys_contract_sharding_vote_addr};
    // don't have auditor / rec / zec
    auto validators = get_validator_addresses(0, 0, 0, 5);

    xrole_xips_manager_t role_mgr("");
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> network_driver;
    std::set<uint16_t> table_ids;
    for (uint16_t i=0; i<64; i++)
        table_ids.insert(i);

    role_mgr.add_role(validators[0], validators, {}, network_driver, table_ids);

}

