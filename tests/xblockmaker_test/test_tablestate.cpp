#include "gtest/gtest.h"
#include "xblockmaker/xunit_maker.h"
#include "xblockmaker/xunit_builder.h"
#include "test_common.hpp"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "xdata/xtablestate.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"

using namespace top::data;
using namespace top::blockmaker;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_tablestate : public testing::Test {
 protected:
    void SetUp() override {
        base::xvblock_t::register_object(base::xcontext_t::instance());
    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_tablestate, tablestate_1) {
    xaccount_index_t info1{1};
    xaccount_index_t info2;
    xtablestate_ptr_t tablestate = make_object_ptr<xtablestate_t>();
    tablestate->set_account_index("aaa", info1);
    tablestate->merge_new_full();
    xassert(true == tablestate->get_account_index("aaa", info2));
    std::string root1 = tablestate->build_root_hash();
    std::string full_str = tablestate->serialize_to_full_data_string();

    std::string binlog_str;
    xtablestate_ptr_t tablestate2 = make_object_ptr<xtablestate_t>(full_str, 10, binlog_str, 10);
    xassert(true == tablestate2->get_account_index("aaa", info2));
    std::string root2 = tablestate2->build_root_hash();
    xassert(root1 == root2);
}

TEST_F(test_tablestate, tablestate_2) {
    xaccount_index_t info1{1};
    xaccount_index_t info2;
    xtablestate_ptr_t tablestate = make_object_ptr<xtablestate_t>();
    tablestate->set_account_index("aaa", info1);
    tablestate->merge_new_full();
    std::string root1 = tablestate->build_root_hash();
    tablestate->set_account_index("bbb", info1);

    std::string full_str = tablestate->serialize_to_full_data_string();
    std::string binlog_str = tablestate->serialize_to_binlog_data_string();

    xtablestate_ptr_t tablestate2 = make_object_ptr<xtablestate_t>(full_str, 10, binlog_str, 20);
    xassert(true == tablestate2->get_account_index("aaa", info2));
    xassert(true == tablestate2->get_account_index("bbb", info2));
    std::string root2 = tablestate2->build_root_hash();
    xassert(root1 == root2);
}
