#include <gtest/gtest.h>

#include <sstream>
#define private public

#include "tests/mock/xvchain_creator.hpp"
#include "xapplication/xerror/xerror.h"
#include "xblockstore/xerror/xerror.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xgenesis/xerror/xerror.h"
#include "xgenesis/xgenesis_manager.h"
#include "xstatestore/xstatestore_face.h"
#include "xvledger/xvblockstore.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"

#include <nlohmann/json.hpp>

using namespace top::genesis;
using json = nlohmann::json;

NS_BEG3(top, tests, genesis)

static std::set<std::string> contract_accounts = {
    sys_contract_rec_registration_addr,
    sys_contract_rec_elect_edge_addr,
    sys_contract_rec_elect_archive_addr,
    sys_contract_rec_elect_fullnode_addr,
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

static auto user_property_json =
    R"T(
{
    "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp": {
        "$0": "500600000",
        "$a": "800000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387590",
        "$08": "0"
    },
    "T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388460",
        "$08": "0"
    },
    "T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB": {
        "$0": "500600000",
        "$a": "800000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387170",
        "$08": "0"
    },
    "T00000LLJ8AsN4hREDtCpuKAxJFwqka9LwiAon3M": {
        "$0": "10600000",
        "$a": "1300000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387110",
        "$08": "0"
    },
    "T00000LefzYnVUayJSgeX3XdKCgB4vk7BVUoqsum": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388640",
        "$08": "0"
    },
    "T00000LXqp1NkfooMAw7Bty2iXTxgTCfsygMnxrT": {
        "$0": "45428931123",
        "$a": "1300000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1610430810",
        "$08": "0"
    },
    "T00000LaFmRAybSKTKjE8UXyf7at2Wcw8iodkoZ8": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387890",
        "$08": "0"
    },
    "T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609389510",
        "$08": "0"
    },
    "T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609389210",
        "$08": "0"
    },
    "T00000LcNfcqFPH9vy3EYApkrcXLcQN2hb1ygZWE": {
        "$0": "600600000",
        "$a": "700000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387500",
        "$08": "0"
    },
    "T00000LUv7e8RZLNtnE1K9sEfE9SYe74rwYkzEub": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387770",
        "$08": "0"
    },
    "T00000LKfBYfwTcNniDSQqj8fj5atiDqP8ZEJJv6": {
        "$0": "600600000",
        "$a": "700000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387530",
        "$08": "0"
    },
    "T00000LXRSDkzrUsseZmfJFnSSBsgm754XwV9SLw": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387170",
        "$08": "0"
    },
    "T00000Lgv7jLC3DQ3i3guTVLEVhGaStR4RaUJVwA": {
        "$0": "5603265359",
        "$a": "1100018840",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609736850",
        "$08": "0"
    },
    "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388580",
        "$08": "0"
    },
    "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388940",
        "$08": "0"
    },
    "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo": {
        "$0": "500600000",
        "$a": "800000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388910",
        "$08": "0"
    },
    "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73": {
        "$0": "300600000",
        "$a": "1000000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387950",
        "$08": "0"
    },
    "T00000LTHfpc9otZwKmNcXA24qiA9A6SMHKkxwkg": {
        "$0": "600600000",
        "$a": "700000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609387560",
        "$08": "0"
    },
    "T00000Ldf7KcME5YaNvtFsr6jCFwNU9i7NeZ1b5a": {
        "$0": "400600000",
        "$a": "900000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609388220",
        "$08": "0"
    },
    "T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx": {
        "$0": "100600000",
        "$a": "1200000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609389600",
        "$08": "0"
    },
    "T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN": {
        "$0": "500600000",
        "$a": "800000000",
        "$b": "0",
        "$c": "0",
        "$d": "0",
        "$00": "0",
        "$03": null,
        "$04": "0",
        "$05": "0",
        "$07": "1609389360",
        "$08": "0"
    }
}         
)T";

auto user_property_json_parse = json::parse(user_property_json);

