#include "gtest/gtest.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xdata/xaction_parse.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xconfig/xconfig_register.h"
#include "xchain_timer/xchain_timer.h"
#include "xloader/xconfig_onchain_loader.h"
#include <algorithm>
#include "test_xtxexecutor_util.hpp"

using namespace top::txexecutor;
using namespace top::data;
using namespace top;
using namespace top::store;

class test_tgas_service : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_source_context->set_lock_token_sum(XGET_CONFIG(min_account_deposit));
        m_source_context->set_context_para(1, "111", 1, 1);  // set demo para
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());
        m_target_context->set_context_para(1, "111", 1, 1);  // set demo para
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_deposit(100000);
        tx->set_digest();

        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    std::string m_source_account{"T-3-11111111111111111"};
    std::string m_target_account{"T-3-11111111111111111"};
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    xcons_transaction_ptr_t             m_trans;
};

TEST_F(test_tgas_service, pledge_no_balance) {
    m_source_context->get_blockchain()->set_balance(0);
    data::xproperty_asset asset_out{10};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_pledge_token_tgas pledge_token_tgas(m_source_context.get(), m_trans);

    pledge_token_tgas.parse();
    pledge_token_tgas.check();
    int32_t ret = pledge_token_tgas.source_fee_exec();
    EXPECT_EQ(xconsensus_service_error_balance_not_enough, ret);
    ret = pledge_token_tgas.target_action_exec();
    EXPECT_NE(0, ret);
}

TEST_F(test_tgas_service, pledge_too_many) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    data::xproperty_asset asset_out{ASSET_TOP(10000000)};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_pledge_token_tgas pledge_token_tgas(m_source_context.get(), m_trans);

    pledge_token_tgas.parse();
    pledge_token_tgas.check();
    int32_t ret = pledge_token_tgas.target_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_tgas_service, DISABLED_pledge_bigger_than_available_tgas) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    m_source_context->get_blockchain()->set_tgas_balance(ASSET_TOP(10000));
    EXPECT_EQ(m_source_context->get_total_tgas(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract));
}

TEST_F(test_tgas_service, illegal_parent) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    data::xproperty_asset asset_out{ASSET_TOP(10000000)};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr("illegal_account");
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_pledge_token_tgas pledge_token_tgas(m_source_context.get(), m_trans);

    pledge_token_tgas.parse();
    int32_t ret = pledge_token_tgas.source_action_exec();
    EXPECT_EQ(0, ret);
    auto sum_token = m_source_context->get_blockchain()->tgas_balance();
    EXPECT_EQ(ASSET_TOP(0), sum_token);
    EXPECT_EQ(ASSET_TOP(0), m_target_context->get_tgas_balance_change());
    ret = pledge_token_tgas.target_action_exec();
    EXPECT_EQ(store::xaccount_property_parent_account_not_exist, ret);

    m_target_context->string_set(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, "null", true);
    xtransaction_pledge_token_tgas pledge_token_tgas1(m_target_context.get(), m_trans);
    pledge_token_tgas1.parse();
    ret = pledge_token_tgas1.target_action_exec();
    EXPECT_EQ(xtransaction_contract_pledge_token_source_target_mismatch, ret);
}
// TODO(jimmy)
TEST_F(test_tgas_service, DISABLED_legal_parent) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    data::xproperty_asset asset_out{ASSET_TOP(10000000)};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr("legal_account");
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    m_target_context->get_blockchain()->set_balance(ASSET_TOP(1));
    m_target_context->string_set(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, m_trans->get_transaction()->get_source_action().get_account_addr(), true);
    xtransaction_pledge_token_tgas pledge_token_tgas(m_target_context.get(), m_trans);
    pledge_token_tgas.parse();
    int32_t ret = pledge_token_tgas.target_action_exec();
    // TODO(wish)
    // EXPECT_EQ(xtransaction_not_enough_token, ret);
    EXPECT_EQ(3, ret);
}

