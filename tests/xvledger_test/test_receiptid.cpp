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
        xassert(pair2.get_confirmid_max() == 2);
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
        xassert(pair3.get_confirmid_max() == 8);
        xassert(pair3.get_recvid_max() == 3);
    }
    {
        xtable_shortid_t sid{2};
        xreceiptid_pair_t pair{10, 8, 5};
        receiptid_state->add_pair(sid, pair);

        xreceiptid_pair_t pair3;
        xassert(receiptid_state->find_pair(sid, pair3));
        xassert(pair3.get_sendid_max() == 10);
        xassert(pair3.get_confirmid_max() == 8);
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
    xassert(pair3.get_sendid_max() == 5);
    xassert(pair3.get_confirmid_max() == 2);
    xassert(pair3.get_recvid_max() == 1);

    std::string root2 = receiptid_state->build_root_hash(enum_xhash_type_sha2_256);
    xassert(!root2.empty());
    xassert(root1 != root2);
}

TEST_F(test_receiptid, receiptid_pairs_1) {
    base::xreceiptid_pairs_ptr_t pairs = make_object_ptr<base::xreceiptid_pairs_t>();
    base::xtable_shortid_t tableid = 100;

    {
        uint64_t sendid = 200;
        pairs->set_sendid_max(tableid, sendid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 200);
        xassert(pair.get_recvid_max() == 0);
        xassert(pair.get_confirmid_max() == 0);
    }
    {
        uint64_t sendid = 190;
        pairs->set_sendid_max(tableid, sendid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 200);
        xassert(pair.get_recvid_max() == 0);
        xassert(pair.get_confirmid_max() == 0);
    }
    {
        uint64_t sendid = 220;
        pairs->set_sendid_max(tableid, sendid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 220);
        xassert(pair.get_recvid_max() == 0);
        xassert(pair.get_confirmid_max() == 0);
    }
    {
        uint64_t recvid = 1000;
        pairs->set_recvid_max(tableid, recvid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 220);
        xassert(pair.get_recvid_max() == 1000);
        xassert(pair.get_confirmid_max() == 0);
    }
    {
        uint64_t confirmid = 150;
        pairs->set_confirmid_max(tableid, confirmid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 220);
        xassert(pair.get_recvid_max() == 1000);
        xassert(pair.get_confirmid_max() == 150);
    }
}

TEST_F(test_receiptid, receiptid_pairs_2) {
    base::xreceiptid_pairs_ptr_t pairs = make_object_ptr<base::xreceiptid_pairs_t>();
    {
        base::xtable_shortid_t tableid = 100;
        uint64_t sendid = 220;
        pairs->set_sendid_max(tableid, sendid);
        uint64_t recvid = 1000;
        pairs->set_recvid_max(tableid, recvid);
        uint64_t confirmid = 150;
        pairs->set_confirmid_max(tableid, confirmid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 220);
        xassert(pair.get_recvid_max() == 1000);
        xassert(pair.get_confirmid_max() == 150);
    }
    {
        base::xtable_shortid_t tableid = 200;
        uint64_t sendid = 2200;
        pairs->set_sendid_max(tableid, sendid);
        uint64_t recvid = 10000;
        pairs->set_recvid_max(tableid, recvid);
        uint64_t confirmid = 1500;
        pairs->set_confirmid_max(tableid, confirmid);
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 2200);
        xassert(pair.get_recvid_max() == 10000);
        xassert(pair.get_confirmid_max() == 1500);
    }
}


TEST_F(test_receiptid, receiptid_pairs_3) {
    base::xreceiptid_pairs_ptr_t pairs = make_object_ptr<base::xreceiptid_pairs_t>();

    base::xreceiptid_pairs_ptr_t pairs_binlog1 = make_object_ptr<base::xreceiptid_pairs_t>();
    {
        base::xtable_shortid_t tableid = 100;
        uint64_t sendid = 220;
        pairs_binlog1->set_sendid_max(tableid, sendid);
        uint64_t recvid = 1000;
        pairs_binlog1->set_recvid_max(tableid, recvid);
        uint64_t confirmid = 150;
        pairs_binlog1->set_confirmid_max(tableid, confirmid);
    }

    pairs->add_binlog(pairs_binlog1);

    base::xreceiptid_pairs_ptr_t pairs_binlog2 = make_object_ptr<base::xreceiptid_pairs_t>();
    {
        base::xtable_shortid_t tableid = 100;
        uint64_t sendid = 220;
        pairs_binlog2->set_sendid_max(tableid, sendid);
        uint64_t recvid = 800;
        pairs_binlog2->set_recvid_max(tableid, recvid);
        uint64_t confirmid = 160;
        pairs_binlog2->set_confirmid_max(tableid, confirmid);
    }
    pairs->add_binlog(pairs_binlog2);
    {
        base::xtable_shortid_t tableid = 100;
        xreceiptid_pair_t pair;
        xassert(pairs->find_pair(tableid, pair));
        xassert(pair.get_sendid_max() == 220);
        xassert(pair.get_recvid_max() == 1000);
        xassert(pair.get_confirmid_max() == 160);
    }
}

