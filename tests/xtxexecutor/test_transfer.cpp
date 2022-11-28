#include "gtest/gtest.h"
#include "xstore/xaccount_context.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xdata/xtransaction.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xchain_fork/xutility.h"

#include "tests/mock/xdatamock_tx.hpp"

using namespace top;
using namespace top::mock;
using namespace top::data;
using namespace top::txexecutor;

class test_transfer : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    void construct_tx(xtransaction_ptr_t & tx, bool is_eth_tx) {
        m_tx_mocker.construct_tx(tx, is_eth_tx);
        m_source_context = m_tx_mocker.get_source_context();
        tx->set_fire_timestamp(xtransaction_t::get_gmttime_s());
        m_target_context = m_tx_mocker.get_target_context();
        m_transfer_out_amount = m_tx_mocker.get_transfer_out_amount();
        m_deposit = m_tx_mocker.get_deposit();
        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void construct_cons_tx_v2(bool is_eth_tx = false) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
        construct_tx(tx, is_eth_tx);
    }

 public:
    xcons_transaction_ptr_t m_trans;
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    uint64_t m_transfer_out_amount{10};
    uint32_t m_deposit{100000};

    mock::xdatamock_tx m_tx_mocker;
};

TEST_F(test_transfer, transfer_v2_free_tgas_enough) {
    construct_cons_tx_v2();

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    uint32_t tx_len = 100;
    m_trans->get_transaction()->set_tx_len(tx_len);
    uint32_t tgas_usage = tx_len * 3;
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance, m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(tgas_usage, m_source_context->get_used_tgas());
    EXPECT_EQ(tgas_usage, m_trans->get_current_used_tgas());

    tx.source_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount, m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));

    tx.source_confirm_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount, m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
}

TEST_F(test_transfer, eth_transfer_v2_free_tgas_enough) {
    construct_cons_tx_v2(true);

    data::xproperty_asset asset{XPROPERTY_ASSET_ETH, ASSET_TOP(100)};

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    evm_common::u256 amount = 100000000;
    m_source_context->get_blockchain()->tep_token_deposit(common::xtoken_id_t::eth, amount);
    auto balance = m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(ASSET_TOP(100), balance);

    uint32_t tx_len = 100;
    m_trans->get_transaction()->set_tx_len(tx_len);
    uint32_t tgas_usage = tx_len * 3;
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(tgas_usage, m_source_context->get_used_tgas());
    EXPECT_EQ(tgas_usage, m_trans->get_current_used_tgas());

    tx.source_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));

    tx.source_confirm_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
}

TEST_F(test_transfer, transfer_v2_deposit_enough) {
    construct_cons_tx_v2();

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    uint32_t tx_len = 10000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tgas_usage = tx_len * 3 = 30 000 = 25 000 + 100,000 / 20 = free_tgas + (m_deposit) / ratio
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(25000, m_source_context->get_used_tgas());
    EXPECT_EQ(25000, m_trans->get_current_used_tgas());
    EXPECT_EQ(ASSET_TOP(0.1), m_trans->get_current_used_deposit());

    tx.source_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));

    tx.source_confirm_action_exec();
    EXPECT_EQ(balance - m_transfer_out_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
}

TEST_F(test_transfer, eth_transfer_v2_deposit_enough) {
    construct_cons_tx_v2(true);

    data::xproperty_asset asset{XPROPERTY_ASSET_ETH, 100};

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    evm_common::u256 amount = 100000000;
    m_source_context->get_blockchain()->tep_token_deposit(common::xtoken_id_t::eth, amount);
    auto eth_balance = m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(100000000, eth_balance);
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);

    uint32_t tx_len = 10000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tgas_usage = tx_len * 3 = 30 000 = 25 000 + 100,000 / 20 = free_tgas + (m_deposit) / ratio
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(25000, m_source_context->get_used_tgas());
    EXPECT_EQ(25000, m_trans->get_current_used_tgas());
    EXPECT_EQ(ASSET_TOP(0.1), m_trans->get_current_used_deposit());

    EXPECT_EQ(eth_balance, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));

    tx.source_action_exec();
    EXPECT_EQ(eth_balance - m_transfer_out_amount, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));

    tx.source_confirm_action_exec();
    EXPECT_EQ(eth_balance - m_transfer_out_amount, m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
}

