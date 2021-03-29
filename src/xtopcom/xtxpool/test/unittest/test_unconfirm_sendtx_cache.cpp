#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xbase/xvledger.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xunconfirm_sendtx_cache.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xtxpool/xtxpool_para.h"
#include "xblockstore/xblockstore_face.h"
#include "xtestca.hpp"

using namespace top::xtxpool;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::test;

class xchain_timer_mock final : public time::xchain_time_face_t {
public:
    void update_time(common::xlogic_time_t, time::xlogic_timer_update_strategy_t) override {
        return;
    }

    //void restore_last_db_time() override {}

    // time::xchain_time_st const & get_local_time() const noexcept override {
    //     static time::xchain_time_st st;
    //     st.xtime_round = 1;
    //     return st;
    // }

    void start() override {
    }

    void stop() override {
    }

    common::xlogic_time_t logic_time() const noexcept override { return timer_height; }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override { return true; }

    bool watch_one(uint64_t, time::xchain_time_watcher) override { return true; }

    bool unwatch(const std::string &) override { return true; }

    void init() override {}

    void                close() override {}
    base::xiothread_t * get_iothread() const noexcept override { return nullptr; }

    uint64_t timer_height{0};
};

class test_unconfirm_sendtx_cache : public testing::Test {
protected:
    void SetUp() override {
        m_chain_timer = std::make_shared<xchain_timer_mock>();
    }

    void TearDown() override {}
public:
    std::shared_ptr<xchain_timer_mock> m_chain_timer;
};

TEST_F(test_unconfirm_sendtx_cache, create_cache_only_genesis_unit) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, nullptr, enum_xtxpool_order_strategy_default);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);


    auto account = store_ptr->clone_account(sender);
    base::xvblock_t * genesis_block = data::xblocktool_t::create_genesis_empty_unit(sender);
    store_ptr->set_vblock(genesis_block);

    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);
    {
        ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_cache_update_fail_state_behind);
    }
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_normal_case_1) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, nullptr, enum_xtxpool_order_strategy_default);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_not_exist);

        ASSERT_EQ(cache.cache_account_size(), 1);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 3);
    }
}

TEST_F(test_unconfirm_sendtx_cache, self_tx_create_contract_tx) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, nullptr, enum_xtxpool_order_strategy_default);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);
    {

        xtransaction_ptr_t    raw_self_tx = make_object_ptr<xtransaction_t>();
        raw_self_tx->set_same_source_target_address(sender);
        raw_self_tx->set_digest();
        xcons_transaction_ptr_t cons_raw_self_tx = make_object_ptr<xcons_transaction_t>(raw_self_tx.get());

        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(cons_raw_self_tx);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_contract_create_tx(store_ptr.get(), sender, txs, receiver);
        blockstore->store_block(block);
        cache.on_unit_update(block->get_account(), 0);

        auto & unit_txinfos = block->get_txs();
        xassert(unit_txinfos[0]->is_self_tx());
        xassert(unit_txinfos[1]->is_send_tx());
        ASSERT_EQ(cache.has_txhash_receipt(sender, unit_txinfos[1]->get_raw_tx()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
    }
}

TEST_F(test_unconfirm_sendtx_cache, recover_unconfirm_txs) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, nullptr, enum_xtxpool_order_strategy_default);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    base::xvblock_t * genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    genesis_block->set_block_flag(base::enum_xvblock_flag_locked);
    genesis_block->set_block_flag(base::enum_xvblock_flag_committed);
    blockstore->store_block(genesis_block);

    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);

    std::vector<xcons_transaction_ptr_t> txs;
    txs.push_back(raw_txs[0]);

    xlightunit_block_para_t para1;
    para1.set_input_txs(txs);
    xblock_t * unit = (xblock_t *)test_blocktuil::create_next_lightunit(para1, genesis_block);

    test_blocktuil::set_block_consensus_para(unit, 1);
    
    xtable_block_para_t table_para;
    table_para.add_unit(unit);

    std::string       table_account = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t * table_genesis_block = xblocktool_t::create_genesis_empty_table(table_account);
    table_genesis_block->set_block_flag(base::enum_xvblock_flag_locked);
    table_genesis_block->set_block_flag(base::enum_xvblock_flag_committed);
    bool result = blockstore->store_block(table_genesis_block);
    ASSERT_EQ(result, true);

    xblock_t * table_proposal_block = (xblock_t *)test_blocktuil::create_next_tableblock(table_para, table_genesis_block);
    table_proposal_block->set_block_flag(base::enum_xvblock_flag_locked);
    table_proposal_block->set_block_flag(base::enum_xvblock_flag_committed);
    result = blockstore->store_block(table_proposal_block);
    ASSERT_EQ(result, true);

    {
        ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_not_exist);

        ASSERT_EQ(cache.cache_account_size(), 1);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 1);
    }

    cache.clear();
    ASSERT_EQ(cache.cache_account_size(), 0);

    base::xauto_ptr<xblockchain2_t> blockchain{store_ptr->clone_account(table_account)};
    ASSERT_NE(blockchain, nullptr);
    std::set<std::string> accounts = blockchain->get_unconfirmed_accounts();
    ASSERT_EQ(accounts.size(), 1);
    auto iter = accounts.find(sender);
    ASSERT_EQ(*iter, sender);

    cache.accounts_update(accounts, now);
    ASSERT_EQ(cache.cache_account_size(), 1);
    ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 1);
    ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
    ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
    ret = cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now);
    ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_not_exist);
}