class test_genesis : public testing::Test {
public:
    std::map<common::xaccount_address_t, chain_data::data_processor_t> get_all_user_data() {
        std::map<common::xaccount_address_t, chain_data::data_processor_t> result;
        std::vector<chain_data::data_processor_t> data_vec;
        get_all_user_data(data_vec);
        for (auto const & data : data_vec) {
            result.insert(std::make_pair(common::xaccount_address_t{data.address}, data));
        }
        return result;
    }

    void get_all_user_data(std::vector<chain_data::data_processor_t> & data_vec) {
        for (auto it = user_property_json_parse.begin(); it != user_property_json_parse.end(); it++) {
            common::xaccount_address_t account_address{it.key()};
            chain_data::data_processor_t data;
            data.address = it.key();
            data.top_balance = (it->count(data::XPROPERTY_BALANCE_AVAILABLE)) ?
                                   base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_BALANCE_AVAILABLE).get<std::string>())) :
                                   0;
            data.burn_balance =
                (it->count(data::XPROPERTY_BALANCE_BURN)) ? base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_BALANCE_BURN).get<std::string>())) : 0;
            data.tgas_balance = (it->count(data::XPROPERTY_BALANCE_PLEDGE_TGAS)) ?
                                    base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_BALANCE_PLEDGE_TGAS).get<std::string>())) :
                                    0;
            data.vote_balance = (it->count(data::XPROPERTY_BALANCE_PLEDGE_VOTE)) ?
                                    base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_BALANCE_PLEDGE_VOTE).get<std::string>())) :
                                    0;
            data.lock_balance =
                (it->count(data::XPROPERTY_BALANCE_LOCK)) ? base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_BALANCE_LOCK).get<std::string>())) : 0;
            data.lock_tgas =
                (it->count(data::XPROPERTY_LOCK_TGAS)) ? base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_LOCK_TGAS).get<std::string>())) : 0;
            data.unvote_num =
                (it->count(data::XPROPERTY_UNVOTE_NUM)) ? base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_UNVOTE_NUM).get<std::string>())) : 0;
            data.create_time = (it->count(data::XPROPERTY_ACCOUNT_CREATE_TIME)) ?
                                   base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_ACCOUNT_CREATE_TIME).get<std::string>())) :
                                   0;
            data.lock_token =
                (it->count(data::XPROPERTY_LOCK_TOKEN_KEY)) ? base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_LOCK_TOKEN_KEY).get<std::string>())) : 0;
            if (it->count(data::XPROPERTY_PLEDGE_VOTE_KEY)) {
                for (auto const & item : it->at(data::XPROPERTY_PLEDGE_VOTE_KEY)) {
                    std::string str = base::xstring_utl::base64_decode(item);
                    data.pledge_vote.emplace_back(str);
                }
            }
            data.expire_vote = (it->count(data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY)) ?
                                   base::xstring_utl::touint64(static_cast<std::string>(it->at(data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY).get<std::string>())) :
                                   0;
            data_vec.emplace_back(data);
        }
    }

    void SetUp() override {
        static mock::xvchain_creator creator(false);
        m_store = creator.get_xstore();
        m_blockstore = creator.get_blockstore();
        m_genesis_manager = top::make_unique<xgenesis_manager_t>(top::make_observer(m_blockstore));
        contract::xcontract_deploy_t::instance().deploy_sys_contracts();
        contract::xcontract_manager_t::instance().instantiate_sys_contracts();
        contract::xcontract_manager_t::instance().register_address();
    }
    void TearDown() override {
    }

    store::xstore_face_t * m_store;
    base::xvblockstore_t * m_blockstore;
    std::unique_ptr<xgenesis_manager_t> m_genesis_manager;
};

TEST_F(test_genesis, test_load_accounts) {
    m_genesis_manager->load_accounts();
    m_genesis_manager->m_user_accounts_data = get_all_user_data();

    EXPECT_EQ(m_genesis_manager->m_contract_accounts.size(), 20 + 64 * 4 + 1);
    for (auto const & account : contract_accounts) {
        if (data::is_sys_sharding_contract_address(common::xaccount_address_t{account})) {
            for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
                EXPECT_EQ(m_genesis_manager->m_contract_accounts.count(data::make_address_by_prefix_and_subaddr(account, i)), 1);
            }
        } else {
            EXPECT_EQ(m_genesis_manager->m_contract_accounts.count(common::xaccount_address_t{account}), 1);
        }
    }
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.size(), datauser_accounts.size() + 2);
#ifdef XBUILD_DEV
    EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.size(), datauser_accounts.size());
