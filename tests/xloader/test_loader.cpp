#define private public
#include "gtest/gtest.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xchaininit/xchain_params.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xchain_param.h"
#include "xgenesis/xgenesis_manager.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvledger.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"

using namespace top::loader;

class test_loader_api : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
    void get_current_path(char * path, int size) {
        int cnt = readlink("/proc/self/exe", path, size);
        if (cnt < 0 || cnt >= size) {
            printf("get test json path error\n");
            return;
        }
        for (auto i = cnt; i >= 0; i--) {
            if (path[i] == '/') {
                path[i + 1] = '\0';
                break;
            }
        }
    }
};

class test_onchain_loader : public xconfig_onchain_loader_t {
public:
    test_onchain_loader(top::observer_ptr<top::store::xstore_face_t> const & store_ptr,
                        top::observer_ptr<top::mbus::xmessage_bus_face_t> const & bus,
                        top::observer_ptr<top::time::xchain_time_face_t> const & logic_timer)
      : xconfig_onchain_loader_t(store_ptr, bus, logic_timer) {
    }
};

TEST_F(test_loader_api, onchain_loader_filter_changes) {
    auto store = top::store::xstore_factory::create_store_with_memdb();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(store.get()), top::make_observer(mbus), top::make_observer(chain_timer.get()));
    onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

    // filter modification
    std::map<std::string, std::string> property_map = {{"id1", "modify"}, {"id2", "test2"}, {"id3", "test3"}};
    std::map<std::string, std::string> filter_map;
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id1"], "modify");
    property_map = {{"id1", ""}, {"id2", "test2"}, {"id3", "test3"}};
    filter_map.clear();
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id1"], "");

    // filter addition
    property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}, {"id4", "test4"}};
    filter_map.clear();
    onchain_loader.filter_changes(property_map, filter_map);
    EXPECT_TRUE(filter_map.size() == 1);
    EXPECT_EQ(filter_map["id4"], "test4");

    // delete in delete case
}

TEST_F(test_loader_api, onchain_loader_delete_params) {
    auto store = top::store::xstore_factory::create_store_with_memdb();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(store.get()), top::make_observer(mbus), top::make_observer(chain_timer.get()));
    onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

    // delete case
    std::map<std::string, std::string> property_map = {{"id1", "modify"}, {"id2", "test2"}};
    std::map<std::string, std::string> delete_map;
    onchain_loader.get_deleted_params(property_map, delete_map);
    EXPECT_TRUE(delete_map.size() == 1);
    EXPECT_EQ(delete_map["id3"], "test3");
}

TEST_F(test_loader_api, onchain_loader_param_changed) {
    auto store = top::store::xstore_factory::create_store_with_memdb();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto mbus = new top::mbus::xmessage_bus_t();

    test_onchain_loader onchain_loader(top::make_observer(store.get()), top::make_observer(mbus), top::make_observer(chain_timer.get()));
    onchain_loader.m_last_param_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};

    // not change
    std::map<std::string, std::string> property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}};
    EXPECT_FALSE(onchain_loader.onchain_param_changed(property_map));

    // changed
    // 1. modify
    property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "modify"}};
    EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));

    // 2. add
    property_map = {{"id1", "test1"}, {"id2", "test2"}, {"id3", "test3"}, {"id4", "test4"}};
    EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));

    // 3. delete
    property_map = {{"id1", "test1"}, {"id2", "test2"}};
    EXPECT_TRUE(onchain_loader.onchain_param_changed(property_map));
}

std::shared_ptr<top::xbase_timer_driver_t> m_timer_driver;
top::xobject_ptr_t<top::mbus::xmessage_bus_face_t> m_bus;
top::xobject_ptr_t<top::store::xstore_face_t> m_store;
top::xobject_ptr_t<top::time::xchain_time_face_t> m_logic_timer;
top::xobject_ptr_t<top::base::xvblockstore_t> m_blockstore;
std::unique_ptr<top::genesis::xgenesis_manager_t> m_genesis_manager;

