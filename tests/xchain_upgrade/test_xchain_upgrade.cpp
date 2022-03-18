#include "gtest/gtest.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xchaininit/xchain_params.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xchain_param.h"
#include "xloader/xconfig_offchain_loader.h"

using namespace top::chain_fork;

class test_chain_fork_center : public xtop_chain_fork_config_center {
public:
    void set_chain_fork_config(xchain_fork_config_t const & config);
    void set_local_chain_fork_config(xchain_fork_config_t const & config);
    xchain_fork_config_t get_local_chain_fork_config() const;
    bool is_fork_point_equal(xfork_point_t const & origin_fork_point, xfork_point_t const & fork_point);
    xchain_fork_config_t local_chain_config;
};

void test_chain_fork_center::set_chain_fork_config(xchain_fork_config_t const & config) {
    mainnet_chain_config = config;
}

void test_chain_fork_center::set_local_chain_fork_config(xchain_fork_config_t const & config) {
    local_chain_config = config;
}

xchain_fork_config_t test_chain_fork_center::get_local_chain_fork_config() const {
    return local_chain_config;
}

class test_xchain_upgrade : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
    void load_dev_config() {
        top::data::xdev_params& dev_params = top::data::xdev_params::get_instance();
        auto& config_register = top::config::xconfig_register_t::get_instance();

        auto parse_fork_config = [](std::string str) -> std::map<std::string, uint64_t> {
            using top::base::xstring_utl;
            std::map<std::string, uint64_t> map;
            std::vector<std::string> str_vec;
            xstring_utl::split_string(str, ',', str_vec);
            for (auto s : str_vec) {
                std::vector<std::string> values;
                xstring_utl::split_string(s, '.', values);
                if (values.size() != 2) {
                    continue;
                }
                map.insert({values[0], xstring_utl::touint32(values[1])});
            }
            return map;
        };

        std::string fork_config_str;
        if (config_register.get("fork_config", fork_config_str) && !fork_config_str.empty()) {
            dev_params.fork_config = parse_fork_config(fork_config_str);
        }
    }

public:
    test_chain_fork_center center;
};

// dead code
#if 0
bool test_chain_fork_center::is_fork_point_equal(xfork_point_t const& origin_fork_point, xfork_point_t const& fork_point) {
    return origin_fork_point.description == fork_point.description && origin_fork_point.fork_type == fork_point.fork_type &&
            origin_fork_point.point == fork_point.point;
}

TEST_F(test_xchain_upgrade, test_config_select) {
    using namespace top::config;
    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_mainnet});
    auto mainnet_config = center.chain_fork_config();
    // ASSERT_TRUE(center.is_fork_point_equal(mainnet_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 0, "original fork point"}));

    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_testnet});
    auto testnet_config = center.chain_fork_config();
    ASSERT_FALSE(testnet_config.reward_fork_point.has_value());


}

//TEST_F(test_xchain_upgrade, test_config_center) {
//    using namespace top::config;
//    config_register.set(std::string{xchain_name_configuration_t::name}, std::string{chain_name_mainnet});
//    auto initial_config = center.chain_fork_config();
//    ASSERT_TRUE(initial_config.reward_fork_point.has_value());
//    // ASSERT_TRUE(center.is_fork_point_equal(initial_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 0, "original fork point"}));
//
//
//    xchain_fork_config_t another_chain_config{ xfork_point_t(xfork_point_type_t::logic_time, 100, "original fork point")};
//    center.set_chain_fork_config(another_chain_config);
//    auto set_config =  center.chain_fork_config();
//    ASSERT_TRUE(set_config.reward_fork_point.has_value());
//    // ASSERT_TRUE(center.is_fork_point_equal(set_config.point.value(), xfork_point_t{xtop_fork_point_type_t::logic_time, 100, "original fork point"}));
//    ASSERT_TRUE(center.is_forked(set_config.reward_fork_point, 100));
//    ASSERT_TRUE(center.is_forked(set_config.reward_fork_point, 110));
//    ASSERT_FALSE(center.is_forked(set_config.reward_fork_point, 99));
//}
#endif

