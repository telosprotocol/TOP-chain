#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xblock.h"
#include "xdata/xblockchain.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction_maker.hpp"
#include "xutility/xhash.h"
#include "xbase/xutl.h"
#include "xbase/xmem.h"

using namespace top;
using namespace top::data;

class test_block : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

class xtest_block_transaction_t : public xbase_dataunit_t<xtest_block_transaction_t, 200> {
  public:
    virtual int32_t do_write(base::xstream_t & stream) {
        const int32_t begin_size = stream.size();
        stream << balance;
        stream << balance2;
        const int32_t end_size = stream.size();
        std::cout << "xtest_block_transaction_t do_write size " << end_size - begin_size << std::endl;
        return end_size - begin_size;
    }
    virtual int32_t do_read(base::xstream_t & stream) {
        const int32_t begin_size = stream.size();
        stream >> balance;
        stream >> balance2;
        const int32_t end_size = stream.size();
        std::cout << "xtest_block_transaction_t do_read size " << begin_size - end_size << std::endl;
        return begin_size - end_size;
    }
    int64_t balance;
    int64_t balance2;
};

REG_CLS(xtest_block_transaction_t);

using xtest_block_transaction_ptr_t = xobject_ptr_t<xtest_block_transaction_t>;

TEST_F(test_block, xtest_block_transaction) {
    xtest_block_transaction_ptr_t transaction = make_object_ptr<xtest_block_transaction_t>();
    transaction->balance = 100;

    base::xstream_t stream(base::xcontext_t::instance());
    transaction->serialize_to(stream);

    xtest_block_transaction_ptr_t transaction2 = make_object_ptr<xtest_block_transaction_t>();
    transaction2->serialize_from(stream);
    ASSERT_EQ(transaction2->balance, 100);

}

TEST_F(test_block, xtest_block_with_body) {
    // xtest_block_transaction_ptr_t transaction1 = make_object_ptr<xtest_block_transaction_t>();
    // transaction1->balance = 100;
    // transaction1->balance2 = 1000;

    // xblock_ptr_t block = make_object_ptr<xblock_t>();
    // block->add_transaction(transaction1);
    // block->set_prev_height(1);
    // block->calc_block_hash();

    // base::xstream_t stream(base::xcontext_t::instance());
    // block->serialize_to(stream);

    // xblock_ptr_t block2 = make_object_ptr<xblock_t>();
    // block2->serialize_from(stream);
    // ASSERT_EQ(block2->get_prev_height(), 1);

    // xtest_block_transaction_ptr_t r_transaction1 = block2->get_one_transaction<xtest_block_transaction_t>();
    // ASSERT_EQ(r_transaction1->balance, 100);
    // ASSERT_EQ(r_transaction1->balance2, 1000);
}

TEST_F(test_block, block_merkle_1) {
    // std::string address = "aaaa";
    // xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
    // xblock_ptr_t block = account->create_new_block();

    // xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();

    // std::string temp = "hello" + std::to_string(1);
    // xtransaction_ptr_t tx = xtransaction_maker::make_transfer_tx(account, temp, 100, 0, 0, 0);
    // tx->set_tx_subtype(enum_transaction_subtype_send);
    // input->add_tx(false, tx);
    // block->add_transaction(input);
    // block->calc_block_hash();

    // xinput_transaction_info_t tx_info;
    // bool find = input->get_tx_info(tx->digest(), tx_info);
    // ASSERT_TRUE(find);
    // std::string tx_info_leaf_str = tx_info.get_merkle_leaf();

    // xmerkle_path_256_t hash_path;
    // bool ret = block->calc_merkle_path(tx_info_leaf_str, hash_path);
    // ASSERT_TRUE(ret);
    // ASSERT_EQ(hash_path.size(), 0);

    // base::xstream_t stream(base::xcontext_t::instance());
    // hash_path.serialize_to(stream);
    // std::cout << "hash_path level_size:" << hash_path.size() << std::endl;
    // std::cout << "hash_path serialize_size:" << stream.size() << std::endl;

    // ret = block->get_block_header()->validate_merkle_path(tx_info_leaf_str, hash_path);
    // ASSERT_TRUE(ret);

    // {
    //     std::string temp = "hello" + std::to_string(2);
    //     uint256_t hash = utl::xsha2_256_t::digest(temp.c_str(), temp.size());
    //     std::string hash_str = std::string((const char*)hash.data(), hash.size());
    //     ret = block->get_block_header()->validate_merkle_path(hash_str, hash_path);
    //     ASSERT_FALSE(ret);
    // }
}

TEST_F(test_block, block_merkle_2) {
    // std::string address = "aaaa";
    // xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
    // xblock_ptr_t block = account->create_new_block();

    // xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();
    // size_t count = 1024;
    // std::string leaf[1024];
    // std::string to = "T00000xxxxxxxx";
    // for (size_t i=0; i < count; i++) {
    //     std::string temp = "hello" + std::to_string(i);
    //     xtransaction_ptr_t tx = xtransaction_maker::make_transfer_tx(account, temp, 100, 0, 0, 0);
    //     tx->set_tx_subtype(enum_transaction_subtype_send);
    //     input->add_tx(false, tx);

    //     xinput_transaction_info_t tx_info;
    //     bool find = input->get_tx_info(tx->digest(), tx_info);
    //     ASSERT_TRUE(find);
    //     leaf[i] = tx_info.get_merkle_leaf();
    // }
    // block->add_transaction(input);

    // {
    // int64_t time_begin = base::xtime_utl::time_now_ms();
    // block->calc_block_hash();
    // int64_t time_end = base::xtime_utl::time_now_ms();
    // std::cout << "merkle calc time: " << time_end - time_begin << std::endl;
    // }

    // {
    //     int64_t time_begin = base::xtime_utl::time_now_ms();
    //     for (size_t i=0; i < count; i++) {
    //         xmerkle_path_256_t hash_path;
    //         bool ret = block->calc_merkle_path(leaf[i], hash_path);
    //         ASSERT_TRUE(ret);
    //         ASSERT_NE(hash_path.size(), 0);
    //         if (i < 5) {
    //             base::xstream_t stream(base::xcontext_t::instance());
    //             hash_path.serialize_to(stream);
    //             std::cout << "hash_path level_size:" << hash_path.size() << std::endl;
    //             std::cout << "hash_path serialize_size:" << stream.size() << std::endl;
    //         }
    //         ret = block->get_block_header()->validate_merkle_path(leaf[i], hash_path);
    //         ASSERT_TRUE(ret);
    //     }
    //     int64_t time_end = base::xtime_utl::time_now_ms();
    //     std::cout << "merkle verify total time: " << (time_end - time_begin) << std::endl;
    //     std::cout << "merkle verify ave time: " << (time_end - time_begin)/count << std::endl;
    // }
}