#endif
    for (auto const & account : datauser_accounts) {
        EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{account}), 1);
#ifdef XBUILD_DEV
        EXPECT_EQ(m_genesis_manager->m_genesis_accounts_data.count(common::xaccount_address_t{account}), 1);
#endif
    }
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{"T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx"}), 1);
    EXPECT_EQ(m_genesis_manager->m_user_accounts_data.count(common::xaccount_address_t{"T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN"}), 1);
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
    std::error_code ec_cmp = top::genesis::error::xenum_errc::genesis_root_has_not_ready;
    EXPECT_EQ(ec, ec_cmp);
}

TEST_F(test_genesis, test_create_genesis_block_before_init) {
    m_genesis_manager->load_accounts();
    m_genesis_manager->m_user_accounts_data = get_all_user_data();
    std::error_code ec;
    m_genesis_manager->create_genesis_of_root_account(ec);
    EXPECT_EQ(bool(ec), false);
    EXPECT_EQ(m_genesis_manager->m_root_finish, true);
    EXPECT_EQ(m_blockstore->exist_genesis_block(data::xrootblock_t::get_rootblock_address()), true);

    // data account
    {
        base::xvaccount_t account{"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp"};
        EXPECT_EQ(m_blockstore->exist_unit(account), false);
        auto vblock_g = m_genesis_manager->create_genesis_block(account, ec);
        EXPECT_EQ(bool(ec), false);
        m_blockstore->store_unit(account, vblock_g.get());

        EXPECT_EQ(m_blockstore->exist_unit(account), true);
        // EXPECT_EQ(m_blockstore->get_latest_executed_block_height(account), 0);
        auto vblock = m_blockstore->load_unit(account, 0);
        auto bstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(vblock.get())->get_bstate();
        EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 500600000);
        EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), 800000000);
        EXPECT_EQ(bstate->load_uint64_var(data::XPROPERTY_ACCOUNT_CREATE_TIME)->get(), 1609387590);
    }
    // contract
    {
        base::xvaccount_t account{sys_contract_zec_reward_addr};
        EXPECT_EQ(m_blockstore->exist_unit(account), false);
        auto vblock_g = m_genesis_manager->create_genesis_block(account, ec);
        EXPECT_EQ(bool(ec), false);
    }
    // genesis
    {
        base::xvaccount_t account{"T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446"};
        m_genesis_manager->m_user_accounts_data.clear();
        m_genesis_manager->m_genesis_accounts_data.insert(std::make_pair(common::xaccount_address_t{"T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446"}, 1000000));
        EXPECT_EQ(m_blockstore->exist_unit(account), false);
        auto vblock_g = m_genesis_manager->create_genesis_block(account, ec);
        EXPECT_EQ(bool(ec), false);
    }
}

TEST_F(test_genesis, test_create_root_twice) {
    std::error_code ec;
    m_genesis_manager->create_genesis_block(data::xrootblock_t::get_rootblock_address(), ec);
    std::error_code ec_cmp = top::genesis::error::xenum_errc::genesis_account_invalid;
    EXPECT_EQ(ec, ec_cmp);
}

TEST_F(test_genesis, test_init_genesis_block_before) {
    EXPECT_EQ(m_genesis_manager->m_root_finish, true);

    uint32_t num{0};
    m_genesis_manager->load_accounts();
    m_genesis_manager->m_user_accounts_data = get_all_user_data();

    for (auto const & account : m_genesis_manager->m_contract_accounts) {
        EXPECT_EQ(m_blockstore->exist_unit(base::xvaccount_t{account.to_string()}), false);
    }
    for (auto const & pair : m_genesis_manager->m_user_accounts_data) {
        if (pair.first.to_string() == "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp") {
            EXPECT_EQ(m_blockstore->exist_unit(pair.first.to_string()), true);
        } else {
            EXPECT_EQ(m_blockstore->exist_unit(pair.first.to_string()), false);
        }
    }
    
    EXPECT_EQ(m_blockstore->exist_unit(base::xvaccount_t{"T00000LKnhu8oSgf4x5hPWUuHB8uC8YQsB1zeDhx"}), false);
    EXPECT_EQ(m_blockstore->exist_unit(base::xvaccount_t{"T00000LKsHzz6D7TEXX3fTiRogHE2nkreR2USoLN"}), false);
}