// xfork_point_t{xfork_point_type_t::logic_time, 6859080, "block fork point"},
// xfork_point_t{xfork_point_type_t::logic_time, 6859080, "blacklist function fork point"},
// xfork_point_t{xfork_point_type_t::logic_time, 6859080, "node_initial_credit_fork_point"},
// xfork_point_t{xfork_point_type_t::logic_time, 7126740, "v3 block fork point"},
// xfork_point_t{xfork_point_type_t::logic_time, 7126740, "enable fullnode election"},
// xfork_point_t{xfork_point_type_t::logic_time, 7129260, "enable fullnode related func"},
// xfork_point_t{xfork_point_type_t::logic_time, 7221960, "tx v2 fee fork point"},//2022-2-21 10:00:00
// xfork_point_t{xfork_point_type_t::logic_time, 7472520, "election contract store miner type & genesis flag"},
// xfork_point_t{xfork_point_type_t::logic_time, 7473960, "partly remove confirm"},

// top::optional<xfork_point_t> block_fork_point;
// top::optional<xfork_point_t> blacklist_function_fork_point;
// top::optional<xfork_point_t> node_initial_credit_fork_point;
// top::optional<xfork_point_t> V3_0_0_0_block_fork_point;
// top::optional<xfork_point_t> enable_fullnode_election_fork_point;
// top::optional<xfork_point_t> enable_fullnode_related_func_fork_point;
// top::optional<xfork_point_t> tx_v2_fee_fork_point;
// top::optional<xfork_point_t> election_contract_stores_miner_type_and_genesis_fork_point;
// top::optional<xfork_point_t> partly_remove_confirm;
TEST_F(test_xchain_upgrade, test_fork_param_set_one) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"tx_v2_fee_fork_point", 9999996});

    xtop_chain_fork_config_center::update(0, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
    EXPECT_EQ((int)config.tx_v2_fee_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 9999996);
#ifndef XCHAIN_FORKED_BY_DEFAULT
    EXPECT_EQ((int)config.block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.block_fork_point.value().point, 6859080);
    EXPECT_EQ((int)config.blacklist_function_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 6859080);
    EXPECT_EQ((int)config.node_initial_credit_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.node_initial_credit_fork_point.value().point, 6859080);
    EXPECT_EQ((int)config.V3_0_0_0_block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 7126740);
    EXPECT_EQ((int)config.enable_fullnode_election_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 7126740);
    EXPECT_EQ((int)config.enable_fullnode_related_func_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 7129260);
    EXPECT_EQ((int)config.election_contract_stores_miner_type_and_genesis_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.election_contract_stores_miner_type_and_genesis_fork_point.value().point, 7472520);
    EXPECT_EQ((int)config.partly_remove_confirm.value().fork_type, 1);
    EXPECT_EQ(config.partly_remove_confirm.value().point, 7473960);
#endif
}

TEST_F(test_xchain_upgrade, test_fork_param_set_all) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"block_fork_point", 9999990});
    new_config.insert({"blacklist_function_fork_point", 9999991});
    new_config.insert({"node_initial_credit_fork_point", 9999992});
    new_config.insert({"V3_0_0_0_block_fork_point", 9999993});
    new_config.insert({"enable_fullnode_election_fork_point", 9999994});
    new_config.insert({"enable_fullnode_related_func_fork_point", 9999995});
    new_config.insert({"tx_v2_fee_fork_point", 9999996});
    new_config.insert({"election_contract_stores_miner_type_and_genesis_fork_point", 9999997});
    new_config.insert({"partly_remove_confirm", 9999998});
    xtop_chain_fork_config_center::update(0, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
    EXPECT_EQ((int)config.block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.block_fork_point.value().point, 9999990);
    EXPECT_EQ((int)config.blacklist_function_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 9999991);
    EXPECT_EQ((int)config.node_initial_credit_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.node_initial_credit_fork_point.value().point, 9999992);
    EXPECT_EQ((int)config.V3_0_0_0_block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 9999993);
    EXPECT_EQ((int)config.enable_fullnode_election_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 9999994);
    EXPECT_EQ((int)config.enable_fullnode_related_func_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 9999995);
    EXPECT_EQ((int)config.tx_v2_fee_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 9999996);
    EXPECT_EQ((int)config.election_contract_stores_miner_type_and_genesis_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.election_contract_stores_miner_type_and_genesis_fork_point.value().point, 9999997);
    EXPECT_EQ((int)config.partly_remove_confirm.value().fork_type, 1);
    EXPECT_EQ(config.partly_remove_confirm.value().point, 9999998);
}

