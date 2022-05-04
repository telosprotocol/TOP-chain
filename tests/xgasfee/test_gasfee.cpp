#include "test_gasfee_fixture.h"
#include "xdata/src/xnative_contract_address.cpp"
#include "xgasfee/xerror/xerror.h"

namespace top {
namespace tests {

TEST(test_gas_state_operator, test_burn) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.burn(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_BURN), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 10);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), 90);
}

TEST(test_gas_state_operator, test_lock) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.lock(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_LOCK), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 10);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), 90);
}

TEST(test_gas_state_operator, test_unlock) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());
    bstate->new_token_var(data::XPROPERTY_BALANCE_LOCK, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.unlock(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_LOCK), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 190);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), 10);
}

TEST(test_gas_state_operator, test_state_set_used_tgas) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.state_set_used_tgas(100, ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_USED_TGAS_KEY), true);
    EXPECT_EQ(bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(100));
}

TEST(test_gas_state_operator, test_state_set_last_time) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.state_set_last_time(10000000, ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_LAST_TX_HOUR_KEY), true);
    EXPECT_EQ(bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(10000000));
}

TEST(test_gas_state_operator, test_new_available_tgas_interface) {
    std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();

    uint64_t last_time = 9999000;
    uint64_t now_time = 10000000;
    uint64_t used_tgas = 10000;
    uint64_t onchain_deposit_tgas = ASSET_TOP(2000000000);

    bstate->new_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY, canvas.get());
    bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->reset(std::to_string(last_time), canvas.get());
    bstate->new_string_var(data::XPROPERTY_USED_TGAS_KEY, canvas.get());
    bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->reset(std::to_string(used_tgas), canvas.get());
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(base::vtoken_t(ASSET_TOP(1000)), canvas.get());
    bstate->new_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS)->deposit(base::vtoken_t(ASSET_TOP(1000)), canvas.get());

    std::shared_ptr<data::xunit_bstate_t> unit_state{std::make_shared<data::xunit_bstate_t>(bstate.get())};
    auto token_price = unit_state->get_token_price(onchain_deposit_tgas);
    auto legcy_available_tgas = unit_state->get_available_tgas(now_time, token_price);
    EXPECT_EQ(legcy_available_tgas, unit_state->available_tgas(now_time, onchain_deposit_tgas));
}

TEST_F(xtest_gasfee_fixture_t, test_init_deposit_not_enough) {
    default_deposit = ASSET_uTOP(99999);
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_deposit_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_init_ok) {
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_free_tgas, default_free_tgas);
    EXPECT_EQ(op.m_total_available_tgas, default_available_tgas);
    EXPECT_EQ(op.m_time, default_onchain_time);
    EXPECT_EQ(op.m_onchain_tgas_deposit, default_onchain_deposit_tgas);
}

TEST_F(xtest_gasfee_fixture_t, test_add_deposit_to_tgas_not_enough) {
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    auto new_usage = default_available_tgas + 1;
    op.add(new_usage, ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_deposit_to_tgas_not_enough));
    EXPECT_EQ(op.m_deposit_usage, default_deposit);
    EXPECT_EQ(op.m_free_tgas_usage, default_free_tgas);
}

TEST_F(xtest_gasfee_fixture_t, test_add_use_all_free_tgas) {
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    uint64_t more_tgas = 1000;
    uint64_t new_usage = default_free_tgas + more_tgas;
    op.add(new_usage, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_deposit_usage, more_tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(op.m_free_tgas_usage, default_free_tgas);
}

TEST_F(xtest_gasfee_fixture_t, test_add_use_free_tgas) {
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    uint64_t less_tgas = 1000;
    uint64_t new_usage = default_free_tgas - less_tgas;
    op.add(new_usage, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_deposit_usage, 0);
    EXPECT_EQ(op.m_free_tgas_usage, new_usage);
}

TEST_F(xtest_gasfee_fixture_t, test_undo_process_fixed_tgas1) {
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.process_fixed_tgas(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), default_balance);
}

TEST_F(xtest_gasfee_fixture_t, test_undo_process_fixed_tgas2) {
    default_sender = rec_registration_contract_address.value();
    default_recver = rec_registration_contract_address.value();
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.process_fixed_tgas(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), default_balance);
}

TEST_F(xtest_gasfee_fixture_t, test_do_process_fixed_tgas) {
    default_recver = rec_registration_contract_address.value();
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.process_fixed_tgas(ec);
    EXPECT_EQ(ec.value(), 0);
#ifdef XENABLE_MOCK_ZEC_STAKE
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), default_balance);
#else
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), default_balance - XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee));
#endif
}

TEST_F(xtest_gasfee_fixture_t, test_process_calculation_tgas_gas_over_limit) {
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    uint64_t gas_usage = static_cast<uint64_t>(default_evm_gas_limit) + 1;
    op.process_calculation_tgas(gas_usage, ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_calculation_gas_over_limit));
}

TEST_F(xtest_gasfee_fixture_t, test_process_calculation_tgas_exceeded) {
    // TODO: related tx_v3 interface not ready yet
}