void store_init() {
    auto io_obj = std::make_shared<top::xbase_io_context_wrapper_t>();
    m_timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_obj);
    m_logic_timer = top::make_object_ptr<top::time::xchain_timer_t>(m_timer_driver);
    m_bus = top::make_object_ptr<top::mbus::xmessage_bus_t>(true, 1000);
    m_store = top::store::xstore_factory::create_store_with_memdb();
    top::base::xvchain_t::instance().set_xdbstore(m_store.get());
    top::base::xvchain_t::instance().set_xevmbus(m_bus.get());
    m_blockstore.attach(top::store::get_vblockstore());

    m_genesis_manager = top::make_unique<top::genesis::xgenesis_manager_t>(top::make_observer(m_blockstore.get()), make_observer(m_store));
    top::contract::xcontract_deploy_t::instance().deploy_sys_contracts();
    top::contract::xcontract_manager_t::instance().instantiate_sys_contracts();
    top::contract::xcontract_manager_t::instance().register_address();
    std::error_code ec;
    m_genesis_manager->init_genesis_block(ec);
    top::error::throw_error(ec);
}

TEST_F(test_loader_api, onchain_offchain_default) {
    store_init();

    const int max_size = 4096 + 1;
    char prev_absolute_path[max_size] = {0};
    get_current_path(prev_absolute_path, max_size);
    std::string file = std::string{prev_absolute_path} + "../../../tests/xloader/test1.json";

    auto& config_center = top::config::xconfig_register_t::get_instance();
    auto offchain_loader = std::make_shared<top::loader::xconfig_offchain_loader_t>(file, std::string{});
    config_center.add_loader(offchain_loader);
    EXPECT_TRUE(config_center.load());
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();

    auto loader = std::make_shared<top::loader::xconfig_onchain_loader_t>(make_observer(m_store), top::make_observer(m_bus.get()), make_observer(m_logic_timer));
    top::config::xconfig_register_t::get_instance().add_loader(loader);
    top::config::xconfig_register_t::get_instance().load();
    config_center.remove_loader(loader);

    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit), ASSET_TOP(200000));
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit), ASSET_TOP(500000));
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit), ASSET_TOP(1000000));
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval), 14 * 24 * 3600 / 10);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval), 4297);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_distribute_interval), 4297);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval), 17);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward), 0);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend), 0);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval), 66);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block), 8640);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_time_block), 8640);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block), 368640);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_table_block), 368640);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value), 10);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_slash_threshold_value), 10);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_award_creditscore), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_award_creditscore), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment), 103680);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration), 3110400);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_nodedeposit_lock_duration_increment), 103680);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_max_nodedeposit_lock_duration), 3110400);

    EXPECT_EQ(XGET_CONFIG(redeem_interval), 25920);
    EXPECT_EQ(XGET_CONFIG(pledge_vote_merge_interval), 8640);
    EXPECT_EQ(XGET_CONFIG(pledge_vote_clock_per_day), 8640);
}

TEST_F(test_loader_api, onchain_offchain_test) {
    const int max_size = 4096 + 1;
    char prev_absolute_path[max_size] = {0};
    get_current_path(prev_absolute_path, max_size);
    std::string file = std::string{prev_absolute_path} + "../../../tests/xloader/test2.json";

    auto& config_center = top::config::xconfig_register_t::get_instance();
    config_center.m_params_map.clear();
    auto offchain_loader = std::make_shared<top::loader::xconfig_offchain_loader_t>(file, std::string{});
    config_center.add_loader(offchain_loader);
    EXPECT_TRUE(config_center.load());
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();

    auto loader = std::make_shared<top::loader::xconfig_onchain_loader_t>(make_observer(m_store), top::make_observer(m_bus.get()), make_observer(m_logic_timer));
    top::config::xconfig_register_t::get_instance().add_loader(loader);
    top::config::xconfig_register_t::get_instance().load();
    config_center.remove_loader(loader);

    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval), 2);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num), 5);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval), 10);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval), 60);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_distribute_interval), 60);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval), 20);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward), 100);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend), 100);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_time_block), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block), 16);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_table_block), 16);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_slash_threshold_value), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_award_creditscore), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_award_creditscore), 10000);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration), 1200);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_nodedeposit_lock_duration_increment), 30);
    EXPECT_EQ(XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_max_nodedeposit_lock_duration), 1200);

    EXPECT_EQ(XGET_CONFIG(redeem_interval), 8640);
    EXPECT_EQ(XGET_CONFIG(pledge_vote_merge_interval), 10);
    EXPECT_EQ(XGET_CONFIG(pledge_vote_clock_per_day), 10);
}