TEST_F(test_tgas_service, owner_pledge) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    data::xproperty_asset asset_out{ASSET_TOP(1)};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr("legal_account");
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    m_target_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_target_context->string_set(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, m_trans->get_transaction()->get_source_action().get_account_addr(), true);
    xtransaction_pledge_token_tgas pledge_token_tgas(m_target_context.get(), m_trans);
    pledge_token_tgas.parse();
    int32_t ret = pledge_token_tgas.target_action_exec();
    EXPECT_EQ(0, ret);
    auto sum_token = m_target_context->get_blockchain()->tgas_balance();
    EXPECT_EQ(asset_out.m_amount, -m_target_context->get_balance_change());
    EXPECT_EQ(asset_out.m_amount, sum_token);
    EXPECT_EQ(asset_out.m_amount, m_target_context->get_tgas_balance_change());
}
// TODO(jimmy)
TEST_F(test_tgas_service, DISABLED_pledge_enough_balance) {
    uint64_t balance = ASSET_TOP(0.2);
    m_source_context->get_blockchain()->set_balance(balance);
    uint16_t count = 0;
    while (++count < 5) {
        const uint16_t pledge_token = 10;
        data::xproperty_asset asset_out{pledge_token};
        xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
        xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
        m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
        m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

        xtransaction_pledge_token_tgas pledge_token_tgas(m_source_context.get(), m_trans);

        pledge_token_tgas.parse();
        pledge_token_tgas.check();
        int32_t ret = pledge_token_tgas.source_action_exec();
        EXPECT_EQ(0, ret);

        auto sum_token = m_source_context->get_blockchain()->tgas_balance();
        EXPECT_EQ(pledge_token * count, sum_token);
        EXPECT_EQ(balance - pledge_token * count, m_source_context->get_blockchain()->balance() + m_source_context->get_balance_change());
    }
}
// TODO(jimmy)
TEST_F(test_tgas_service, DISABLED_redeem_no_token) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(100000));
    data::xproperty_asset asset_out{100};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_redeem_token_tgas redeem_token_tgas(m_source_context.get(), m_trans);

    redeem_token_tgas.parse();
    redeem_token_tgas.check();
    int32_t ret = redeem_token_tgas.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = redeem_token_tgas.target_action_exec();
    EXPECT_NE(0, ret);
}

TEST_F(test_tgas_service, redeem_token) {
    const uint16_t pledge_token = 11;
    const uint16_t redeem_token = 5;
    {
        m_source_context->get_blockchain()->set_balance(ASSET_TOP(0.2));
        data::xproperty_asset asset_out{pledge_token};
        xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
        xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
        m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
        m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

        xtransaction_pledge_token_tgas pledge_token_tgas(m_source_context.get(), m_trans);

        pledge_token_tgas.parse();
        pledge_token_tgas.check();
        int32_t ret = pledge_token_tgas.target_action_exec();
        EXPECT_EQ(0, ret);
        auto sum_token = m_source_context->get_blockchain()->tgas_balance();
        EXPECT_EQ(pledge_token, sum_token);
        auto tgas_change = m_source_context->get_tgas_balance_change();
        EXPECT_EQ(tgas_change, sum_token);
    }

    {
        data::xproperty_asset asset_out{redeem_token};
        data::xaction_asset_out action_asset_out;
        xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
        xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_redeem_token);
        m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
        m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

        xtransaction_redeem_token_tgas redeem_token_tgas(m_source_context.get(), m_trans);

        redeem_token_tgas.parse();
        redeem_token_tgas.check();
        int32_t ret = redeem_token_tgas.target_action_exec();
        EXPECT_EQ(0, ret);
        auto sum_token = m_source_context->get_blockchain()->tgas_balance();
        EXPECT_EQ(pledge_token - redeem_token, sum_token);
        auto tgas_change = m_source_context->get_tgas_balance_change();
        EXPECT_EQ(tgas_change, pledge_token - redeem_token);
    }
}

