#include "gtest/gtest.h"
#include "xdata/xblock.h"
#include "xvledger/xaccountindex.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xblocktool.h"
#include "xcrypto/xcrypto_util.h"

using namespace top;
using namespace top::data;

class test_tableindex : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

std::map<std::string, xaccount_index_t> create_accounts_indexs(uint64_t count) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    std::map<std::string, xaccount_index_t> accounts_info;
    for (uint64_t i = 0; i < count; i++) {
        xaccount_index_t info(i);
        std::string addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
        accounts_info[addr] = info;
    }
    xassert(accounts_info.size() == count);
    return accounts_info;
}


TEST_F(test_tableindex, unit_index_info_1) {
    {
        uint64_t unit_height = 100;
        uint64_t unit_clock = 200;
        std::string unit_hash = "123456789";
        base::enum_xvblock_class block_class = base::enum_xvblock_class_full;
        base::enum_xvblock_type block_type = base::enum_xvblock_type_cmds;
        enum_xblock_consensus_type consensus_type = enum_xblock_consensus_flag_locked;
        bool has_unconfirm_tx = true;
        bool is_account_destroy = true;
        xaccount_index_t info1{unit_height, unit_hash, block_class, block_type, consensus_type, has_unconfirm_tx, is_account_destroy};
        ASSERT_EQ(info1.get_latest_unit_height(), unit_height);
        ASSERT_EQ(info1.is_match_unit_hash(unit_hash), true);
        ASSERT_EQ(info1.get_latest_unit_class(), block_class);
        ASSERT_EQ(info1.get_latest_unit_type(), block_type);
        ASSERT_EQ(info1.get_latest_unit_consensus_type(), consensus_type);
    }
    {
        uint64_t unit_height = 100;
        uint64_t unit_clock = 200;
        std::string unit_hash = "44444444";
        base::enum_xvblock_class block_class = base::enum_xvblock_class_nil;
        base::enum_xvblock_type block_type = base::enum_xvblock_type_user;
        enum_xblock_consensus_type consensus_type = enum_xblock_consensus_flag_committed;
        bool has_unconfirm_tx = true;
        bool is_account_destroy = false;
        xaccount_index_t info1{unit_height, unit_hash, block_class, block_type, consensus_type, has_unconfirm_tx, is_account_destroy};
        ASSERT_EQ(info1.get_latest_unit_height(), unit_height);
        ASSERT_EQ(info1.is_match_unit_hash(unit_hash), true);
        ASSERT_EQ(info1.get_latest_unit_class(), block_class);
        ASSERT_EQ(info1.get_latest_unit_type(), block_type);
        ASSERT_EQ(info1.get_latest_unit_consensus_type(), consensus_type);
    }
}

TEST_F(test_tableindex, bucket_1) {
    xaccount_index_t info1{1};

    xtable_mbt_bucket_node_ptr_t bucket = make_object_ptr<xtable_mbt_bucket_node_t>();
    bucket->set_account_index("account1", info1);
    {
        xaccount_index_t info;
        xassert(true == bucket->get_account_index("account1", info));
        xassert(info.get_latest_unit_height() == info1.get_latest_unit_height());
        xassert(info.is_has_unconfirm_tx() == info1.is_has_unconfirm_tx());
        xassert(info.is_account_destroy() == info1.is_account_destroy());
    }
    std::string root1 = bucket->build_bucket_hash();
    std::string root2 = bucket->build_bucket_hash();
    ASSERT_EQ(root1, root2);

    // store tree basic
    std::string bucket_str;
    xassert(bucket->serialize_to_string(bucket_str) > 0);
    xtable_mbt_bucket_node_ptr_t bucket2 = make_object_ptr<xtable_mbt_bucket_node_t>();
    bucket2->serialize_from_string(bucket_str);
    {
        xaccount_index_t info;
        xassert(true == bucket2->get_account_index("account1", info));
        xassert(info.get_latest_unit_height() == info1.get_latest_unit_height());
        xassert(info.is_has_unconfirm_tx() == info1.is_has_unconfirm_tx());
    }
    std::string root3 = bucket2->build_bucket_hash();
    ASSERT_EQ(root1, root3);
}