TEST_F(test_unconfirm_sendtx_cache, get_unconfirm_tx) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, nullptr, enum_xtxpool_order_strategy_default);

    std::string account0 = test_xtxpool_util_t::get_account(0);
    std::string account1 = test_xtxpool_util_t::get_account(1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), account0);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);
    std::vector<xcons_transaction_ptr_t> raw_txs2 = test_xtxpool_util_t::create_cons_transfer_txs(1 , 2, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), account0, txs);
        blockstore->store_block(block);
    }

    {
        int32_t ret = cache.has_txhash_receipt(account0, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        auto tx1 = cache.get_unconfirm_tx(account0, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_NE(tx1, nullptr);
        auto tx2 = cache.get_unconfirm_tx(account1, raw_txs2[0]->get_transaction()->digest(), now);
        ASSERT_EQ(tx2, nullptr);
    }
}


TEST_F(test_unconfirm_sendtx_cache, timeout_retry_send_1) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);

    std::string sender = test_xtxpool_util_t::get_account(0);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    std::string table_addr = "table_tmp";
    xunconfirm_sendtx_cache_t cache(para, table_addr);
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        blockstore->store_block(block);
    }
    {
        ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
        ret = cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);

        ASSERT_EQ(cache.cache_account_size(), 1);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 3);
    }

    xtxpool_receipt_receiver_counter counter;
    std::vector<xcons_transaction_ptr_t> resend_txs = cache.on_timer_check_cache(xverifier::xtx_utl::get_gmttime_s() + send_tx_receipt_first_retry_timeout + 1, counter);
    ASSERT_EQ(resend_txs.size(), 1);
}

TEST_F(test_unconfirm_sendtx_cache, multi_sign_error) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);

    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr, make_observer(m_chain_timer));

    std::string table_account = xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xblock_t* genesis_block = (xblock_t*)test_blocktuil::create_genesis_empty_table(table_account);
    xassert(true == blockstore->store_block(genesis_block));

    std::vector<base::xvblock_t*> blocks;

    base::xvblock_t* prev_block = genesis_block;
    uint64_t clock = 10;
    for (uint64_t i = 1; i < 80; i++) {
        xblock_t* empty_block = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block, clock++);
        blocks.push_back(empty_block);
        prev_block = empty_block;
    }

    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 2);
    xtable_block_t * tx_tableblock1;
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(txs);
        base::xvblock_t * sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, sender_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock1 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock1;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock1, clock++);
        prev_block = empty_block2;

        blocks.push_back(tx_tableblock1);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);

        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        tx_tableblock1->create_txreceipts(sendtx_receipts, recvtx_receipts);
        assert(!sendtx_receipts.empty());
        for (auto iter : sendtx_receipts) {
            iter->set_commit_prove_with_parent_cert(empty_block2->get_cert());
        }
    }

    xschnorrcert_t * schnorrcert = dynamic_cast<xschnorrcert_t *>(cert_ptr.get());
    schnorrcert->set_verify_muti_sign_ret(base::enum_vcert_auth_result::enum_verify_fail);
    int32_t ret = xtxpool->push_recv_ack_tx(sendtx_receipts[0]);
    ASSERT_EQ(ret, xtxpool_error_tx_multi_sign_error);
}