TEST_F(test_xchain_upgrade, test_fork_param_set_more) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"block_fork_point", 9999990});
    new_config.insert({"blacklist_function_fork_point", 9999991});
    new_config.insert({"node_initial_credit_fork_point", 9999992});
    new_config.insert({"V3_0_0_0_block_fork_point", 9999993});
    new_config.insert({"enable_fullnode_election_fork_point", 9999994});
    new_config.insert({"enable_fullnode_related_func_fork_point", 9999995});
    new_config.insert({"tx_v2_fee_fork_point", 9999996});
    new_config.insert({"election_contract_stores_miner_type_and_genesis_fork_point", 9999997});
    new_config.insert({"partly_remove_confirm", 9999998});
    new_config.insert({"more_fork1", 9999999});
    new_config.insert({"more_fork2", 9999999});
    new_config.insert({"more_fork3", 9999999});

    xtop_chain_fork_config_center::update(0, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
    EXPECT_EQ((int)config.block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.block_fork_point.value().point, 9999990);
    EXPECT_EQ((int)config.blacklist_function_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 9999991);
    EXPECT_EQ((int)config.node_initial_credit_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.node_initial_credit_fork_point.value().point, 9999992);
    EXPECT_EQ((int)config.V3_0_0_0_block_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 9999993);
    EXPECT_EQ((int)config.enable_fullnode_election_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 9999994);
    EXPECT_EQ((int)config.enable_fullnode_related_func_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 9999995);
    EXPECT_EQ((int)config.tx_v2_fee_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 9999996);
    EXPECT_EQ((int)config.election_contract_stores_miner_type_and_genesis_fork_point.value().fork_type, 1);
    EXPECT_EQ(config.election_contract_stores_miner_type_and_genesis_fork_point.value().point, 9999997);
    EXPECT_EQ((int)config.partly_remove_confirm.value().fork_type, 1);
    EXPECT_EQ(config.partly_remove_confirm.value().point, 9999998);
}

TEST_F(test_xchain_upgrade, test_fork_param_part_wrong) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"block_fork_point", 9999990});
    new_config.insert({"blacklist_function_fork_point", 9999991});
    new_config.insert({"node_initial_credit_fork_point", 9999992});
    new_config.insert({"V3_0_0_0_block_fork_point", 9999993});
    new_config.insert({"enable_fullnode_election_fork_point", 9999994});
    new_config.insert({"enable_fullnode_related_func_fork_point", 9999995});
    new_config.insert({"more_fork1", 9999999});
    new_config.insert({"more_fork2", 9999999});
    new_config.insert({"more_fork3", 9999999});

    xtop_chain_fork_config_center::update(0, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
    EXPECT_EQ(config.block_fork_point.value().point, 9999990);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 9999991);
    EXPECT_EQ(config.node_initial_credit_fork_point.value().point, 9999992);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 9999993);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 9999994);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 9999995);
#ifndef XCHAIN_FORKED_BY_DEFAULT
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 7221960);
    EXPECT_EQ(config.election_contract_stores_miner_type_and_genesis_fork_point.value().point, 7472520);
    EXPECT_EQ(config.partly_remove_confirm.value().point, 7473960);