TEST_F(test_tableindex, bucket_tree_1) {
    std::map<std::string, xaccount_index_t> accounts_info;
    xaccount_index_t info1{1};
    xaccount_index_t info2{2};
    accounts_info["account1"] = info1;
    accounts_info["account2"] = info2;

    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    tree->set_accounts_index_info(accounts_info);
    ASSERT_EQ(tree->get_bucket_size(), 2);
    ASSERT_EQ(tree->get_account_size(), 2);
    {
        xaccount_index_t info;
        xassert(true == tree->get_account_index("account1", info));
        xassert(info.get_latest_unit_height() == info1.get_latest_unit_height());
    }
    {
        xaccount_index_t info;
        xassert(true == tree->get_account_index("account2", info));
        xassert(info.get_latest_unit_height() == info2.get_latest_unit_height());
    }
    {
        xaccount_index_t info;
        xassert(false == tree->get_account_index("account3", info));
    }
}

TEST_F(test_tableindex, bucket_tree_2) {
    std::map<std::string, xaccount_index_t> accounts_info;
    xaccount_index_t info1{1};
    xaccount_index_t info2{2};
    accounts_info["account1"] = info1;
    accounts_info["account2"] = info2;

    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    tree->set_accounts_index_info(accounts_info);
    ASSERT_EQ(tree->check_tree(), true);

    // store tree basic
    std::string tree_str;
    xassert(tree->serialize_to_string(tree_str) > 0);
    xtable_mbt_ptr_t tree2 = make_object_ptr<xtable_mbt_t>();
    xassert(tree2->serialize_from_string(tree_str) > 0);
    ASSERT_EQ(tree2->check_tree(), true);

    {
        xaccount_index_t info;
        tree2->get_account_index("account1", info);
        xassert(info.get_latest_unit_height() == info1.get_latest_unit_height());
        xassert(info.is_has_unconfirm_tx() == info1.is_has_unconfirm_tx());
    }
}

TEST_F(test_tableindex, bucket_tree_3) {
    std::map<std::string, xaccount_index_t> accounts_info;
    xaccount_index_t info11{1};
    xaccount_index_t info12{2};
    accounts_info["account1"] = info11;
    accounts_info["account2"] = info12;
    std::map<std::string, xaccount_index_t> accounts_info2;
    xaccount_index_t info21{1};
    xaccount_index_t info22{2};
    accounts_info2["account1"] = info21;
    accounts_info2["account2"] = info22;
    std::map<std::string, xaccount_index_t> accounts_info3;
    xaccount_index_t info31{1};
    xaccount_index_t info32{2};
    accounts_info3["account3"] = info31;
    accounts_info3["account4"] = info32;

    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    tree->set_accounts_index_info(accounts_info);
    std::string root1 = tree->get_root_hash();
    ASSERT_EQ(root1.empty(), false);
    tree->set_accounts_index_info(accounts_info2);
    std::string root2 = tree->get_root_hash();
    ASSERT_EQ(root1, root2);
    tree->set_accounts_index_info(accounts_info3);
    std::string root3 = tree->get_root_hash();
    ASSERT_NE(root2, root3);
}

TEST_F(test_tableindex, bucket_tree_4) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;

    uint64_t count = 10000;
    std::map<std::string, xaccount_index_t> accounts_info = create_accounts_indexs(count);
    xassert(accounts_info.size() == count);
    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    tree->set_accounts_index_info(accounts_info);
    xassert(tree->get_bucket_size() == tree->get_max_bucket_size());

    base::xstream_t stream(base::xcontext_t::instance());
    xassert(tree->serialize_to(stream) > 0);
    std::cout << "mbt account count " << count << " selialize size = " << stream.size() << std::endl;
}

TEST_F(test_tableindex, bucket_tree_5) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;

    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();

        std::map<std::string, xaccount_index_t> accounts_info;
        accounts_info["account1"] = xaccount_index_t{1};
        accounts_info["account2"] = xaccount_index_t{1};
        tree->set_accounts_index_info(accounts_info);
        ASSERT_EQ(tree->get_account_size(), 2);
