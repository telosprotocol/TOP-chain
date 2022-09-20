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
    base::xaccount_index_t index1{1, std::to_string(1), std::to_string(1), 1, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index2{2, std::to_string(2), std::to_string(2), 2, base::enum_xvblock_class_light, base::enum_xvblock_type_general};
    base::xaccount_index_t index3{3, std::to_string(3), std::to_string(3), 3, base::enum_xvblock_class_light, base::enum_xvblock_type_general};

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