TEST_F(test_transfer, transfer_v2_not_enough) {
    construct_cons_tx_v2();

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    uint32_t tx_len = 20000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tgas_usage = tx_len * 3 = 60 000 > (25 000 + 100,000 / 20) = free_tgas + (m_deposit) / ratio
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, store::xtransaction_not_enough_pledge_token_tgas);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(55000, m_source_context->get_used_tgas());
    EXPECT_EQ(55000, m_trans->get_current_used_tgas());
}

TEST_F(test_transfer, eth_transfer_v2_not_enough) {
    construct_cons_tx_v2(true);

    data::xproperty_asset asset{XPROPERTY_ASSET_ETH, 100};

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    evm_common::u256 amount = 100000000;
    m_source_context->get_blockchain()->tep_token_deposit(common::xtoken_id_t::eth, amount);
    auto eth_balance = m_source_context->get_blockchain()->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(100000000, eth_balance);
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);

    uint32_t tx_len = 20000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tgas_usage = tx_len * 3 = 60 000 > (25 000 + 100,000 / 20) = free_tgas + (m_deposit) / ratio
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, store::xtransaction_not_enough_pledge_token_tgas);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_deposit, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));
    EXPECT_EQ(55000, m_source_context->get_used_tgas());
    EXPECT_EQ(55000, m_trans->get_current_used_tgas());
}

TEST_F(test_transfer, v2_burn_token) {
    m_tx_mocker.set_target_account(top::black_hole_addr);
    construct_cons_tx_v2();

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    uint32_t tx_len = 10;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    ret = tx.source_action_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance - m_tx_mocker.get_transfer_out_amount(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(m_tx_mocker.get_transfer_out_amount(), m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));

    ret = tx.target_fee_exec();
    EXPECT_EQ(ret, 0);
}
#if 0 
// fork based on tx fire timestamp
TEST_F(test_transfer, transfer_v1) {
    {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        construct_tx(tx);
    }

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    data::xproperty_asset asset_out{10};
    xaction_asset_out::serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    xaction_asset_in::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_asset_in);

    uint32_t tx_len = 10000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tx_len * 3 = 30 000 = 25 000 + 100,000 / 20
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(100000, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));

    tx.source_action_exec();
    EXPECT_EQ(balance - asset_out.m_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(25000, m_source_context->get_used_tgas());

    tx.source_confirm_action_exec();
    EXPECT_EQ(balance - asset_out.m_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
}

// fork based on transaction version
TEST_F(test_transfer, transfer_v1) {
    {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        construct_tx(tx);
    }

    m_source_context->top_token_transfer_in(ASSET_TOP(100));
    auto balance = m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE);
    EXPECT_EQ(ASSET_TOP(100), balance);

    data::xproperty_asset asset_out{10};
   base::xstream_t stream(base::xcontext_t::instance());
    stream << asset_out.m_token_name;
    stream << asset_out.m_amount;
    std::string param((char *)stream.data(), stream.size());
    m_trans->get_transaction()->set_source_action_type(xaction_type_asset_out);
    m_trans->get_transaction()->set_source_action_para(param);
    m_trans->get_transaction()->set_target_action_type(xaction_type_asset_in);
    m_trans->get_transaction()->set_target_action_para(param);

    uint32_t tx_len = 10000;
    m_trans->get_transaction()->set_tx_len(tx_len);
    xtransaction_transfer tx(m_source_context.get(), m_trans);

    tx.parse();
    // tx_len * 3 = 30 000 = 25 000 + 100,000 / 20
    auto ret = tx.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(balance - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(0, m_source_context->token_balance(XPROPERTY_BALANCE_BURN));
    EXPECT_EQ(100000, m_source_context->token_balance(XPROPERTY_BALANCE_LOCK));

    tx.source_action_exec();
    EXPECT_EQ(balance - asset_out.m_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
    EXPECT_EQ(25000, m_source_context->get_used_tgas());

    tx.source_confirm_action_exec();
    EXPECT_EQ(balance - asset_out.m_amount - m_trans->get_transaction()->get_deposit(), m_source_context->token_balance(XPROPERTY_BALANCE_AVAILABLE));
}
#endif