TEST_F(test_receiptid, receiptid_check_1) {
    xreceiptid_state_ptr_t receiptid_state = make_object_ptr<xreceiptid_state_t>();

    xtable_shortid_t sid{1};
    xreceiptid_pair_t pair{5, 1, 2};
    receiptid_state->add_pair(sid, pair);

    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 8);
        receiptid_check.set_sendid(sid, 7);
        xassert(false == receiptid_check.check_contious(receiptid_state));
    }
    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 6);
        receiptid_check.set_recvid(sid, 4);
        xassert(false == receiptid_check.check_contious(receiptid_state));
    }
    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 6);
        receiptid_check.set_recvid(sid, 3);
        receiptid_check.set_confirmid(sid, 3);
        xassert(false == receiptid_check.check_contious(receiptid_state));
    }
    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 8);
        receiptid_check.set_sendid(sid, 7);
        receiptid_check.set_sendid(sid, 6);
        receiptid_check.set_recvid(sid, 4);
        receiptid_check.set_recvid(sid, 3);
        receiptid_check.set_confirmid(sid, 3);
        receiptid_check.set_confirmid(sid, 2);
        xassert(true == receiptid_check.check_contious(receiptid_state));
    }
}

TEST_F(test_receiptid, receiptid_check_2) {
    xreceiptid_state_ptr_t receiptid_state = make_object_ptr<xreceiptid_state_t>();

    xtable_shortid_t sid{1};
    xreceiptid_pair_t pair{5, 1, 2};
    receiptid_state->add_pair(sid, pair);
    std::cout << "binlog=" << receiptid_state->get_binlog()->dump() << std::endl;

    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 8);
        receiptid_check.set_sendid(sid, 7);
        receiptid_check.set_sendid(sid, 6);
        receiptid_check.set_confirmid(sid, 3);
        receiptid_check.set_confirmid(sid, 2);
        receiptid_check.set_recvid(sid, 4);
        receiptid_check.set_recvid(sid, 3);
        xassert(true == receiptid_check.check_contious(receiptid_state));
        receiptid_check.update_state(receiptid_state);
        xreceiptid_pair_t pair2;
        xassert(receiptid_state->find_pair(sid, pair2));
        xassert(pair2.get_sendid_max() == 8);
        xassert(pair2.get_confirmid_max() == 3);
        xassert(pair2.get_recvid_max() == 4);
        std::cout << "binlog=" << receiptid_state->get_binlog()->dump() << std::endl;
    }
    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 10);
        receiptid_check.set_sendid(sid, 9);
        receiptid_check.set_confirmid(sid, 5);
        receiptid_check.set_confirmid(sid, 4);
        receiptid_check.set_recvid(sid, 6);
        receiptid_check.set_recvid(sid, 5);
        xassert(true == receiptid_check.check_contious(receiptid_state));
        receiptid_check.update_state(receiptid_state);
        xreceiptid_pair_t pair2;
        xassert(receiptid_state->find_pair(sid, pair2));
        xassert(pair2.get_sendid_max() == 10);
        xassert(pair2.get_confirmid_max() == 5);
        xassert(pair2.get_recvid_max() == 6);
        std::cout << "binlog=" << receiptid_state->get_binlog()->dump() << std::endl;
    }
    {
        receiptid_state->merge_new_full();
        std::cout << "full_state=" << receiptid_state->get_last_full_state()->dump() << std::endl;
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 12);
        receiptid_check.set_sendid(sid, 11);
        receiptid_check.set_confirmid(sid, 7);
        receiptid_check.set_confirmid(sid, 6);
        receiptid_check.set_recvid(sid, 8);
        receiptid_check.set_recvid(sid, 7);
        xassert(true == receiptid_check.check_contious(receiptid_state));
        receiptid_check.update_state(receiptid_state);
        xreceiptid_pair_t pair2;
        xassert(receiptid_state->find_pair(sid, pair2));
        xassert(pair2.get_sendid_max() == 12);
        xassert(pair2.get_confirmid_max() == 7);
        xassert(pair2.get_recvid_max() == 8);
        std::cout << "binlog=" << receiptid_state->get_binlog()->dump() << std::endl;
    }
}

TEST_F(test_receiptid, receiptid_check_3) {
    xreceiptid_state_ptr_t receiptid_state = make_object_ptr<xreceiptid_state_t>();

    xtable_shortid_t sid{1};
    xreceiptid_pair_t pair{5, 1, 2};
    receiptid_state->add_pair(sid, pair);

    {
        xreceiptid_check_t receiptid_check;
        receiptid_check.set_sendid(sid, 8);
        receiptid_check.set_sendid(sid, 7);
        receiptid_check.set_sendid(sid, 6);
        receiptid_check.set_confirmid(sid, 3);
        receiptid_check.set_confirmid(sid, 2);
        receiptid_check.set_recvid(sid, 4);
        receiptid_check.set_recvid(sid, 3);
        xassert(true == receiptid_check.check_contious(receiptid_state));
        std::cout << "receiptid_check=" << receiptid_check.dump() << std::endl;
    }
}
