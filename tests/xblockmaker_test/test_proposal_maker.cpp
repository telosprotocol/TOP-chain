#include "gtest/gtest.h"
#include "xblockmaker/xtable_maker.h"
#include "xblockmaker/xproposal_maker.h"
#include "xblockmaker/xproposal_maker_mgr.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "xvledger/xvblock.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"
#include "test_common.hpp"
#if 0  // TODO(jimmy)
using namespace top::txexecutor;
using namespace top::store;
using namespace top::base;
using namespace top::mock;
using namespace top::xtxpool_v2;
using namespace top::utl;
using namespace top;

class test_proposal_maker : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_proposal_maker, table_maker_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::shared_ptr<xunit_service::xblock_maker_face> blockmaker = xblockmaker_factory::create_table_proposal(make_observer(resouces->get_store()), make_observer(resouces->get_blockstore()), make_observer(resouces->get_txpool()));
    ASSERT_NE(blockmaker, nullptr);
}

TEST_F(test_proposal_maker, table_maker_2) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));

    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), make_observer(blockstore.get()), nullptr, nullptr, nullptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    xblockmaker_resources_ptr_t resouces = std::make_shared<xblockmaker_resources_impl_t>(make_observer(store_ptr.get()), make_observer(blockstore.get()), make_observer(xtxpool.get()));
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::shared_ptr<xunit_service::xblock_maker_face> blockmaker = xblockmaker_factory::create_table_proposal(make_observer(store_ptr.get()), make_observer(blockstore.get()), make_observer(xtxpool.get()));
    ASSERT_NE(blockmaker, nullptr);

    xdatamock_tx datamock_account1(resouces);
    xdatamock_tx datamock_account2(resouces);
    std::string account1 = datamock_account1.get_account();
    std::string dstaccount = datamock_account2.get_account();

    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 1);
    ASSERT_EQ(txs1[0]->get_transaction()->get_tx_nonce(), 2);

    xaccount_ptr_t blockchain = resouces->get_store()->query_account(account1);
    ASSERT_NE(blockchain, nullptr);

    // ASSERT_EQ(xsuccess, resouces->get_txpool()->push_send_tx(txs1[0]));
    xassert(false);

    // uint64_t table_committed_height = 0;
    // uint64_t unit_committed_height = 0;
    // auto pull_txs = resouces->get_txpool()->get_account_txs(account1, table_committed_height, unit_committed_height,
    //                     blockchain->account_send_trans_number(), blockchain->account_send_trans_hash(), now);
    // ASSERT_EQ(pull_txs.size(), 1);
}


class test_xchain_timer_mock final : public time::xchain_time_face_t {
public:
    bool update_time(data::xblock_t* timer_block, bool force = false) override { return true; }

    //void restore_last_db_time() override {}

    // time::xchain_time_st const & get_local_time() const noexcept override {
    //     static time::xchain_time_st st;
    //     st.xtime_round = 1;
    //     return st;
    // }

    common::xlogic_time_t logic_time() const noexcept override { return timer_height; }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override { return true; }

    bool watch_one(uint64_t, time::xchain_time_watcher) override { return true; }

    bool unwatch(const std::string &) override { return true; }

    void init() override {}

    void                close() override {}
    base::xiothread_t * get_iothread() const noexcept override { return nullptr; }

    uint64_t timer_height{0};
};

TEST_F(test_proposal_maker, table_maker_3) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    std::shared_ptr<test_xchain_timer_mock> chain_timer = std::make_shared<test_xchain_timer_mock>();

    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), nullptr,  make_observer(chain_timer), nullptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    xblockmaker_resources_ptr_t resouces = std::make_shared<xblockmaker_resources_impl_t>(make_observer(store_ptr.get()), make_observer(blockstore.get()), make_observer(xtxpool.get()));

    xdatamock_tx datamock_account1(resouces);
    xdatamock_tx datamock_account2(resouces);
    std::string account1 = datamock_account1.get_account();
    std::string dstaccount = datamock_account2.get_account();
    uint16_t subaddr1 = base::xvaccount_t::get_ledgersubaddr_from_account(account1);
    std::string taccount = data::xblocktool_t::make_address_shard_table_account(subaddr1);

    std::shared_ptr<xproposal_maker_t> blockmaker = std::make_shared<xproposal_maker_t>(taccount, resouces);
    ASSERT_NE(blockmaker, nullptr);

    xaccount_ptr_t blockchain = resouces->get_store()->query_account(account1);
    ASSERT_NE(blockchain, nullptr);
    ASSERT_EQ(blockchain->account_send_trans_number(), 1);
    base::xauto_ptr<base::xvblock_t> commmit_unit = resouces->get_blockstore()->get_latest_committed_block(account1);
    ASSERT_EQ(blockchain->get_chain_height(), commmit_unit->get_height());
    xdbg("JIMMY account=%s,nonce=%ld", account1.c_str(), blockchain->account_send_trans_number());

    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 1);
    ASSERT_EQ(txs1[0]->get_transaction()->get_tx_nonce(), 2);

    // ASSERT_EQ(xsuccess, resouces->get_txpool()->push_send_tx(txs1[0]));
    xassert(false);

    time_t gmt = base::xtime_utl::gmttime();
    long long_gmt = gmt;
    uint64_t new_clock = long_gmt - base::TOP_BEGIN_GMTIME;
    uint64_t view_id = 100;
    uint64_t timestamp = base::xtime_utl::gmttime();
    uint32_t viewtoken = 100;

    // base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    // xblock_consensus_para_t proposal_para(taccount, new_clock, view_id, viewtoken, latest_blocks.get_latest_cert_block()->get_height() + 1);
    // proposal_para.set_latest_blocks(latest_blocks);
    // xtablemaker_para_t table_para;
    // ASSERT_EQ(true, blockmaker->update_txpool_txs(proposal_para, table_para));

    // auto table_txpool = resouces->get_txpool()->get_txpool_table(taccount);

    // uint64_t table_committed_height = 0;
    // auto pull_txs = table_txpool->get_account_txs(account1, table_committed_height, commmit_unit->get_height(),
    //                     blockchain->account_send_trans_number(), blockchain->account_send_trans_hash(), now);
    // ASSERT_EQ(pull_txs.size(), 1);
}
#endif