TEST_F(test_tgas_service, owner_redeem_token) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000000));
    data::xproperty_asset asset_out{ASSET_TOP(30)};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_redeem_token);
    m_trans->get_transaction()->get_source_action().set_account_addr("legal_account");
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    m_target_context->get_blockchain()->set_tgas_balance(ASSET_TOP(100));
    m_target_context->string_set(XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY, m_trans->get_transaction()->get_source_action().get_account_addr(), true);
    xtransaction_redeem_token_tgas redeem_token_tgas(m_target_context.get(), m_trans);
    redeem_token_tgas.parse();
    int32_t ret = redeem_token_tgas.target_action_exec();
    EXPECT_EQ(0, ret);
    auto sum_token = m_target_context->get_blockchain()->tgas_balance();
    EXPECT_EQ(ASSET_TOP(70), sum_token);
    EXPECT_EQ(ASSET_TOP(30), -m_target_context->get_tgas_balance_change());
}

TEST_F(test_tgas_service, transfer_no_tgas) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(100));

    data::xproperty_asset asset_out{100};
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    xaction_pledge_token::serialze_to(m_trans->get_transaction()->get_target_action(), asset_out, xaction_type_pledge_token);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_transfer transfer_tgas(m_source_context.get(), m_trans);

    transfer_tgas.parse();
    transfer_tgas.check();
    int32_t ret = transfer_tgas.source_fee_exec();
    EXPECT_EQ(0, ret);
}
// TODO(jimmy)
TEST_F(test_tgas_service, DISABLED_TOP_1535) {
    m_source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset) - 1);
    int32_t mock_used_tgas{800};
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, std::to_string(mock_used_tgas), true);

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    action_asset_out.serialze_to(m_trans->get_transaction()->get_target_action(), asset_out);
    m_trans->get_transaction()->get_source_action().set_account_addr("T0000011111111111111111");
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_transfer transfer_tgas(m_source_context.get(), m_trans);

    transfer_tgas.parse();
    int32_t ret = transfer_tgas.source_fee_exec();
    EXPECT_EQ(0, ret);
    auto used_tgas = m_source_context->get_used_tgas();
    EXPECT_EQ(used_tgas, mock_used_tgas);
    auto used_deposit = m_trans->get_current_used_deposit();
    auto tgas_usage = transfer_tgas.get_tx_fee().get_tgas_usage(true);
    EXPECT_EQ(used_deposit, tgas_usage * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
}

// sender consumes no resources when executing sys contract
TEST_F(test_tgas_service, TOP_1513) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_trans->get_transaction()->set_deposit(ASSET_TOP(0.4));

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
    // dc.serialze_to(m_trans->get_transaction()->get_target_action(), 0, code);
    xtransaction_create_contract_account tx_dc(m_source_context.get(), m_trans);
    tx_dc.target_action_exec();

    data::xaction_run_contract run_con;
    run_con.serialze_to(m_trans->get_transaction()->get_target_action(), "set_property", "");

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(100);
    state.set_used_deposit(300);
    // m_trans->m_last_action_send_tx_lock_tgas = 100;
    // m_trans->m_recv_tx_use_send_tx_tgas = 200;
    // m_trans->m_tx_last_action_used_res.m_used_deposit = 300;
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_run_contract tx_rc(m_source_context.get(), recvtx);
    auto ret = tx_rc.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(m_trans->get_current_send_tx_lock_tgas(), 0);
    EXPECT_EQ(m_trans->get_current_recv_tx_use_send_tx_tgas(), 0);
    EXPECT_EQ(m_trans->get_current_used_deposit(), 0);
    // EXPECT_EQ(m_trans->m_send_tx_lock_tgas, 100);
    // EXPECT_EQ(m_trans->m_recv_tx_use_send_tx_tgas, 0);
    // EXPECT_EQ(m_trans->m_used_res.m_used_deposit, 300);
}

TEST_F(test_tgas_service, TOP_3460) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_trans->get_transaction()->set_deposit(ASSET_TOP(0.1));

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");

    // dc.serialze_to(m_trans->get_transaction()->get_target_action(), 0, code);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(20);
    // deal used tgas 62, 99200 = 100000 - (40 * 20), 40 is deposit used tgas
    state.set_used_deposit(99200);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account tx_dc(m_source_context.get(), recvtx);
    auto ret = tx_dc.target_action_exec();
    EXPECT_EQ(store::xtransaction_contract_not_enough_tgas, ret);
}

