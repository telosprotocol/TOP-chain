// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sstream>
#include <gtest/gtest.h>

#include "tests/xsystem_contract/xrec_standby_algorithm/xtest_rec_standby_contract_fixture.h"
#include "xvm/manager/xcontract_manager.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xnative_contract_address.h"
#include "xvm/xcontract_helper.h"

using namespace top;
using namespace top::contract;
using namespace top::xvm;

NS_BEG3(top, tests, rec_standby)


class xtest_rec_standby_contract_algorithm
  : public xtop_test_rec_standby_contract_fixture
  , public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtest_rec_standby_contract_algorithm);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtest_rec_standby_contract_algorithm);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtest_rec_standby_contract_algorithm);

protected:
    void SetUp() override {
        m_registration_data.clear();
        standby_result_store.m_results.clear();
        init();

    }

    void init() {
        auto contract_addr = common::xnode_id_t{sys_contract_rec_standby_pool_addr};
        auto vbstate = make_object_ptr<xvbstate_t>(sys_contract_rec_standby_pool_addr, 1, 1, std::string{}, std::string{}, 0, 0, 0);
        auto unitstate = std::make_shared<data::xunit_bstate_t>(vbstate.get());
        auto account_context = std::make_shared<xaccount_context_t>(unitstate);
        auto contract_helper = std::make_shared<xcontract_helper>(account_context.get(), contract_addr, rec_standby_pool_contract_address);
        rec_standby_contract.set_contract_helper(contract_helper);
        rec_standby_contract.setup();
    }

    void TearDown() override {
    }
};

TEST_F(xtest_rec_standby_contract_algorithm, test_TOP_3495) {
    std::string node_id{"T00000LLyxLtWoTxRt1U5fS3K3asnywoHwENzNTi"};
    common::xnode_id_t xnode_id{node_id};
    std::string program_version_1{"verison_1"};
    top::xpublic_key_t pub_key_1{"test_pub_key_1"};

    data::system_contract::xreg_node_info node_info;
    node_info.consensus_public_key = pub_key_1;
    node_info.m_account_mortgage = 1000000000000;
    node_info.miner_type(common::xminer_type_t::advance);
    node_info.m_account = xnode_id;
    node_info.genesis(false);
    node_info.m_network_ids = std::set<common::xnetwork_id_t>{ common::xtestnet_id };
    add_reg_info(node_info);

    auto & standby_node_info = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(standby_node_info.program_version.empty());
    EXPECT_TRUE(standby_node_info.consensus_public_key.empty());
    EXPECT_TRUE(standby_node_info.genesis == false);

    std::string program_version_2{"version_2"};
    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));
    EXPECT_TRUE(standby_node_info.program_version == program_version_2);
    EXPECT_FALSE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));  // rejoin shouldn't changed the result.

    top::xpublic_key_t pub_key_2{"test_pub_key_2"};
    node_info.consensus_public_key = pub_key_2;
    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));
    EXPECT_TRUE(standby_node_info.consensus_public_key == pub_key_2);
}

TEST_F(xtest_rec_standby_contract_algorithm, test_on_timer_update_pubkey_and_role) {
    std::string node_id{"T00000LLyxLtWoTxRt1U5fS3K3asnywoHwENzNTi"};
    common::xnode_id_t xnode_id{node_id};
    std::string program_version_1{"verison_1"};
    top::xpublic_key_t pub_key_1{"test_pub_key_1"};

    data::system_contract::xreg_node_info node_info;
    node_info.consensus_public_key = pub_key_1;
    node_info.m_account_mortgage = 1000000000000;
    node_info.m_vote_amount = 1000000000;
    node_info.miner_type(common::xminer_type_t::advance);
    node_info.m_account = xnode_id;
    node_info.genesis(false);
    node_info.m_network_ids = std::set<common::xnetwork_id_t>{ common::xtestnet_id };
    add_reg_info(node_info);

    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_1, node_info, standby_result_store));

    auto & standby_node_info = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(standby_node_info.program_version == program_version_1);
    EXPECT_TRUE(standby_node_info.consensus_public_key == pub_key_1);
    EXPECT_TRUE(standby_node_info.genesis == false);

#define rec_standby_on_timer_update rec_standby_contract.update_standby_result_store(m_registration_data, standby_result_store, record, 0)

    EXPECT_TRUE(rec_standby_on_timer_update);

    // changed pub_key in reg && rec_standby on_timer update:
    top::xpublic_key_t pub_key_2{"test_pub_key_2"};
    change_public_key(xnode_id, pub_key_2);
    EXPECT_TRUE(rec_standby_on_timer_update);
    EXPECT_TRUE(standby_node_info.consensus_public_key == pub_key_2);
    change_public_key(xnode_id, pub_key_1);
    EXPECT_TRUE(rec_standby_on_timer_update);

