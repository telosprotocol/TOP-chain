#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_face.h"
#include "xtxpool_v2/xtx_account_filter.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top::xtxpool_v2;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

class test_account_recv_filter : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {}
public:
    std::vector<xcons_transaction_ptr_t> get_tx(base::xvblockstore_t *blockstore, xstore_face_t *store, std::string sender,
                        std::string receiver, std::vector<xcons_transaction_ptr_t> txs, xblock_t **block){
        *block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, store, sender, txs);
        data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(*block);
        std::vector<xcons_transaction_ptr_t> receipts1;
        std::vector<xcons_transaction_ptr_t> receipts2;
        lightunit->create_txreceipts(receipts1, receipts2);

        return receipts1;
    }
    // xobject_ptr_t<xstore_face_t> m_blockdb;
    // xobject_ptr_t<base::xvblockstore_t> m_blockstore;
};

TEST_F(test_account_recv_filter, recvtx_has_been_committed) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //insert committed txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 1);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> recvtxs = get_tx(blockstore, xstore, sender, receiver, txs, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, xstore, receiver, recvtxs);

    //construct sendtx filter
    xaccount_recvtx_filter_ptr_t filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], deny);
        ASSERT_EQ(deny, true);
        ASSERT_EQ(ret, xtxpool_success);
    }
    creator.clean_all();
}

TEST_F(test_account_recv_filter, recvtx_has_not_been_committed_and_blockstore_only_exist_genesis_block) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //construct txs, commit txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> recvtxs = get_tx(blockstore, xstore, sender, receiver, txs, &block);

    test_xtxpool_util_t::create_genesis_account(xstore, receiver);

    //construct sendtx filter
    xaccount_recvtx_filter_ptr_t filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }

    filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], 0, deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }
    creator.clean_all();
}

TEST_F(test_account_recv_filter, recvtx_has_not_been_committed_and_blockstore_exist_some_blocks) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //construct txs, commit txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 2);
    std::vector<xcons_transaction_ptr_t> txs1;
    txs1.push_back(txs[0]);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> recvtxs = get_tx(blockstore, xstore, sender, receiver, txs1, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, xstore, receiver, recvtxs);

    std::vector<xcons_transaction_ptr_t> txs2;
    txs2.push_back(txs[1]);
     xblock_t *block1;
    recvtxs = get_tx(blockstore, xstore, sender, receiver, txs2, &block1);

    //construct sendtx filter
    xaccount_recvtx_filter_ptr_t filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }

    filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], block->get_height(), deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }
}

TEST_F(test_account_recv_filter, lack_some_block_in_the_blockstore) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //construct txs, commit txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 3);
    std::vector<xcons_transaction_ptr_t> txs1;
    txs1.push_back(txs[0]);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> recvtxs = get_tx(blockstore, xstore, sender, receiver, txs1, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, xstore, receiver, recvtxs);

    // construct not exist block
    std::vector<xcons_transaction_ptr_t> txs2;
    txs2.push_back(txs[1]);
    xlightunit_block_para_t para;
    para.set_input_txs(txs2);
    base::xvblock_t *not_exist_block = test_blocktuil::create_next_lightunit_with_consensus(para, block);

    //construct sendtx filter
    xaccount_recvtx_filter_ptr_t filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2; i++){
        enum_xtxpool_error_type ret = filter->update_reject_rule(dynamic_cast<data::xblock_t *>(not_exist_block));
        ASSERT_EQ(ret, xtxpool_success);
    }

    std::vector<xcons_transaction_ptr_t> txs3;
    txs3.push_back(txs[2]);
    recvtxs = get_tx(blockstore, xstore, sender, receiver, txs3, &block);
    filter = make_object_ptr<xaccount_recvtx_filter>(receiver, blockstore);
    for (uint32_t i = 0; i < 2; i++){
        enum_xtxpool_error_type ret = filter->reject(recvtxs[0], block->get_height() + 1, deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_error_unitblock_lack);
    }
}


class test_account_confirmtx_filter : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
public:
    // xobject_ptr_t<xstore_face_t> m_blockdb;
    // xobject_ptr_t<base::xvblockstore_t> m_blockstore;

    std::vector<xcons_transaction_ptr_t> get_tx(base::xvblockstore_t *blockstore, xstore_face_t *store, std::string sender,
                    std::string receiver, std::vector<xcons_transaction_ptr_t> txs, xblock_t **block){
        *block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, store, sender, txs);
        data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(*block);
        std::vector<xcons_transaction_ptr_t> receipts1;
        std::vector<xcons_transaction_ptr_t> receipts2;
        lightunit->create_txreceipts(receipts1, receipts2);

        *block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, store, receiver, receipts1);
        lightunit = dynamic_cast<data::xlightunit_block_t *>(*block);
        std::vector<xcons_transaction_ptr_t> receipts4;
        lightunit->create_txreceipts(receipts1, receipts4);

        return receipts4;
    }
};