TEST_F(test_tgas_service, deploy_contract_illegal) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_trans->get_transaction()->set_deposit(ASSET_TOP(0.4));

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key_illegal('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_illegal_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");

    // dc.serialze_to(m_trans->get_transaction()->get_target_action(), 0, code);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(4000);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account tx_dc(m_source_context.get(), recvtx);
    auto ret = tx_dc.target_action_exec();
    EXPECT_EQ(static_cast<int32_t>(xvm::enum_xvm_error_code::enum_lua_code_pcall_error), ret);
    EXPECT_EQ(m_trans->get_current_send_tx_lock_tgas(), m_trans->get_last_action_send_tx_lock_tgas());
}

TEST_F(test_tgas_service, run_contract_illegal) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_trans->get_transaction()->set_deposit(ASSET_TOP(0.4));

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
");
    // dc.serialze_to(m_trans->get_transaction()->get_target_action(), 0, code);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);
    xtransaction_create_contract_account tx_dc(m_source_context.get(), m_trans);
    auto ret = tx_dc.target_action_exec();
    EXPECT_EQ(0, ret);

    data::xaction_run_contract run_con;
    run_con.serialze_to(m_trans->get_transaction()->get_target_action(), "set_property", "");

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(4000);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_run_contract tx_rc(m_source_context.get(), recvtx);
    ret = tx_rc.parse();
    EXPECT_EQ(ret, 0);

    ret = tx_rc.target_action_exec();
    EXPECT_EQ(static_cast<int32_t>(xvm::enum_xvm_error_code::enum_lua_code_pcall_error), ret);
    EXPECT_EQ(m_trans->get_current_send_tx_lock_tgas(), m_trans->get_last_action_send_tx_lock_tgas());
}

// TODO(jimmy) this unit test should be modified
TEST_F(test_tgas_service, DISABLED_run_contract) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    m_trans->get_transaction()->set_deposit(ASSET_TOP(0.4));

    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);
    // dc.serialze_to(m_trans->get_transaction()->get_target_action(), 0, code);
    xtransaction_create_contract_account tx_dc(m_source_context.get(), m_trans);
    tx_dc.target_action_exec();

    // m_trans->get_transaction()->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
    data::xaction_run_contract run_con;
    run_con.serialze_to(m_trans->get_transaction()->get_target_action(), "set_property", "");


    xtransaction_run_contract tx_rc(m_source_context.get(), m_trans);
    auto ret = tx_rc.parse();
    EXPECT_EQ(ret, 0);

    ret = tx_rc.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, m_source_context->get_balance_change());

    xtransaction_exec_state_t state;
    state.set_tx_exec_status(enum_xunit_tx_exec_status_fail);
    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(m_trans->get_transaction(), state);

    xtransaction_run_contract tx_confirm(m_source_context.get(), confirmtx);
    ret = tx_confirm.parse();
    EXPECT_EQ(ret, 0);

    //m_source_context->get_blockchain()->set_lock_deposit_balance(asset_out.m_amount);
    ret = tx_confirm.source_confirm_fee_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, -m_source_context->get_lock_balance_change());
}

TEST_F(test_tgas_service, transfer_use_deposit) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(400));
    m_trans->get_transaction()->set_deposit(400000);

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

TEST_F(test_tgas_service, gift_tgas) {
    m_source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset) - 1);
    auto tgas = m_source_context->get_blockchain()->get_free_tgas();
    EXPECT_EQ(tgas, 0);

    m_source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset));
    tgas = m_source_context->get_blockchain()->get_free_tgas();
    EXPECT_EQ(tgas, XGET_ONCHAIN_GOVERNANCE_PARAMETER(free_gas));
}