#endif
}

TEST_F(test_xchain_upgrade, test_fork_param_all_wrong) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"more fork1", 9999999});
    new_config.insert({"more fork2", 9999999});
    new_config.insert({"more fork3", 9999999});
    new_config.insert({"more fork4", 9999999});
    new_config.insert({"more fork5", 9999999});
    new_config.insert({"more fork6", 9999999});
    new_config.insert({"more fork7", 9999999});
    new_config.insert({"more fork8", 9999999});
    new_config.insert({"more fork9", 9999999});
    new_config.insert({"more fork10", 9999999});

    xtop_chain_fork_config_center::update(0, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
#ifndef XCHAIN_FORKED_BY_DEFAULT
    EXPECT_EQ(config.block_fork_point.value().point, 6859080);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 6859080);
    EXPECT_EQ(config.node_initial_credit_fork_point.value().point, 6859080);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 7126740);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 7126740);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 7129260);
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 7221960);
    EXPECT_EQ(config.election_contract_stores_miner_type_and_genesis_fork_point.value().point, 7472520);
    EXPECT_EQ(config.partly_remove_confirm.value().point, 7473960);
#endif
}

TEST_F(test_xchain_upgrade, test_fork_param_invalid_time) {
    xtop_chain_fork_config_center::init();

    std::map<std::string, uint64_t> new_config;
    new_config.insert({"block_fork_point", 6859070});
    new_config.insert({"blacklist_function_fork_point", 7100000});
    new_config.insert({"V3_0_0_0_block_fork_point", 6900000});
    new_config.insert({"enable_fullnode_election_fork_point", 7100000});
    new_config.insert({"enable_fullnode_related_func_fork_point", 7300000});

    xtop_chain_fork_config_center::update(7000000, new_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
#ifndef XCHAIN_FORKED_BY_DEFAULT
    EXPECT_EQ(config.block_fork_point.value().point, 6859080);
    EXPECT_EQ(config.blacklist_function_fork_point.value().point, 6859080);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 7126740);
    EXPECT_EQ(config.enable_fullnode_election_fork_point.value().point, 7100000);
    EXPECT_EQ(config.enable_fullnode_related_func_fork_point.value().point, 7300000);
#endif
}

// block_fork_point.2.6000000,v3_block_fork_point.2.7000000,tx_v2_fee_fork_point.3.8000000
TEST_F(test_xchain_upgrade, test_fork_param_from_file) {
    const int max_size = 4096 + 1;
    char prev_absolute_path[max_size] = {0};

    int cnt = readlink("/proc/self/exe", prev_absolute_path, max_size);
    if (cnt < 0 || cnt >= max_size) {
        printf("get test json path error\n");
        return;
    }
    for (auto i = cnt; i >= 0; i--) {
        if (prev_absolute_path[i] == '/') {
            prev_absolute_path[i + 1] = '\0';
            break;
        }
    }

    std::string path = std::string{prev_absolute_path} + "../../../tests/xchain_upgrade/test.json";
    printf("test json path: %s\n" , path.c_str());

    auto & config_center = top::config::xconfig_register_t::get_instance();
    auto offchain_loader = std::make_shared<top::loader::xconfig_offchain_loader_t>(path, std::string{});
    config_center.add_loader(offchain_loader);
    EXPECT_TRUE(config_center.load());
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();
    load_dev_config();

    xtop_chain_fork_config_center::init();
    top::chain_fork::xchain_fork_config_center_t::update(7100000, top::data::xdev_params::get_instance().fork_config);

    auto config = xtop_chain_fork_config_center::get_chain_fork_config();
#ifndef XCHAIN_FORKED_BY_DEFAULT
    EXPECT_EQ(config.block_fork_point.value().point, 6859080);
    EXPECT_EQ(config.V3_0_0_0_block_fork_point.value().point, 7126740);
    EXPECT_EQ(config.tx_v2_fee_fork_point.value().point, 8000000);
#endif
}