TEST_F(test_account_confirmtx_filter, confirmtx_has_been_committed) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //insert committed txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 1);

    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> confirmtxs = get_tx(blockstore, xstore, sender, receiver, txs, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, xstore, sender, confirmtxs);

    //construct sendtx filter
    std::shared_ptr<xaccount_confirmtx_filter> filter = std::make_shared<xaccount_confirmtx_filter>(sender, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(confirmtxs[0], deny);
        ASSERT_EQ(deny, true);
        ASSERT_EQ(ret, xtxpool_success);
    }
}

// TEST_F(test_account_recv_filter, recvtx_has_not_been_committed_and_blockstore_only_exist_genesis_block) {
//     //construct block store
//     std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
//     xobject_ptr_t<xstore_face_t> blockdb = store::xstore_factory::create_store_with_memdb();
//     xobject_ptr_t<base::xvblockstore_t> blockstore;
//     blockstore.attach(store::get_vblockstore());

//     //construct account
//     std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
//     std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

//     //construct txs, commit txs to blockstore
//     std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);
//     xblock_t *block = test_xtxpool_util_t::create_unit_with_cons_txs(xstore, sender, txs);
//     data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(block);
//     std::vector<xcons_transaction_ptr_t> receipts1;
//     std::vector<xcons_transaction_ptr_t> receipts2;
//     lightunit->create_txreceipts(receipts1, receipts2);

//     block = test_xtxpool_util_t::create_unit_with_cons_txs(xstore, receiver, receipts1);
//     lightunit = dynamic_cast<data::xlightunit_block_t *>(block);
//     std::vector<xcons_transaction_ptr_t> receipts4;
//     lightunit->create_txreceipts(receipts1, receipts4);

//     //construct sendtx filter
//     std::shared_ptr<xaccount_confirmtx_filter> filter = std::make_shared<xaccount_confirmtx_filter>(sender, blockstore);
//     bool deny = false;
//     for (uint32_t i = 0; i < 2;i++){
//         enum_xtxpool_error_type ret = filter->reject(receipts4[0], deny);
//         ASSERT_EQ(deny, false);
//         ASSERT_EQ(ret, xtxpool_success);
//     }

//     for (uint32_t i = 0; i < 2;i++){
//         enum_xtxpool_error_type ret = filter->reject(receipts1[0], 0, deny);
//         ASSERT_EQ(deny, false);
//         ASSERT_EQ(ret, xtxpool_success);
//     }
// }

TEST_F(test_account_confirmtx_filter, confirmtx_has_not_been_committed_and_blockstore_exist_some_blocks) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //construct txs, commit txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 2);
    std::vector<xcons_transaction_ptr_t> txs1;
    txs1.push_back(txs[0]);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> confirmtxs = get_tx(blockstore,xstore, sender, receiver, txs1, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore,xstore, sender, confirmtxs);

    std::vector<xcons_transaction_ptr_t> txs2;
    txs2.push_back(txs[1]);
    confirmtxs = get_tx(blockstore,xstore, sender, receiver, txs2, &block);

    //construct sendtx filter
    std::shared_ptr<xaccount_confirmtx_filter> filter = std::make_shared<xaccount_confirmtx_filter>(sender, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(confirmtxs[0], deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }

    filter = std::make_shared<xaccount_confirmtx_filter>(sender, blockstore);
    for (uint32_t i = 0; i < 2;i++){
        enum_xtxpool_error_type ret = filter->reject(confirmtxs[0], block->get_height() + 1, deny);
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_success);
    }
}

TEST_F(test_account_confirmtx_filter, lack_some_block_in_the_blockstore) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    //construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    //construct txs, commit txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 2);
    std::vector<xcons_transaction_ptr_t> txs1;
    txs1.push_back(txs[0]);
    xblock_t *block;
    std::vector<xcons_transaction_ptr_t> confirmtxs = get_tx(blockstore,xstore, sender, receiver, txs1, &block);
    block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore,xstore, sender, confirmtxs);

    // construct not exist block
    xlightunit_block_para_t para;
    std::vector<xcons_transaction_ptr_t> txs2;
    txs2.push_back(txs[1]);
    para.set_input_txs(txs2);
    base::xvblock_t *not_exist_block = test_blocktuil::create_next_lightunit_with_consensus(para, block);

    //construct sendtx filter
    std::shared_ptr<xaccount_confirmtx_filter> filter = std::make_shared<xaccount_confirmtx_filter>(sender, blockstore);
    bool deny = false;
    for (uint32_t i = 0; i < 2; i++){
        enum_xtxpool_error_type ret = filter->update_reject_rule(dynamic_cast<data::xblock_t *>(not_exist_block));
        ASSERT_EQ(deny, false);
        ASSERT_EQ(ret, xtxpool_error_unitblock_lack);
    }
}