#if 0
TEST_F(test_unconfirm_sendtx_cache, create_cache_normal_case_2) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = 0;
    xunconfirm_sendtx_cache_t cache(make_observer(store_ptr));
    {
        bool result;
        ret = cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_not_exist);
    }

    xblock_t * block01 = nullptr;
    xblock_t * block23 = nullptr;
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        txs.push_back(raw_txs[1]);
        block01 = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_not_exist);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 2);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        txs.push_back(raw_txs[3]);
        block23 = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 4);
    }
    {
        data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(block01);
        std::vector<xcons_transaction_ptr_t> sendtx_receipts;
        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);

        block01 = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, sendtx_receipts);
        lightunit = dynamic_cast<data::xlightunit_block_t *>(block01);
        std::vector<xcons_transaction_ptr_t> sendtx_receipts1;
        std::vector<xcons_transaction_ptr_t> recvtx_receipts1;
        lightunit->create_txreceipts(sendtx_receipts1, recvtx_receipts1);

        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, recvtx_receipts1);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_not_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_not_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_EQ(cache.cache_account_size(), 1);
        ASSERT_EQ(cache.get_account_cache(sender)->cache_size(), 2);
    }
    {
        data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(block23);
        std::vector<xcons_transaction_ptr_t> sendtx_receipts;
        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);

        block23 = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, sendtx_receipts);
        lightunit = dynamic_cast<data::xlightunit_block_t *>(block23);
        std::vector<xcons_transaction_ptr_t> sendtx_receipts1;
        std::vector<xcons_transaction_ptr_t> recvtx_receipts1;
        lightunit->create_txreceipts(sendtx_receipts1, recvtx_receipts1);

        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, recvtx_receipts1);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_not_exist);
        ASSERT_EQ(cache.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), now), xtxpool_error_unconfirm_sendtx_not_exist);
        ASSERT_EQ(cache.cache_account_size(), 0);
    }
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_abnormal_case_1_state_behind) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    uint64_t     now = 0;
    int32_t      ret;
    auto         store_ptr_2 = store::xstore_factory::create_store_with_memdb(nullptr);
    xblock_ptr_t genesis_unit = store_ptr->get_block_by_height(data::enum_xblock_type::xblock_type_unit, sender, 0);
    store_ptr_2->set_vblock(genesis_unit.get());
    xunconfirm_sendtx_cache_t cache_2{make_observer(store_ptr_2)};
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        store_ptr_2->set_vblock(unit);
    }
    cache_2.on_unit_update(sender, now);

    {
        ret = cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_NE(ret, xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_NE(ret, xtxpool_error_unconfirm_sendtx_not_exist);
    }

    xblock_ptr_t unit1 = store_ptr->get_block_by_height(data::enum_xblock_type::xblock_type_unit, sender, 1);
    store_ptr_2->set_vblock(unit1.get());
    cache_2.on_unit_update(sender, now);

    xblock_t *                           missing_unit;
    std::vector<xcons_transaction_ptr_t> missing_txs;
    {
        missing_txs.push_back(raw_txs[2]);
        missing_unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, missing_txs);
        // here not set unit, missing
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[3]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        store_ptr_2->set_vblock(unit);
    }
    cache_2.on_unit_update(sender, now);

    {
        auto account_cache = cache_2.get_account_cache(sender);
        ASSERT_EQ(account_cache->get_account_max_height(), 4);
        ASSERT_EQ(account_cache->get_cache_max_height(), 2);
        ASSERT_EQ(account_cache->cache_complete(), false);
        ret = cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_NE(ret, xtxpool_error_unconfirm_sendtx_exist);
        ASSERT_NE(ret, xtxpool_error_unconfirm_sendtx_not_exist);
    }

    store_ptr_2->set_vblock(missing_unit);
    cache_2.on_unit_update(sender, now);

    {
        auto account_cache = cache_2.get_account_cache(sender);
        ASSERT_EQ(account_cache->get_account_max_height(), 4);
        ASSERT_EQ(account_cache->get_cache_max_height(), 4);
        ASSERT_EQ(account_cache->cache_complete(), true);
        ret = cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
    }
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_not_continuous_1) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    uint64_t                  now = 0;
    int32_t                   ret;
    xunconfirm_sendtx_cache_t cache_2{make_observer(store_ptr)};
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    cache_2.on_unit_update(sender, now);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 2);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 2);

    {
        std::vector<xcons_transaction_ptr_t> txs;
        raw_txs[0]->get_transaction()->set_tx_subtype(enum_transaction_subtype_confirm);
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        raw_txs[1]->get_transaction()->set_tx_subtype(enum_transaction_subtype_confirm);
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[3]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    cache_2.on_unit_update(sender, now);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 2);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 6);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_not_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_not_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_not_continuous_2) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    uint64_t                  now = 0;
    int32_t                   ret;
    xunconfirm_sendtx_cache_t cache_2{make_observer(store_ptr)};
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 2);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 2);

    {
        std::vector<xcons_transaction_ptr_t> txs;
        raw_txs[0]->get_transaction()->set_tx_subtype(enum_transaction_subtype_confirm);
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        raw_txs[1]->get_transaction()->set_tx_subtype(enum_transaction_subtype_confirm);
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[3]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_not_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_not_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 2);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 6);
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_not_continuous_3) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    uint64_t                  now = 0;
    int32_t                   ret;
    xunconfirm_sendtx_cache_t cache_2{make_observer(store_ptr)};
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 2);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 2);

    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[3]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[1]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[2]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.has_txhash_receipt(sender, raw_txs[3]->get_transaction()->digest(), 0), xtxpool_error_unconfirm_sendtx_exist);
    ASSERT_EQ(cache_2.get_account_cache(sender)->cache_size(), 4);
    ASSERT_EQ(cache_2.get_account_cache_height(sender), 4);
}

