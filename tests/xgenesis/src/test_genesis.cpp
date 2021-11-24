#define private public

#include "tests/mock/xvchain_creator.hpp"
#include "xapplication/xerror/xerror.h"
#include "xapplication/xgenesis_manager.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xstake/xstake_algorithm.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"

using namespace top::application;

NS_BEG3(top, tests, application)

mock::xvchain_creator creator;

static std::set<std::string> contract_accounts = {
    sys_contract_rec_registration_addr,
    sys_contract_rec_elect_edge_addr,
    sys_contract_rec_elect_archive_addr,
    sys_contract_rec_elect_rec_addr,
    sys_contract_rec_elect_zec_addr,
    sys_contract_rec_tcc_addr,
    sys_contract_rec_standby_pool_addr,
    sys_contract_zec_workload_addr,
    sys_contract_zec_vote_addr,
    sys_contract_zec_reward_addr,
    sys_contract_zec_slash_info_addr,
    sys_contract_zec_elect_consensus_addr,
    sys_contract_zec_standby_pool_addr,
    sys_contract_zec_group_assoc_addr,
    sys_contract_sharding_vote_addr,
    sys_contract_sharding_reward_claiming_addr,
    sys_contract_sharding_statistic_info_addr,
};

static std::set<std::string> datauser_accounts = {
    "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp",
    "T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446",
    "T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB",
    "T00000LLJ8AsN4hREDtCpuKAxJFwqka9LwiAon3M",
    "T00000LefzYnVUayJSgeX3XdKCgB4vk7BVUoqsum",
    "T00000LXqp1NkfooMAw7Bty2iXTxgTCfsygMnxrT",
    "T00000LaFmRAybSKTKjE8UXyf7at2Wcw8iodkoZ8",
    "T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL",
    "T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg",
    "T00000LcNfcqFPH9vy3EYApkrcXLcQN2hb1ygZWE",
    "T00000LUv7e8RZLNtnE1K9sEfE9SYe74rwYkzEub",
    "T00000LKfBYfwTcNniDSQqj8fj5atiDqP8ZEJJv6",
    "T00000LXRSDkzrUsseZmfJFnSSBsgm754XwV9SLw",
    "T00000Lgv7jLC3DQ3i3guTVLEVhGaStR4RaUJVwA",
    "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB",
    "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy",
    "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo",
    "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73",
    "T00000LTHfpc9otZwKmNcXA24qiA9A6SMHKkxwkg",
    "T00000Ldf7KcME5YaNvtFsr6jCFwNU9i7NeZ1b5a",
};

class test_genesis : public testing::Test {
public:
    void SetUp() override {
        m_store = creator.get_xstore();
        m_blockstore = creator.get_blockstore();
        m_statestore = creator.get_xblkstatestore();
        m_genesis_manager = make_unique<xgenesis_manager_t>(top::make_observer(m_blockstore), make_observer(m_store));
        contract::xcontract_deploy_t::instance().deploy_sys_contracts();
        contract::xcontract_manager_t::instance().instantiate_sys_contracts();
        contract::xcontract_manager_t::instance().register_address();
    }
    void TearDown() override {
    }

    store::xstore_face_t * m_store;
    base::xvblockstore_t * m_blockstore;
    base::xvblkstatestore_t * m_statestore;
    std::unique_ptr<xgenesis_manager_t> m_genesis_manager;
};

TEST_F(test_genesis, test_load_accounts) {
    m_genesis_manager->load_accounts();

    EXPECT_EQ(m_genesis_manager->m_root_account.value(), genesis_root_addr_main_chain);
    EXPECT_EQ(m_genesis_manager->m_contract_accounts.size(), 14 + 64 * 3);
    for (auto const & account : contract_accounts) {
        if (data::is_sys_sharding_contract_address(common::xaccount_address_t{account})) {
            for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
                EXPECT_EQ(m_genesis_manager->m_contract_accounts.count(data::make_address_by_prefix_and_subaddr(account, i)), 1);
            }
        } else {
            EXPECT_EQ(m_genesis_manager->m_contract_accounts.count(common::xaccount_address_t{account}), 1);
        }
    }
#ifdef XBUILD_DEV
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.size(), datauser_accounts.size() + 2);
    EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.size(), datauser_accounts.size());
    for (auto const & account : datauser_accounts) {
        EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{account}), 1);
        EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.count(common::xaccount_address_t{account}), 1);
    }
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{"T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx"}), 1);
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{"T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN"}), 1);
#endif
}

TEST_F(test_genesis, test_release_accounts) {
    m_genesis_manager->load_accounts();
    m_genesis_manager->release_accounts();

    EXPECT_EQ(m_genesis_manager->m_contract_accounts.size(), 0);
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.size(), 0);
    EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.size(), 0);
}

TEST_F(test_genesis, test_root_not_ready) {
    base::xvaccount_t account{std::string{"T00000LWtyNvjRk2Z2tcH4j6n27qPnM8agwf9ZJv"}};
    std::error_code ec;
    m_genesis_manager->create_genesis_block(account, ec);
    std::error_code ec_cmp = top::application::error::xenum_errc::genesis_root_has_not_ready;
    EXPECT_EQ(ec, ec_cmp);
}

