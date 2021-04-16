#include "gtest/gtest.h"
#include "xvledger/xreceiptid.h"

using namespace top;
using namespace top::base;

class test_receiptid : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_receiptid, receiptid_state_1) {
    xreceiptid_state_ptr_t receiptid_state = make_object_ptr<xreceiptid_state_t>();

    {
        xtable_shortid_t sid{1};
        xreceiptid_pair_t pair{5, 2, 1};
        receiptid_state->add_pair(sid, pair);

        xreceiptid_pair_t pair2;
        xassert(receiptid_state->find_pair(sid, pair2));
        xassert(pair2.get_sendid_max() == 5);
        xassert(pair2.get_sendid_min() == 5-2);
        xassert(pair2.get_recvid_max() == 1);

        xtable_shortid_t sid2{2};
        xassert(false == receiptid_state->find_pair(sid2, pair2));
    }
    {
        xtable_shortid_t sid{1};
        xreceiptid_pair_t pair{8, 8, 3};
        receiptid_state->add_pair(sid, pair);

        xreceiptid_pair_t pair3;
        xassert(receiptid_state->find_pair(sid, pair3));
        xassert(pair3.get_sendid_max() == 8);
        xassert(pair3.get_sendid_min() == 8-8);
        xassert(pair3.get_recvid_max() == 3);
    }
    {
        xtable_shortid_t sid{2};
        xreceiptid_pair_t pair{10, 8, 5};
        receiptid_state->add_pair(sid, pair);

        xreceiptid_pair_t pair3;
        xassert(receiptid_state->find_pair(sid, pair3));
        xassert(pair3.get_sendid_max() == 10);
        xassert(pair3.get_sendid_min() == 10-8);
        xassert(pair3.get_recvid_max() == 5);

        xassert(receiptid_state->get_full_size() == 0);
        xassert(receiptid_state->get_binlog_size() == 2);
    }
}

TEST_F(test_receiptid, receiptid_state_2) {
    xreceiptid_state_ptr_t receiptid_state = make_object_ptr<xreceiptid_state_t>();
    std::string root1 = receiptid_state->build_root_hash(enum_xhash_type_sha2_256);

    xtable_shortid_t sid{1};
    xreceiptid_pair_t pair{5, 2, 1};
    receiptid_state->add_pair(sid, pair);
    xassert(receiptid_state->get_full_size() == 0);
    xassert(receiptid_state->get_binlog_size() == 1);

    receiptid_state->merge_new_full();
    xassert(receiptid_state->get_full_size() == 1);
    xassert(receiptid_state->get_binlog_size() == 0);

    xreceiptid_pair_t pair3;
    xassert(receiptid_state->find_pair(sid, pair3));
    std::string root2 = receiptid_state->build_root_hash(enum_xhash_type_sha2_256);
    xassert(!root2.empty());
    xassert(root1 != root2);
}