TEST_F(test_unconfirm_sendtx_cache, create_cache_abnormal_case_2_only_unit_without_txs) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t ret;
    auto    store_ptr_2 = store::xstore_factory::create_store_with_memdb(nullptr);

    xblock_ptr_t genesis_unit = store_ptr->get_block_by_height(data::enum_xblock_type::xblock_type_unit, sender, 0);
    store_ptr_2->set_vblock(genesis_unit.get());
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        store_ptr_2->set_vblock(unit);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        store_ptr_2->set_vblock(unit);
    }
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[2]);
        auto unit = test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        store_ptr_2->set_vblock(unit);
    }

    uint64_t                  now = 0;
    xunconfirm_sendtx_cache_t cache_2{make_observer(store_ptr_2)};
    {
        ret = cache_2.has_txhash_receipt(sender, raw_txs[0]->get_transaction()->digest(), now);
        ASSERT_EQ(ret, xtxpool_error_unconfirm_sendtx_exist);
    }
}

class xreceipt_tranceiver_demo_t : public xreceipt_tranceiver_face_t {
public:
    void send_receipt(const xcons_transaction_ptr_t & receipt) {
        m_sendtx_count++;
        m_max_nonce = receipt->get_transaction()->get_tx_nonce();
        xdbg("retry sendtx nonce:%ld", receipt->get_transaction()->get_tx_nonce());
    }
    uint32_t m_sendtx_count = 0;
    uint64_t m_max_nonce = 0;
};
#if 0  // TODO(nathan)
TEST_F(test_unconfirm_sendtx_cache, timeout_retry_send_1) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  now = xverifier::xtx_utl::get_gmttime_s();
    xunconfirm_sendtx_cache_t cache(make_observer(store_ptr));
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    cache.on_unit_update(sender, now);
    xreceipt_tranceiver_demo_t tranceiver;
    cache.on_timer_check_cache(now);
    ASSERT_EQ(tranceiver.m_sendtx_count, 0);
    now += 10000;
    cache.on_timer_check_cache(now);
    ASSERT_EQ(tranceiver.m_sendtx_count, 1);
    cache.on_timer_check_cache(now);
    ASSERT_EQ(tranceiver.m_sendtx_count, 1);
    now += 10000;
    cache.on_timer_check_cache(now);
    ASSERT_EQ(tranceiver.m_sendtx_count, 2);
}
#endif
#if 0
TEST_F(test_unconfirm_sendtx_cache, timeout_retry_send_2) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    int32_t                   ret;
    uint64_t                  timebase = xverifier::xtx_utl::get_gmttime_s();
    uint64_t                  now = timebase;
    xunconfirm_sendtx_cache_t cache(make_observer(store_ptr));
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    cache.on_unit_update(sender, now);
    {
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[1]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    now = timebase + 9999;
    cache.on_unit_update(sender, now);

    now = timebase;
    xreceipt_tranceiver_demo_t tranceiver;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, 0);
    now = timebase + 10000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, 1);
    now = timebase + 20000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, 3);

    {
        std::vector<xcons_transaction_ptr_t> txs;
        raw_txs[0]->get_transaction()->set_tx_subtype(enum_transaction_subtype_confirm);
        txs.push_back(raw_txs[0]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
    }
    cache.on_unit_update(sender, now);
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, 3);
}

