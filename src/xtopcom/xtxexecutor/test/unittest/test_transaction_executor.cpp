#include "gtest/gtest.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"

using namespace top::txexecutor;
using namespace top::store;

class test_transaction_executor : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_transaction_executor, executor_1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("1111111111111111111");
    std::string dst1 = xblocktool_t::make_address_user_account("1111111111111111112");
    std::string dst2 = xblocktool_t::make_address_user_account("1111111111111111113");
    uint64_t init_balance = 10000000;
    auto genesis_block = xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    xassert(true == store->set_vblock(genesis_block));
    xassert(true == store->execute_block(genesis_block));

    xaccount_ptr_t account = store->query_account(address);
    auto tx1 = account->make_transfer_tx(dst1, 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    auto tx2 = account->make_transfer_tx(dst1, 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    auto tx3 = account->make_transfer_tx(dst1, 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    xcons_transaction_ptr_t ctx1 = make_object_ptr<xcons_transaction_t>(tx1.get());
    xcons_transaction_ptr_t ctx2 = make_object_ptr<xcons_transaction_t>(tx2.get());
    xcons_transaction_ptr_t ctx3 = make_object_ptr<xcons_transaction_t>(tx3.get());
    std::vector<xcons_transaction_ptr_t> batch_txs;
    batch_txs.push_back(ctx1);
    batch_txs.push_back(ctx2);
    batch_txs.push_back(ctx3);

    std::shared_ptr<store::xaccount_context_t> account_context = std::make_shared<store::xaccount_context_t>(address, store.get());
    xbatch_txs_result_t txs_result;
    int32_t ret = xtransaction_executor::exec_batch_txs(account_context.get(), batch_txs, txs_result);
    ASSERT_EQ(ret, xsuccess);
    ASSERT_EQ(txs_result.m_exec_succ_txs.size(), 3);
    ASSERT_EQ(txs_result.m_exec_fail_tx, nullptr);
    ASSERT_EQ(txs_result.m_exec_fail_tx_ret, xsuccess);
}

TEST_F(test_transaction_executor, executor_2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("1111111111111111111");
    std::string dst1 = xblocktool_t::make_address_user_account("1111111111111111112");
    std::string dst2 = xblocktool_t::make_address_user_account("1111111111111111113");
    uint64_t init_balance = 10000000;
    auto genesis_block = xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    xassert(true == store->set_vblock(genesis_block));
    xassert(true == store->execute_block(genesis_block));

    xaccount_ptr_t account = store->query_account(address);
    auto tx1 = account->make_transfer_tx(dst1, 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    auto tx2 = account->make_transfer_tx(dst1, init_balance + 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    auto tx3 = account->make_transfer_tx(dst1, 100, xverifier::xtx_utl::get_gmttime_s(), 0, ASSET_TOP(0.1));
    xcons_transaction_ptr_t ctx1 = make_object_ptr<xcons_transaction_t>(tx1.get());
    xcons_transaction_ptr_t ctx2 = make_object_ptr<xcons_transaction_t>(tx2.get());
    xcons_transaction_ptr_t ctx3 = make_object_ptr<xcons_transaction_t>(tx3.get());
    std::vector<xcons_transaction_ptr_t> batch_txs;
    batch_txs.push_back(ctx1);
    batch_txs.push_back(ctx2);
    batch_txs.push_back(ctx3);

    std::shared_ptr<store::xaccount_context_t> account_context = std::make_shared<store::xaccount_context_t>(address, store.get());
    xbatch_txs_result_t txs_result;
    int32_t ret = xtransaction_executor::exec_batch_txs(account_context.get(), batch_txs, txs_result);
    ASSERT_NE(ret, xsuccess);
    ASSERT_EQ(txs_result.m_exec_succ_txs.size(), 1);
    ASSERT_EQ(txs_result.m_exec_fail_tx->get_transaction()->digest(), ctx2->get_transaction()->digest());
    ASSERT_NE(txs_result.m_exec_fail_tx_ret, xsuccess);
}