TEST_F(test_genesis, test_init_genesis_block) {
    EXPECT_EQ(m_genesis_manager->m_root_finish, true);

    EXPECT_EQ(m_blockstore->exist_unit(base::xvaccount_t{"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp"}), true);

    EXPECT_EQ(false, chain_data::xtop_chain_data_processor::check_state());
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    EXPECT_EQ(bool(ec), false);
    EXPECT_EQ(m_genesis_manager->m_root_finish, true);
    EXPECT_EQ(true, chain_data::xtop_chain_data_processor::check_state());
}

TEST_F(test_genesis, test_create_genesis_block_after_init) {
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    EXPECT_EQ(bool(ec), false);
    // contract account
    {
        base::xvaccount_t contract{sys_contract_rec_registration_addr};
        EXPECT_EQ(m_blockstore->exist_unit(contract), true);
        auto block = m_blockstore->create_genesis_block(contract, ec);
        EXPECT_EQ(bool(ec), false);
        EXPECT_NE(block, nullptr);
        auto vblock = m_blockstore->load_unit(contract, 0);
        auto bstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(vblock.get())->get_bstate();
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.empty(), false);
        EXPECT_EQ(property_set.count(data::system_contract::XPORPERTY_CONTRACT_REG_KEY), true);
    }
    // datauser account
    {
        base::xvaccount_t datauser{"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp"};
        EXPECT_EQ(m_blockstore->exist_unit(datauser), true);
        auto block = m_blockstore->create_genesis_block(datauser, ec);
        EXPECT_EQ(bool(ec), false);
        EXPECT_NE(block, nullptr);
        auto vblock = m_blockstore->load_unit(datauser, 0);
        auto bstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(vblock.get())->get_bstate();
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.count(data::XPROPERTY_BALANCE_AVAILABLE), true);
    }
    // common account
    {
        base::xvaccount_t account{"T00000LWtyNvjRk2Z2tcH4j6n27qPnM8agwf9ZJv"};
        EXPECT_EQ(m_blockstore->exist_unit(account), false);
        auto vblock_g = m_blockstore->create_genesis_block(account, ec);
        EXPECT_NE(vblock_g, nullptr);
        m_blockstore->store_unit(account, vblock_g.get());
        EXPECT_EQ(bool(ec), false);
        EXPECT_EQ(m_blockstore->exist_unit(account), true);
        auto vblock = m_blockstore->load_unit(account, 0);
        auto bstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(vblock.get())->get_bstate();
        auto property_set = bstate->get_all_property_names();
        EXPECT_EQ(property_set.empty(), true);
    }
    {
        base::xvaccount_t account{"T00000LWtyNvjRk2Z2tcH4j6n27qPnM8agwf9ZJv"};
        EXPECT_EQ(m_blockstore->exist_unit(account), true);
        auto vblock_g = m_genesis_manager->create_genesis_block(account, ec);
        EXPECT_NE(vblock_g, nullptr);
    }
}

TEST_F(test_genesis, test_blockstore_callback) {
    base::xvaccount_t account{"T00000LLhsJByy2XRLh1jPm7AHZ3X2CpzyxGzfoa"};
    EXPECT_EQ(m_blockstore->exist_unit(account), false);
    std::error_code ec;
    auto block = m_blockstore->create_genesis_block(account, ec);
    m_blockstore->store_unit(account, block.get());
    EXPECT_EQ(bool(ec), false);
    EXPECT_EQ(m_blockstore->exist_unit(account), true);
    auto vblock = m_blockstore->load_unit(account, 0);
    auto bstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(vblock.get())->get_bstate();
    auto property_set = bstate->get_all_property_names();
    EXPECT_EQ(property_set.empty(), true);
}

NS_END3