TEST_F(test_genesis, test_create_genesis_block_before_init) {
    m_genesis_manager->load_accounts();
    std::error_code ec;
    m_genesis_manager->create_genesis_of_root_account(base::xvaccount_t{m_genesis_manager->m_root_account.value()}, ec);
    EXPECT_EQ(bool(ec), false);
    m_genesis_manager->m_root_finish = true;

#ifdef XBUILD_DEV
    base::xvaccount_t account{"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp"};
    EXPECT_EQ(m_blockstore->exist_genesis_block(account), false);
    m_genesis_manager->create_genesis_block(account, ec);
    EXPECT_EQ(bool(ec), false);

    EXPECT_EQ(m_blockstore->exist_genesis_block(account), true);
    EXPECT_EQ(m_blockstore->get_latest_executed_block_height(account), 0);
    auto vblock = m_blockstore->get_genesis_block(account);
    auto bstate = m_statestore->get_block_state(vblock.get());
    EXPECT_EQ(bstate->load_token_var(XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 500600000);
    EXPECT_EQ(bstate->load_token_var(XPROPERTY_BALANCE_BURN)->get_balance(), 800000000);
    EXPECT_EQ(bstate->load_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME)->get(), 1609387590);
#endif
}

TEST_F(test_genesis, test_init_genesis_block_before) {
    EXPECT_EQ(m_genesis_manager->m_root_finish, false);
    EXPECT_EQ(m_genesis_manager->m_init_finish, false);

    m_genesis_manager->load_accounts();

    for (auto const & account : m_genesis_manager->m_contract_accounts) {
        if (data::is_sys_sharding_contract_address(account)) {
            for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
                EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{(data::make_address_by_prefix_and_subaddr(account.value(), i)).value()}), false);
            }
        } else {
            EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{account.value()}), false);
        }
    }
#ifdef XBUILD_DEV
    for (auto const & pair : m_genesis_manager->m_user_accounts_data) {
        if (pair.first.value() == "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp") {
            EXPECT_EQ(m_blockstore->exist_genesis_block(pair.first.value()), true);
        } else {
            EXPECT_EQ(m_blockstore->exist_genesis_block(pair.first.value()), false);
        }
    }
    EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{"T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx"}), false);
    EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{"T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN"}), false);
#endif
}

TEST_F(test_genesis, test_init_genesis_block) {
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    EXPECT_EQ(bool(ec), false);
    EXPECT_EQ(m_genesis_manager->m_root_finish, true);
    EXPECT_EQ(m_genesis_manager->m_init_finish, true);

    EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{m_genesis_manager->m_root_account.value()}), true);
    for (auto const & account : m_genesis_manager->m_contract_accounts) {
        if (data::is_sys_sharding_contract_address(account)) {
            for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
                EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{(data::make_address_by_prefix_and_subaddr(account.value(), i)).value()}), true);
            }
        } else {
            EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{account.value()}), true);
        }
    }
#ifdef XBUILD_DEV
    for (auto const & pair : m_genesis_manager->m_user_accounts_data) {
        EXPECT_EQ(m_blockstore->exist_genesis_block(pair.first.value()), true);
    }
    EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{"T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx"}), true);
    EXPECT_EQ(m_blockstore->exist_genesis_block(base::xvaccount_t{"T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN"}), true);
#endif

    EXPECT_EQ(m_genesis_manager->m_root_finish, true);
    EXPECT_EQ(m_genesis_manager->m_init_finish, true);
    EXPECT_EQ(m_genesis_manager->m_contract_accounts.size(), 0);
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.size(), 0);
    EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.size(), 0);
}

TEST_F(test_genesis, test_create_genesis_block_after_init) {
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    EXPECT_EQ(bool(ec), false);
    // contract account
    {
        base::xvaccount_t contract{sys_contract_rec_registration_addr};
        EXPECT_EQ(m_blockstore->exist_genesis_block(contract), true);
        m_genesis_manager->create_genesis_block(contract, ec);
        EXPECT_EQ(bool(ec), false);
        auto vblock = m_blockstore->get_genesis_block(contract);
        auto bstate = m_statestore->get_block_state(vblock.get());
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.empty(), false);
        EXPECT_EQ(property_set.count(xstake::XPORPERTY_CONTRACT_REG_KEY), true);
    }
#ifdef XBUILD_DEV
    // datauser account
    {
        base::xvaccount_t datauser{"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp"};
        EXPECT_EQ(m_blockstore->exist_genesis_block(datauser), true);
        m_genesis_manager->create_genesis_block(datauser, ec);
        EXPECT_EQ(bool(ec), false);
        auto vblock = m_blockstore->get_genesis_block(datauser);
        auto bstate = m_statestore->get_block_state(vblock.get());
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.count(XPROPERTY_BALANCE_AVAILABLE), true);
    }
#endif
    // common account
    {
        base::xvaccount_t account{"T00000LWtyNvjRk2Z2tcH4j6n27qPnM8agwf9ZJv"};
        EXPECT_EQ(m_blockstore->exist_genesis_block(account), false);
        m_genesis_manager->create_genesis_block(account, ec);
        EXPECT_EQ(bool(ec), false);
        EXPECT_EQ(m_blockstore->exist_genesis_block(account), true);
        auto vblock = m_blockstore->get_genesis_block(account);
        auto bstate = m_statestore->get_block_state(vblock.get());
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.empty(), true);
    }
}

TEST_F(test_genesis, test_blockstore_callback) {
    base::xvaccount_t account{"T00000LLhsJByy2XRLh1jPm7AHZ3X2CpzyxGzfoa"};
    EXPECT_EQ(m_blockstore->exist_genesis_block(account), false);
    std::error_code ec;
    m_blockstore->create_genesis_block(account, ec);
    EXPECT_EQ(bool(ec), false);
    EXPECT_EQ(m_blockstore->exist_genesis_block(account), true);
    auto vblock = m_blockstore->get_genesis_block(account);
    auto bstate = m_statestore->get_block_state(vblock.get());
    auto property_set = bstate->get_all_property_names();
    EXPECT_EQ(property_set.empty(), true);
}

NS_END3