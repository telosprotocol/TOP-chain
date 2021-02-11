#include "gtest/gtest.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xdata/xaction_parse.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xconfig/xconfig_register.h"

using namespace top::txexecutor;
using namespace top::data;
using namespace top;
using namespace top::store;

class test_disk_service : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_source_context->set_lock_token_sum(XGET_CONFIG(min_account_deposit));
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset_out{100};
        xaction_source_null::serialze_to(tx->get_source_action());
        xaction_pledge_token::serialze_to(tx->get_target_action(), asset_out, xaction_type_pledge_token);
        tx->set_different_source_target_address(m_source_account, m_target_account);
        tx->set_deposit(100000);
        tx->set_digest();

        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    std::string m_source_account{"T0000011111111111111111"};
    std::string m_target_account{"T0000011111111111111111"};
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    xcons_transaction_ptr_t             m_trans;
};

TEST_F(test_disk_service, pledge_no_balance) {
    m_source_context->get_blockchain()->set_balance(10);

    xtransaction_pledge_token_disk pledge_token_disk(m_source_context.get(), m_trans);

    pledge_token_disk.parse();
    pledge_token_disk.check();
    int32_t ret = pledge_token_disk.source_fee_exec();
    EXPECT_EQ(xconsensus_service_error_balance_not_enough, ret);
    ret = pledge_token_disk.source_action_exec();
    EXPECT_EQ(0, ret);
    ret = pledge_token_disk.target_action_exec();
    EXPECT_NE(0, ret);

    m_trans->get_transaction()->set_different_source_target_address(m_source_account, "m_target_account");
    ret = pledge_token_disk.target_action_exec();
    EXPECT_EQ(xaccount_property_parent_account_not_exist, ret);
}

/*
TEST_F(test_disk_service, pledge_balance) {
    uint64_t balance = ASSET_TOP(300);
    m_source_context->get_blockchain()->set_balance(balance);
	m_source_context->set_lock_token_sum(ASSET_TOP(200));
    uint16_t count = 0;
    while (++count < 5) {
        const uint16_t pledge_token = 10;
        data::xproperty_asset asset_out{pledge_token};
        data::xaction_asset_out action_asset_out;
        action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
        action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out);
        m_trans->get_transaction()->set_different_source_target_address(m_source_account, m_target_account);

        xtransaction_pledge_token_disk pledge_token_disk(m_source_context.get(), m_trans);

        pledge_token_disk.parse();
        pledge_token_disk.check();
        int32_t ret = pledge_token_disk.target_action_exec();
        EXPECT_EQ(0, ret);

        auto sum_token = m_source_context->get_blockchain()->disk_balance();
        EXPECT_EQ(pledge_token * count, sum_token);
        auto disk_change = m_source_context->get_disk_balance_change();
        EXPECT_EQ(disk_change, sum_token);
        EXPECT_EQ(balance - pledge_token * count, m_source_context->get_blockchain()->balance() + m_source_context->get_balance_change());
    }
}*/

TEST_F(test_disk_service, redeem_no_token) {
    m_source_context->get_blockchain()->set_balance(100000000000);
    data::xproperty_asset asset_out{100};

    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_redeem_token);
    m_trans->get_transaction()->set_different_source_target_address(m_source_account, m_target_account);

    xtransaction_redeem_token_disk redeem_token_disk(m_source_context.get(), m_trans);

    redeem_token_disk.parse();
    redeem_token_disk.check();
    int32_t ret = redeem_token_disk.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = redeem_token_disk.source_action_exec();
    EXPECT_EQ(0, ret);
    ret = redeem_token_disk.target_action_exec();
    EXPECT_NE(0, ret);

    m_trans->get_transaction()->set_different_source_target_address(m_source_account, "m_target_account");
    ret = redeem_token_disk.target_action_exec();
    EXPECT_EQ(xaccount_property_parent_account_not_exist, ret);
}