TEST_F(xtest_gasfee_fixture_t, test_process_calculation_tgas_used_tgas_over_limit) {
    // TODO: related tx_v3 interface not ready yet
}

TEST_F(xtest_gasfee_fixture_t, test_store_in_one_stage) {
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.m_free_tgas_usage = op.m_free_tgas;
    op.m_deposit_usage = op.deposit() / 2;
    op.store_in_one_stage(ec);

    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), op.m_free_tgas);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), op.m_deposit_usage);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_used_deposit));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(default_used_deposit));
}

TEST_F(xtest_gasfee_fixture_t, test_store_send) {
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.m_free_tgas_usage = op.m_free_tgas;
    op.m_deposit_usage = op.deposit() / 2;
    op.store_in_send_stage(ec);

    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), op.m_free_tgas);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), op.m_deposit_usage);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_deposit));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(default_deposit));
}

TEST_F(xtest_gasfee_fixture_t, test_store_recv) {
    make_recv_default();
    auto op = make_recv_operator();
    std::error_code ec;
    op.store_in_recv_stage(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), default_used_deposit);
    EXPECT_EQ(default_recv_cons_tx->get_current_recv_tx_use_send_tx_tgas(), 0);
}

TEST_F(xtest_gasfee_fixture_t, test_store_confirm) {
    make_confirm_default();
    auto op = make_confirm_operator();
    EXPECT_EQ(default_confirm_cons_tx->get_last_action_recv_tx_use_send_tx_tgas(), 0);
    EXPECT_EQ(default_confirm_cons_tx->get_last_action_used_deposit(), default_used_deposit);
    std::error_code ec;
    op.store_in_confirm_stage(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_confirm_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    EXPECT_EQ(default_confirm_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), default_used_deposit);
}

TEST_F(xtest_gasfee_fixture_t, demo_common_transfer_not_use_deposit) {
    // send
    make_default();
    auto op_send = make_operator();
    std::error_code ec;
    op_send.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute send tx
    uint64_t supplement_gas = 0;
    op_send.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 525);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 0);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(525 + 8842));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), 0);
    EXPECT_EQ(default_recv_cons_tx->get_current_recv_tx_use_send_tx_tgas(), 0);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(525 + 8842));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 0);
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
}

TEST_F(xtest_gasfee_fixture_t, demo_common_transfer_use_deposit) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    // send
    make_default();
    auto op_send = make_operator();
    std::error_code ec;
    op_send.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute send tx
    uint64_t supplement_gas = 0;
    op_send.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 525 * 20);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), 525 * 20);
    EXPECT_EQ(default_recv_cons_tx->get_current_recv_tx_use_send_tx_tgas(), 0);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // ignore transfer
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 0);
}

TEST_F(xtest_gasfee_fixture_t, demo_common_transfer_self) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    default_recver = "T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc";
    // send
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute self tx
    uint64_t supplement_gas = 0;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 525 / 3 * 20);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 / 3 * 20));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 / 3 * 20));
}

TEST_F(xtest_gasfee_fixture_t, demo_common_run_contract) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    default_recver = top::rec_standby_pool_contract_address.value();
    // send
    make_bstate();
    make_unit_state(default_bstate);
    xobject_ptr_t<data::xtransaction_v2_t> tx{make_object_ptr<data::xtransaction_v2_t>()};
    data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, default_amount};
    tx->make_tx_run_contract("on_timer", "");
    tx->set_last_trans_hash_and_nonce(uint256_t(), uint64_t(0));
    tx->set_different_source_target_address(default_sender, default_recver);
    tx->set_fire_timestamp(default_fire);
    tx->set_expire_duration(default_expire);
    tx->set_deposit(default_deposit);
    tx->set_digest();
    utl::xecprikey_t pri_key_obj((uint8_t *)(DecodePrivateString(default_sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    default_tx_v2 = tx;
    make_cons_tx();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute send tx
    uint64_t supplement_gas = 0;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 606 * 20);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_deposit - ASSET_TOP(100)));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(default_deposit));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), 606 * 20);
    EXPECT_EQ(default_recv_cons_tx->get_current_recv_tx_use_send_tx_tgas(), 0);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 606 * 20 - ASSET_TOP(100)));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(0));
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 606 * 20);

}

TEST_F(xtest_gasfee_fixture_t, demo_common_contract_self) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    default_sender = top::zec_reward_contract_address.value();
    make_bstate();
    make_unit_state(default_bstate);
    xobject_ptr_t<data::xtransaction_v2_t> tx{make_object_ptr<data::xtransaction_v2_t>()};
    data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, default_amount};
    tx->make_tx_run_contract("on_timer", "");
    tx->set_last_trans_hash_and_nonce(uint256_t(), uint64_t(0));
    tx->set_same_source_target_address(default_sender);
    tx->set_fire_timestamp(default_fire);
    tx->set_expire_duration(default_expire);
    tx->set_deposit(0);
    tx->set_digest();
    utl::xecprikey_t pri_key_obj((uint8_t *)(DecodePrivateString(default_sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    default_tx_v2 = tx;
    make_cons_tx();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute self tx
    uint64_t supplement_gas = 0;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 0);
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
}

}  // namespace tests
}  // namespace top