#define EXPECT_HAS(node_type) EXPECT_TRUE(standby_node_info.stake_container.find(node_type) != standby_node_info.stake_container.end())
#define EXPECT_HAS_NOT(node_type) EXPECT_TRUE(standby_node_info.stake_container.find(node_type) == standby_node_info.stake_container.end())

    EXPECT_HAS(common::xnode_type_t::consensus_auditor);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);  // advance won't be elected as archive since v1.3.0
    EXPECT_HAS(common::xnode_type_t::rec);
    EXPECT_HAS(common::xnode_type_t::zec);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS_NOT(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);

    change_miner_type(xnode_id, common::xminer_type_t::validator);
    EXPECT_HAS(common::xnode_type_t::consensus_auditor);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);  // advance won't be elected as archive since v1.3.0
    EXPECT_HAS(common::xnode_type_t::rec);
    EXPECT_HAS(common::xnode_type_t::zec);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS_NOT(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);
    // EXPECT_TRUE(rec_standby_on_timer_update);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    // EXPECT_HAS_NOT(common::xnode_type_t::rec);
    // EXPECT_HAS_NOT(common::xnode_type_t::zec);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS_NOT(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);

    change_miner_type(xnode_id, common::xminer_type_t::edge);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    // EXPECT_HAS_NOT(common::xnode_type_t::rec);
    // EXPECT_HAS_NOT(common::xnode_type_t::zec);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS_NOT(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);
    // EXPECT_TRUE(rec_standby_on_timer_update);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    // EXPECT_HAS_NOT(common::xnode_type_t::rec);
    // EXPECT_HAS_NOT(common::xnode_type_t::zec);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_validator);
    // EXPECT_HAS(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);

    change_miner_type(xnode_id, common::xminer_type_t::exchange);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    // EXPECT_HAS_NOT(common::xnode_type_t::rec);
    // EXPECT_HAS_NOT(common::xnode_type_t::zec);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_validator);
    // EXPECT_HAS(common::xnode_type_t::edge);
    EXPECT_HAS_NOT(common::xnode_type_t::storage_exchange);
    // EXPECT_TRUE(rec_standby_on_timer_update);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    // EXPECT_HAS_NOT(common::xnode_type_t::rec);
    // EXPECT_HAS_NOT(common::xnode_type_t::zec);
    // EXPECT_HAS_NOT(common::xnode_type_t::consensus_validator);
    EXPECT_HAS_NOT(common::xnode_type_t::edge);
    // EXPECT_HAS(common::xnode_type_t::storage_exchange);

#undef rec_standby_on_timer_update
#undef EXPECT_HAS
#undef EXPECT_HAS_NOT
}

TEST_F(xtest_rec_standby_contract_algorithm, test_on_timer_update_stake) {
    std::string node_id{"T00000LLyxLtWoTxRt1U5fS3K3asnywoHwENzNTi"};
    common::xnode_id_t xnode_id{node_id};
    std::string program_version_1{"verison_1"};
    top::xpublic_key_t pub_key_1{"test_pub_key_1"};

    data::system_contract::xreg_node_info node_info;
    node_info.consensus_public_key = pub_key_1;
    node_info.m_account_mortgage = 1000000000000;
    node_info.m_vote_amount = 1;
    node_info.miner_type(common::xminer_type_t::advance);
    node_info.m_account = xnode_id;
    node_info.genesis(false);
    node_info.m_network_ids = std::set<common::xnetwork_id_t>{ common::xtestnet_id };
    EXPECT_TRUE(add_reg_info(node_info));

    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_1, node_info, standby_result_store));

    auto & standby_node_info = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(standby_node_info.program_version == program_version_1);
    EXPECT_TRUE(standby_node_info.consensus_public_key == pub_key_1);
    EXPECT_TRUE(standby_node_info.genesis == false);

#define rec_standby_on_timer_update rec_standby_contract.update_standby_result_store(m_registration_data, standby_result_store, record)
#define EXPECT_HAS(node_type) EXPECT_TRUE(standby_node_info.stake_container.find(node_type) != standby_node_info.stake_container.end())
#define EXPECT_HAS_NOT(node_type) EXPECT_TRUE(standby_node_info.stake_container.find(node_type) == standby_node_info.stake_container.end())
#if defined(XENABLE_MOCK_ZEC_STAKE)
    EXPECT_TRUE(node_info.can_be_auditor()); // ticket check pass.
    EXPECT_HAS(common::xnode_type_t::consensus_auditor);