// TODO(jimmy) this unit test should be modified
TEST_F(test_tgas_service, DISABLED_deploysc_no_pledge_tgas) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T0000011111111111111111"};
    std::string target_account{"T-3-11111111111111112"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());
    source_context->set_context_para(1, "111", 1, 1);  // set demo para
    auto target_context = std::make_shared<xaccount_context_t>(target_account, store.get());
    target_context->set_context_para(1, "111", 1, 1);  // set demo para

    // set account with no free tgas
    source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset) - 1);

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(tx->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");
    // dc.serialze_to(tx->get_target_action(), 0, code);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_create_contract_account tx_dc(source_context.get(), trans);
    tx_dc.parse();
    tx_dc.check();
    int32_t ret = tx_dc.source_fee_exec();
    EXPECT_EQ(ret, 0);
    // no free tgas should use deposit
    EXPECT_EQ(tx->get_deposit(), -source_context->get_balance_change());
    EXPECT_EQ(tx->get_deposit(), source_context->get_lock_balance_change());
    EXPECT_EQ(0, source_context->get_lock_tgas_change());

    auto used_tgas = tx_dc.get_tx_fee().get_tgas_usage(true);
    EXPECT_EQ(used_tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio), trans->get_current_used_deposit());
    EXPECT_EQ(0, trans->get_current_used_tgas());
    EXPECT_EQ(0, trans->get_current_send_tx_lock_tgas());

    ret = tx_dc.source_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(tx->get_deposit() + asset_out.m_amount, -source_context->get_balance_change());

    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(tx.get(), trans->get_tx_execute_state());
    EXPECT_EQ(0, target_context->get_balance_change());
    xtransaction_create_contract_account recvtx_dc(target_context.get(), recvtx);
    ret = recvtx_dc.parse();
    EXPECT_EQ(0, ret);
    ret = recvtx_dc.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, target_context->get_balance_change());
    EXPECT_EQ((used_tgas + 62) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio), recvtx->get_current_used_deposit());

    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(tx.get(), recvtx->get_tx_execute_state());
    xtransaction_create_contract_account confirmtx_dc(source_context.get(), confirmtx);
    ret = confirmtx_dc.source_confirm_fee_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ((used_tgas + 62) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio) + asset_out.m_amount, -source_context->get_balance_change());
    EXPECT_EQ(0, source_context->get_lock_balance_change());
    EXPECT_EQ(0, source_context->get_lock_tgas_change());
}

// TODO(jimmy)
TEST_F(test_tgas_service, DISABLED_deploysc_pledge_tgas) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T0000011111111111111111"};
    std::string target_account{"T-3-11111111111111112"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());
    source_context->set_context_para(1, "111", 1, 1);  // set demo para
    auto target_context = std::make_shared<xaccount_context_t>(target_account, store.get());
    target_context->set_context_para(1, "111", 1, 1);  // set demo para

    // set account with no free tgas
    source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset) - ASSET_TOP(2));
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(tx->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");
    // dc.serialze_to(tx->get_target_action(), 0, code);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_create_contract_account tx_dc(source_context.get(), trans);

    // use pledge token to get tgas
    source_context->set_pledge_token_tgas(ASSET_TOP(0.1));
    tx_dc.parse();
    tx_dc.check();
    int32_t ret = tx_dc.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(trans->get_transaction()->get_deposit(), -source_context->get_balance_change());
    EXPECT_EQ(trans->get_transaction()->get_deposit(), source_context->get_lock_balance_change());
    auto used_tgas = tx_dc.get_tx_fee().get_tgas_usage(true);
    auto available_tgas = source_context->get_available_tgas();
    EXPECT_EQ(available_tgas, source_context->get_blockchain()->tgas_balance() * source_context->get_token_price() / TOP_UNIT  - used_tgas);
    auto tx_lock_tgas = std::min(available_tgas, (uint64_t)((trans->get_transaction()->get_deposit() - trans->get_current_used_deposit()) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio)));
    EXPECT_EQ(tx_lock_tgas, source_context->get_lock_tgas_change());

    EXPECT_EQ(used_tgas, source_context->get_used_tgas());
    EXPECT_EQ(used_tgas, trans->get_current_used_tgas());
    EXPECT_EQ(tx_lock_tgas, trans->get_current_send_tx_lock_tgas());

    ret = tx_dc.source_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(trans->get_transaction()->get_deposit() + asset_out.m_amount, -source_context->get_balance_change());


    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(tx.get(), trans->get_tx_execute_state());
    EXPECT_EQ(0, target_context->get_balance_change());
    xtransaction_create_contract_account recvtx_dc(target_context.get(), recvtx);
    ret = recvtx_dc.parse();
    EXPECT_EQ(0, ret);
    ret = recvtx_dc.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, target_context->get_balance_change());
    EXPECT_EQ(0, m_trans->get_current_used_deposit());
    EXPECT_EQ(62, recvtx->get_current_recv_tx_use_send_tx_tgas());

    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(tx.get(), recvtx->get_tx_execute_state());
    xtransaction_create_contract_account confirmtx_dc(source_context.get(), confirmtx);
    ret = confirmtx_dc.source_confirm_fee_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, -source_context->get_balance_change());
    EXPECT_EQ(0, source_context->get_lock_balance_change());
    EXPECT_EQ(0, source_context->get_lock_tgas_change());
    EXPECT_EQ(used_tgas + 62, source_context->get_used_tgas());
}