TEST_F(test_disk_service, redeem_token) {
    const uint16_t pledge_token = 11;
    const uint16_t redeem_token = 5;
    const uint64_t balance = ASSET_TOP(300);
    {
        m_source_context->get_blockchain()->set_balance(balance);
		m_source_context->set_lock_token_sum( ASSET_TOP(200));
        data::xproperty_asset asset_out{pledge_token};
        xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
        xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
        m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
        m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

        xtransaction_pledge_token_disk pledge_token_disk(m_source_context.get(), m_trans);

        pledge_token_disk.parse();
        pledge_token_disk.check();
        int32_t ret = pledge_token_disk.target_action_exec();
        EXPECT_EQ(0, ret);
        auto sum_token = m_source_context->get_blockchain()->disk_balance();
        EXPECT_EQ(pledge_token, sum_token);
        auto disk_change = m_source_context->get_disk_balance_change();
        EXPECT_EQ(disk_change, sum_token);
        EXPECT_EQ(balance - pledge_token, m_source_context->get_blockchain()->balance() + m_source_context->get_balance_change());
    }

    {
        data::xproperty_asset asset_out{redeem_token};
        xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
        xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_redeem_token);
        m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
        m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

        xtransaction_redeem_token_disk redeem_token_disk(m_source_context.get(), m_trans);

        redeem_token_disk.parse();
        redeem_token_disk.check();
        int32_t ret = redeem_token_disk.target_action_exec();
        EXPECT_EQ(0, ret);
        auto sum_token = m_source_context->get_blockchain()->disk_balance();
        EXPECT_EQ(pledge_token - redeem_token, sum_token);
        auto disk_change = m_source_context->get_disk_balance_change();
        EXPECT_EQ(disk_change, pledge_token - redeem_token);
        // redeemed token not unlocked yet
        EXPECT_EQ(balance - pledge_token, m_source_context->get_blockchain()->balance() + m_source_context->get_balance_change());
        // unlock redeemed token,needs substract redeem fee
        // EXPECT_EQ(balance - pledge_token + redeem_token * 0.999, m_source_context->get_blockchain()->balance() + m_source_context->get_balance_change());
    }
}

TEST_F(test_disk_service, transfer_illegal) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + 100000);
    m_source_context->string_set(XPROPERTY_USED_DISK_KEY, "409500", true);

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr("m_target_account");

    xtransaction_transfer transfer_tgas(m_source_context.get(), m_trans);
    m_trans->get_transaction()->set_deposit(100);
    auto ret = transfer_tgas.check();
    EXPECT_EQ(xverifier::xverifier_error::xverifier_error_tx_min_deposit_invalid, ret);

    m_trans->get_transaction()->set_deposit(100000);
    asset_out.m_amount = 0;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    xtransaction_transfer transfer_tgas1(m_source_context.get(), m_trans);
    ret = transfer_tgas1.check();
    EXPECT_EQ(xverifier::xverifier_error::xverifier_error_transfer_tx_min_amount_invalid, ret);

    asset_out.m_amount = TOTAL_ISSUANCE + 1;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    xtransaction_transfer transfer_tgas2(m_source_context.get(), m_trans);
    transfer_tgas2.parse();
    ret = transfer_tgas2.check();
    EXPECT_EQ(xverifier::xverifier_error::xverifier_error_transfer_tx_amount_over_max, ret);

    asset_out.m_amount = 1;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xproperty_asset asset_out2{2};
    action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out2);
    xtransaction_transfer transfer_tgas3(m_source_context.get(), m_trans);
    transfer_tgas3.parse();
    ret = transfer_tgas3.check();
    EXPECT_EQ(xverifier::xverifier_error::xverifier_error_trnsafer_tx_src_dst_amount_not_same, ret);
}

TEST_F(test_disk_service, transfer_no_disk) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + 100000);
    m_source_context->string_set(XPROPERTY_USED_DISK_KEY, "409500", true);

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_transfer transfer_tgas(m_source_context.get(), m_trans);

    transfer_tgas.parse();
    transfer_tgas.check();
    int32_t ret = transfer_tgas.source_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_disk_service, transfer_use_pledge_disk) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + ASSET_TOP(10));
	m_source_context->set_lock_token_sum(ASSET_TOP(200));
    m_source_context->string_set(XPROPERTY_USED_DISK_KEY, "409500", true);
    m_source_context->get_blockchain()->set_disk_balance(1000);


    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_transfer transfer_tgas(m_source_context.get(), m_trans);

    transfer_tgas.parse();
    transfer_tgas.check();
    int32_t ret = transfer_tgas.source_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_disk_service, create_user_fee_debug) {
    xaction_source_null source_null;
    source_null.serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_create_user_account create_user;
    create_user.serialze_to(m_trans->get_transaction()->get_target_action(), m_target_account);
    xtransaction_create_user_account transfer_tgas(m_source_context.get(), m_trans);
    int32_t ret = transfer_tgas.parse();
    EXPECT_EQ(0, ret);

    int32_t fee = transfer_tgas.source_fee_exec();
    EXPECT_EQ(0, fee);
    ret = transfer_tgas.source_action_exec();
    EXPECT_EQ(0, ret);
    ret = transfer_tgas.target_action_exec();
    EXPECT_EQ(0, ret);
}