#ifdef DEBUG
    {
        const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & buckets = tree->get_buckets();
        for (auto & v : buckets) {
            const std::map<std::string, xaccount_index_t> & account_indexs = v.second->get_account_indexs();
            for (auto & account_index : account_indexs) {
                xdbg("JIMMY bucket_tree_5 height=1. account=%s,index=%s",
                    account_index.first.c_str(), account_index.second.dump().c_str());
            }
        }
    }
#endif
        std::map<std::string, xaccount_index_t> accounts_info2;
        accounts_info2["account3"] = xaccount_index_t{2};
        accounts_info2["account4"] = xaccount_index_t{2};
        accounts_info2["account5"] = xaccount_index_t{2};
        accounts_info2["account6"] = xaccount_index_t{2};
        accounts_info2["account7"] = xaccount_index_t{2};
        accounts_info2["account8"] = xaccount_index_t{2};
        tree->set_accounts_index_info(accounts_info2);
        ASSERT_EQ(tree->get_account_size(), 6+2);

#ifdef DEBUG
    {
        const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & buckets = tree->get_buckets();
        for (auto & v : buckets) {
            const std::map<std::string, xaccount_index_t> & account_indexs = v.second->get_account_indexs();
            for (auto & account_index : account_indexs) {
                xdbg("JIMMY bucket_tree_5 height=2. account=%s,index=%s",
                    account_index.first.c_str(), account_index.second.dump().c_str());
            }
        }
    }
#endif

        {
            std::map<std::string, xaccount_index_t> accounts_info3;
            accounts_info3["account9"] = xaccount_index_t{3};
            accounts_info3["account10"] = xaccount_index_t{3};
            accounts_info3["account11"] = xaccount_index_t{3};
            accounts_info3["account12"] = xaccount_index_t{3};
            accounts_info3["account13"] = xaccount_index_t{3};
            tree->set_accounts_index_info(accounts_info3);
            ASSERT_EQ(tree->get_account_size(), 6+2+5);
        }

#ifdef DEBUG
    {
        const std::map<uint16_t, xtable_mbt_bucket_node_ptr_t> & buckets = tree->get_buckets();
        for (auto & v : buckets) {
            const std::map<std::string, xaccount_index_t> & account_indexs = v.second->get_account_indexs();
            for (auto & account_index : account_indexs) {
                xdbg("JIMMY bucket_tree_5 height=3. account=%s,index=%s",
                    account_index.first.c_str(), account_index.second.dump().c_str());
            }
        }
    }
#endif
    {
        xaccount_index_t index;
        ASSERT_TRUE(tree->get_account_index("account1", index));
        ASSERT_EQ(1, index.get_latest_unit_height());
        ASSERT_TRUE(tree->get_account_index("account2", index));
        ASSERT_EQ(1, index.get_latest_unit_height());
    }
}

// TEST_F(test_tableindex, test_xvproperty) {
//     std::map<std::string, std::string> val;
//     val["111"] = "111";
//     val["222"] = "222";
//     base::xvalue_t xvalue{val};
//     std::map<std::string, std::string> val2 = xvalue.get_strmap();
//     val2["333"] = "333";
//     xassert(val.size() == val2.size());
//     xassert(val2["111"] == "111");
// }

TEST_F(test_tableindex, tableindex_block_para) {
    std::map<std::string, xaccount_index_t> accounts_info;
    xaccount_index_t info1{1};
    xaccount_index_t info2{2};
    accounts_info["account1"] = info1;
    accounts_info["account2"] = info2;
    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    xtable_mbt_binlog_ptr_t binlog = make_object_ptr<xtable_mbt_binlog_t>(accounts_info);
    xfulltable_block_para_t blockpara(tree, binlog);
    xtable_mbt_ptr_t tree1 = blockpara.get_new_state();
    std::string root1 = blockpara.get_new_state()->get_root_hash();

    std::map<std::string, xaccount_index_t> accounts_info2;
    xaccount_index_t info3{3};
    accounts_info2["account3"] = info3;
    xtable_mbt_binlog_ptr_t binlog2 = make_object_ptr<xtable_mbt_binlog_t>(accounts_info2);
    xfulltable_block_para_t blockpara2(tree1, binlog2);
    xtable_mbt_ptr_t tree2 = blockpara2.get_new_state();
    std::string root2 = blockpara2.get_new_state()->get_root_hash();
    ASSERT_NE(root1, root2);
}