TEST_F(test_unconfirm_sendtx_cache, timeout_retry_send_3_round_robin_200) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    uint32_t max_count = 200;
    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, max_count);

    int32_t                   ret;
    uint64_t                  timebase = xverifier::xtx_utl::get_gmttime_s();
    uint64_t                  now = timebase;
    xunconfirm_sendtx_cache_t cache(make_observer(store_ptr), common::xnode_type_t::auditor);
    for (uint32_t i = 0; i < max_count; i++) {
        now = timebase + i;
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[i]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        cache.on_unit_update(sender, now);
    }

    xreceipt_tranceiver_demo_t tranceiver;
    now = timebase + 10000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT);
    ASSERT_EQ(tranceiver.m_max_nonce, MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT);
    now = timebase + 20000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT * 2);
    ASSERT_EQ(tranceiver.m_max_nonce, MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT * 2);
}

TEST_F(test_unconfirm_sendtx_cache, timeout_retry_send_3_round_robin_10) {
    auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);

    uint32_t max_count = 10;
    test_xtxpool_util_t::create_genesis_account(store_ptr.get(), sender);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, max_count);

    int32_t                   ret;
    uint64_t                  timebase = xverifier::xtx_utl::get_gmttime_s();
    uint64_t                  now = timebase;
    xunconfirm_sendtx_cache_t cache(make_observer(store_ptr), common::xnode_type_t::auditor);
    for (uint32_t i = 0; i < max_count; i++) {
        now = timebase + i;
        std::vector<xcons_transaction_ptr_t> txs;
        txs.push_back(raw_txs[i]);
        test_xtxpool_util_t::create_unit_with_cons_txs(store_ptr.get(), sender, txs);
        cache.on_unit_update(sender, now);
    }

    xreceipt_tranceiver_demo_t tranceiver;
    now = timebase + 10000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, max_count);
    ASSERT_EQ(tranceiver.m_max_nonce, max_count);
    now = timebase + 20000;
    cache.on_timer_check_cache(now, tranceiver);
    ASSERT_EQ(tranceiver.m_sendtx_count, max_count * 2);
    ASSERT_EQ(tranceiver.m_max_nonce, max_count);
}
#endif
#endif
