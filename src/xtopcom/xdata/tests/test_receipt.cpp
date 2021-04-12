#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xblock.h"
#include "xdata/xblockchain.h"
#include "xdata/xdata_common.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction.h"
#include "xutility/xhash.h"
#include "xbase/xutl.h"

using namespace top;
using namespace top::data;

class test_receipt : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// TEST_F(test_receipt, recv_receipt_1) {
//     std::string address = "aaaa";
//     xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
//     xblock_ptr_t block = account->create_new_block();

//     xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();

//     std::string temp = "hello" + std::to_string(1);
//     uint256_t hash = utl::xsha2_256_t::digest(temp.c_str(), temp.size());
//     input->add_send_to_self_tx("", hash);
//     block->add_transaction(input);
//     block->calc_block_hash();

//     xreceipt_ptr_t receipt = make_object_ptr<xreceipt_t>();
//     auto ret = receipt->add_receipt_tx(hash, block);
//     ASSERT_TRUE(ret);
//     ASSERT_EQ(0, receipt->verify());

//     // modify receipt and make verify fail
//     xblock_header_ptr_t header = receipt->get_block_header();
//     uint256_t root_ori = header->get_event_root();
//     std::string temp2 = "hello" + std::to_string(2);
//     uint256_t root = utl::xsha2_256_t::digest(temp2.c_str(), temp2.size());
//     header->set_event_count_and_root(10, root);
//     uint256_t root_2 = header->get_event_root();
//     ASSERT_NE(root_ori, root_2);
//     ASSERT_NE(0, receipt->verify());
// }

// TEST_F(test_receipt, recv_receipt_2) {
//     std::string address = "aaaa";
//     xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
//     xblock_ptr_t block = account->create_new_block();

//     xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();

//     std::string temp = "hello" + std::to_string(1);
//     uint256_t hash = utl::xsha2_256_t::digest(temp.c_str(), temp.size());
//     input->add_send_to_self_tx("", hash);
//     block->add_transaction(input);
//     block->calc_block_hash();

//     xreceipt_ptr_t receipt = make_object_ptr<xreceipt_t>();
//     auto ret = receipt->add_receipt_tx(hash, block);
//     ASSERT_TRUE(ret);
//     ASSERT_EQ(0, receipt->verify());

//     base::xstream_t stream(base::xcontext_t::instance());
//     receipt->serialize_to(stream);

//     xreceipt_ptr_t receipt2 = make_object_ptr<xreceipt_t>();
//     receipt2->serialize_from(stream);
//     ASSERT_EQ(0, receipt->verify());
// }

// TEST_F(test_receipt, send_receipt_1) {
//     std::string address = "aaaa";
//     xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
//     xblock_ptr_t block = account->create_new_block();

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//     data::xproperty_asset asset_out{100};
//     tx->make_tx_transfer(asset_out);
//     tx->set_different_source_target_address("aaaa", "bbbb");
//     tx->set_digest();

//     xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();
//     input->add_send_to_self_tx(tx->digest());
//     block->add_transaction(input);
//     block->calc_block_hash();

//     xreceipt_ptr_t receipt = make_object_ptr<xreceipt_t>();
//     auto ret = receipt->add_receipt_tx(tx, block);
//     ASSERT_TRUE(ret);
//     ASSERT_EQ(0, receipt->verify());

//     // modify receipt and make verify fail
//     xblock_header_ptr_t header = receipt->get_block_header();
//     uint256_t root_ori = header->get_event_root();
//     std::string temp2 = "hello" + std::to_string(2);
//     uint256_t root = utl::xsha2_256_t::digest(temp2.c_str(), temp2.size());
//     header->set_event_count_and_root(10, root);
//     uint256_t root_2 = header->get_event_root();
//     ASSERT_NE(root_ori, root_2);
//     ASSERT_NE(0, receipt->verify());
// }

// TEST_F(test_receipt, send_receipt_2) {
//     std::string address = "aaaa";
//     xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
//     xblock_ptr_t block = account->create_new_block();

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//     data::xproperty_asset asset_out{100};
//     tx->make_tx_transfer(asset_out);
//     tx->set_different_source_target_address("aaaa", "bbbb");
//     tx->set_digest();

//     xlightunit_input_ptr_t input = make_object_ptr<xlightunit_input_t>();
//     input->add_send_to_self_tx(tx->digest());
//     block->add_transaction(input);
//     block->calc_block_hash();

//     xreceipt_ptr_t receipt = make_object_ptr<xreceipt_t>();
//     auto ret = receipt->add_receipt_tx(tx, block);
//     ASSERT_TRUE(ret);
//     ASSERT_EQ(0, receipt->verify());

//     base::xstream_t stream(base::xcontext_t::instance());
//     receipt->serialize_to(stream);

//     xreceipt_ptr_t receipt2 = make_object_ptr<xreceipt_t>();
//     receipt2->serialize_from(stream);
//     ASSERT_EQ(0, receipt->verify());
// }
