#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xvledger/xaccountindex.h"

using namespace top;
using namespace top::base;

class test_accountindex : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_accountindex, write_and_read_2)
{
    base::xaccount_index_t index1{base::enum_xaccountindex_version_state_hash, 1, std::to_string(1), std::to_string(1), 1};
    base::xaccount_index_t index2{base::enum_xaccountindex_version_state_hash, 2, std::to_string(2), std::to_string(2), 2};
    base::xaccount_index_t index3{base::enum_xaccountindex_version_state_hash, 3, std::to_string(3), std::to_string(3), 3};

    base::xaccount_indexs_t  indexs;
    indexs.add_account_index("aaa", index1);
    indexs.add_account_index("bbb", index2);
    indexs.add_account_index("ccc", index3);
    ASSERT_EQ(indexs.get_account_indexs().size(), 3);

    std::string serialize_bin;
    int ret = indexs.serialize_to_string(serialize_bin);
    ASSERT_TRUE(ret > 0);
    std::cout << "serialize_bin " << serialize_bin.size() << std::endl;
    base::xaccount_indexs_t  indexs2;
    int ret2 = indexs2.serialize_from_string(serialize_bin);
    ASSERT_EQ(ret, ret2);
    ASSERT_EQ(indexs2.get_account_indexs().size(), indexs.get_account_indexs().size());
    ASSERT_EQ(indexs2.get_account_indexs()[0].first, "aaa");
    ASSERT_EQ(indexs2.get_account_indexs()[0].second.get_latest_unit_height(), 1);
    ASSERT_EQ(indexs2.get_account_indexs()[1].first, "bbb");
    ASSERT_EQ(indexs2.get_account_indexs()[1].second.get_latest_unit_height(), 2);
    ASSERT_EQ(indexs2.get_account_indexs()[2].first, "ccc");
    ASSERT_EQ(indexs2.get_account_indexs()[2].second.get_latest_unit_height(), 3);    
}

TEST_F(test_accountindex, old_and_new_version)
{
    base::xaccount_index_t index_old(1, 2, 3, enum_xblock_consensus_flag_committed, base::enum_xvblock_class_light, base::enum_xvblock_type_general, false, false);
    std::string old_version_str;
    index_old.old_serialize_to(old_version_str);
    std::cout << "old_version_str size:" << old_version_str.size() << std::endl;

    base::xaccount_index_t index_old_tmp;
    index_old_tmp.old_serialize_from(old_version_str);

    ASSERT_EQ(index_old_tmp.get_latest_tx_nonce(), index_old_tmp.get_latest_tx_nonce());
    ASSERT_EQ(index_old_tmp.get_latest_unit_height(), index_old_tmp.get_latest_unit_height());
    ASSERT_EQ(index_old_tmp.get_latest_unit_viewid(), index_old_tmp.get_latest_unit_viewid());
    ASSERT_EQ(index_old_tmp.get_latest_unit_hash(), index_old_tmp.get_latest_unit_hash());
    ASSERT_EQ(index_old_tmp.get_latest_state_hash(), index_old_tmp.get_latest_state_hash());

    base::xaccount_index_t index_new{base::enum_xaccountindex_version_state_hash, 1, std::to_string(1), std::to_string(1), 1};
    std::string new_version_str;
    index_new.serialize_to(new_version_str);
    std::cout << "new_version_str size:" << new_version_str.size() << std::endl;

    base::xaccount_index_t index_new_tmp;
    index_new_tmp.serialize_from(new_version_str);

    ASSERT_EQ(index_new_tmp.get_latest_tx_nonce(), index_new.get_latest_tx_nonce());
    ASSERT_EQ(index_new_tmp.get_latest_unit_height(), index_new.get_latest_unit_height());
    ASSERT_EQ(index_new_tmp.get_latest_unit_viewid(), index_new.get_latest_unit_viewid());
    ASSERT_EQ(index_new_tmp.get_latest_unit_hash(), index_new.get_latest_unit_hash());
    ASSERT_EQ(index_new_tmp.get_latest_state_hash(), index_new.get_latest_state_hash());
}