TEST_F(test_tableindex, tableindex_block_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xauto_ptr<base::xvblock_t> genesis_block(xblocktool_t::create_genesis_empty_table(account));

    std::map<std::string, xaccount_index_t> accounts_info;
    xaccount_index_t info1{1};
    xaccount_index_t info2{2};
    accounts_info["account1"] = info1;
    accounts_info["account2"] = info2;
    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    xtable_mbt_binlog_ptr_t binlog = make_object_ptr<xtable_mbt_binlog_t>(accounts_info);
    xfulltable_block_para_t blockpara(tree, binlog);

    base::xauto_ptr<base::xvblock_t>  block1 = xfull_tableblock_t::create_next_block(blockpara, genesis_block.get());
    ASSERT_EQ(block1->get_account(), account);
    ASSERT_EQ(block1->get_height(), 1);
    ASSERT_EQ(block1->get_block_class(), base::enum_xvblock_class_full);
    ASSERT_EQ(block1->get_block_type(), base::enum_xvblock_type_general);
    ASSERT_EQ(block1->get_header()->get_block_level(), base::enum_xvblock_level_table);

    xfull_tableblock_t* ftable1 = dynamic_cast<xfull_tableblock_t*>(block1.get());
    ASSERT_EQ(ftable1->get_offdata_hash(), blockpara.get_new_state_root());
    ASSERT_EQ(ftable1->get_fulltable_binlog_resource()->get_binlog()->get_accounts_index().size(), accounts_info.size());
}

TEST_F(test_tableindex, tableindex_block_2_size) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xauto_ptr<base::xvblock_t> genesis_block(xblocktool_t::create_genesis_empty_table(account));
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;

    uint64_t count = 10000;
    std::map<std::string, xaccount_index_t> accounts_info = create_accounts_indexs(count);

    xtable_mbt_ptr_t tree = make_object_ptr<xtable_mbt_t>();
    xtable_mbt_binlog_ptr_t binlog = make_object_ptr<xtable_mbt_binlog_t>(accounts_info);
    xfulltable_block_para_t blockpara(tree, binlog);

    base::xauto_ptr<base::xvblock_t>  block1 = xfull_tableblock_t::create_next_block(blockpara, genesis_block.get());
    {
        base::xstream_t header_stream(base::xcontext_t::instance());
        xassert(block1->serialize_to(header_stream) > 0);
        xassert(block1->get_input()->get_resources_hash().empty());
        xassert(block1->get_output()->get_resources_hash().empty());
        std::cout << "full tableblock 10000 account index size = " << header_stream.size() << std::endl;
    }
}

TEST_F(test_tableindex, new_xbase_serialize) {
    {
        uint64_t    m_data1 = 1;
        top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
        _test_stream.write_compact_var(m_data1);
        ASSERT_EQ(_test_stream.size(), 1);
    }
    {
        uint64_t    m_data1 = 1;
        top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
        _test_stream << m_data1;
        ASSERT_EQ(_test_stream.size(), 8);
    }
    {
        uint64_t    m_data1 = 0x10000000;
        top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
        _test_stream.write_compact_var(m_data1);
        std::cout << "size=" << _test_stream.size() << std::endl;
    }
    {
        std::string    m_data1 = "00000001";
        top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
        _test_stream.write_compact_var(m_data1);
        std::cout << "size=" << _test_stream.size() << std::endl;
    }
    {
        std::string    m_data1 = "00000001";
        top::base::xautostream_t<1024> _test_stream(top::base::xcontext_t::instance());
        _test_stream << m_data1;
        std::cout << "size=" << _test_stream.size() << std::endl;
    }
}