#else
    EXPECT_FALSE(node_info.can_be_auditor());
    EXPECT_HAS_NOT(common::xnode_type_t::consensus_auditor);
#endif
    EXPECT_HAS_NOT(common::xnode_type_t::storage_archive);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS(common::xnode_type_t::rec);
    EXPECT_HAS(common::xnode_type_t::zec);

    node_info.m_vote_amount = 1000000000;
    EXPECT_TRUE(update_reg_info(node_info));

    EXPECT_TRUE(node_info.can_be_auditor());
    // EXPECT_TRUE(rec_standby_on_timer_update);

    // EXPECT_HAS(common::xnode_type_t::consensus_auditor);
    // EXPECT_HAS(common::xnode_type_t::storage_archive);
    EXPECT_HAS(common::xnode_type_t::consensus_validator);
    EXPECT_HAS(common::xnode_type_t::rec);
    EXPECT_HAS(common::xnode_type_t::zec);
#undef rec_standby_on_timer_update
#undef EXPECT_HAS
#undef EXPECT_HAS_NOT
}

TEST_F(xtest_rec_standby_contract_algorithm, test_genesis_adv_to_fullnode) {
    std::string node_id{"T00000LeEMLtDCHkwrBrK8Gdqfik66Kjokewp23q"};
    common::xnode_id_t xnode_id{node_id};
    std::string program_version_1{"verison_1"};
    top::xpublic_key_t pub_key_1{"BPZmAPKWLhhVDkJvWbSPAp3uoBqfTZG0j2QLyOaT5s3JqOxjIvTQFnmBXNUiMV3xwJ/bp9Sq7vD47fvAiGnC4DA="};

    data::system_contract::xreg_node_info node_info;
    node_info.consensus_public_key = pub_key_1;
    node_info.m_account_mortgage = 1000000000000;
    node_info.miner_type(common::xminer_type_t::advance);
    node_info.m_account = xnode_id;
    node_info.genesis(true);
    node_info.m_network_ids = std::set<common::xnetwork_id_t>{ common::xtestnet_id };
    add_reg_info(node_info);

    auto & standby_node_info = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(standby_node_info.program_version.empty());
    EXPECT_TRUE(standby_node_info.consensus_public_key.empty());
    EXPECT_TRUE(standby_node_info.genesis == false);

    std::string program_version_2{"version_2"};
    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));
    EXPECT_TRUE(standby_node_info.program_version == program_version_2);
    EXPECT_FALSE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));  // rejoin shouldn't changed the result.

    auto & node_info_result = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(node_info_result.genesis == true);
    EXPECT_TRUE(node_info_result.stake(common::xnode_type_t::fullnode) == 0);
}

TEST_F(xtest_rec_standby_contract_algorithm, test_no_genesis_adv_to_fullnode) {
    std::string node_id{"T00000LLyxLtWoTxRt1U5fS3K3asnywoHwENzNTi"};
    common::xnode_id_t xnode_id{node_id};
    std::string program_version_1{"verison_1"};
    top::xpublic_key_t pub_key_1{"test_pub_key_1"};

    data::system_contract::xreg_node_info node_info;
    node_info.consensus_public_key = pub_key_1;
    node_info.m_account_mortgage = 1000000000000;
    node_info.miner_type(common::xminer_type_t::advance);
    node_info.m_account = xnode_id;
    node_info.genesis(false);
    node_info.m_network_ids = std::set<common::xnetwork_id_t>{ common::xtestnet_id };
    add_reg_info(node_info);

    auto & standby_node_info = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(standby_node_info.program_version.empty());
    EXPECT_TRUE(standby_node_info.consensus_public_key.empty());
    EXPECT_TRUE(standby_node_info.genesis == false);

    std::string program_version_2{"version_2"};
    EXPECT_TRUE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));
    EXPECT_TRUE(standby_node_info.program_version == program_version_2);
    EXPECT_FALSE(rec_standby_contract.nodeJoinNetworkImpl(program_version_2, node_info, standby_result_store));  // rejoin shouldn't changed the result.

    auto & node_info_result = standby_result_store.result_of(common::xnetwork_id_t{255}).result_of(xnode_id);
    EXPECT_TRUE(node_info_result.genesis == false);
    //no found fullnode type
    auto it = node_info_result.stake_container.find(common::xnode_type_t::fullnode);
    EXPECT_TRUE(it == node_info_result.stake_container.end());
}

NS_END3