TEST_F(test_tgas_service, DISABLED_deploysc_use_tgas_deposit) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T0000011111111111111111"};
    std::string target_account{"T-3-11111111111111112"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());
    source_context->set_context_para(1, "111", 1, 1);  // set demo para
    auto target_context = std::make_shared<xaccount_context_t>(target_account, store.get());
    target_context->set_context_para(1, "111", 1, 1);  // set demo para

    // set account with no free tgas
    source_context->get_blockchain()->set_balance(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset) - ASSET_TOP(2));
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(tx->get_source_action(), asset_out);
    data::xaction_deploy_contract dc;
    std::string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");
    // dc.serialze_to(tx->get_target_action(), 0, code);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_create_contract_account tx_dc(source_context.get(), trans);

    // pledge token equal to 871 = 860 + 11 tgas, 860 = tx_dc.get_tx_fee().get_tgas_usage(true) = tx_size * 2;
    source_context->set_pledge_token_tgas(872 * TOP_UNIT / source_context->get_token_price());
    tx_dc.parse();
    tx_dc.check();
    int32_t ret = tx_dc.source_fee_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(trans->get_transaction()->get_deposit(), -source_context->get_balance_change());
    EXPECT_EQ(trans->get_transaction()->get_deposit(), source_context->get_lock_balance_change());
    auto used_tgas = tx_dc.get_tx_fee().get_tgas_usage(true);
    auto available_tgas = source_context->get_available_tgas();
    EXPECT_EQ(available_tgas, (uint64_t)(source_context->get_blockchain()->tgas_balance() * source_context->get_token_price() / TOP_UNIT - used_tgas));
    auto tx_lock_tgas = std::min(available_tgas, (uint64_t)((trans->get_transaction()->get_deposit() - trans->get_current_used_deposit()) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio)));
    EXPECT_EQ(tx_lock_tgas, source_context->get_lock_tgas_change());
    EXPECT_NE(0, tx_lock_tgas);

    EXPECT_EQ(used_tgas, source_context->get_used_tgas());
    EXPECT_EQ(used_tgas, trans->get_current_used_tgas());
    EXPECT_EQ(tx_lock_tgas, trans->get_current_send_tx_lock_tgas());

    ret = tx_dc.source_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(trans->get_transaction()->get_deposit() + asset_out.m_amount, -source_context->get_balance_change());

    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(tx.get(), trans->get_tx_execute_state());
    EXPECT_EQ(0, target_context->get_balance_change());
    xtransaction_create_contract_account recvtx_dc(target_context.get(), recvtx);
    ret = recvtx_dc.parse();
    EXPECT_EQ(0, ret);
    ret = recvtx_dc.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(asset_out.m_amount, target_context->get_balance_change());
    EXPECT_EQ((62 - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio), recvtx->get_current_used_deposit());
    EXPECT_EQ(tx_lock_tgas, recvtx->get_current_recv_tx_use_send_tx_tgas());

    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(tx.get(), recvtx->get_tx_execute_state());
    xtransaction_create_contract_account confirmtx_dc(source_context.get(), confirmtx);
    ret = confirmtx_dc.source_confirm_fee_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ((62 - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio) + asset_out.m_amount, -source_context->get_balance_change());
    EXPECT_EQ(0, source_context->get_lock_balance_change());
    EXPECT_EQ(0, source_context->get_lock_tgas_change());
    EXPECT_EQ(used_tgas + tx_lock_tgas, source_context->get_used_tgas());
}

