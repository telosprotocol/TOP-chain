#define protected public
#define private public
#include "test_gasfee_fixture.h"
#include "xdata/src/xnative_contract_address.cpp"
#include "xgasfee/xerror/xerror.h"
#include "xtxexecutor/xatomictx_executor.h"
#include "xtxexecutor/xtvm_v2.h"
#include "xtxexecutor/xvm_face.h"

namespace top {
namespace tests {

TEST(test_gas, calculate_big_data) {
    uint64_t max_gasfee{0};
    auto price = evm_common::u256(1640847);
    auto limit = evm_common::u256(2500000032);
    auto eth_max_gasfee = limit * price * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio) / evm_common::u256(1e12);
    max_gasfee = static_cast<uint64_t>(eth_max_gasfee);
    EXPECT_EQ(eth_max_gasfee, max_gasfee);
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
#if 0
TEST_F(xtest_gasfee_fixture_t, test_v2_transfer_init_deposit_not_enough) {
    default_deposit = ASSET_uTOP(99999);
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_deposit_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v2_transfer_init_balance_not_enough) {
    default_balance = ASSET_uTOP(99999);
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v2_transfer_init_ok) {
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_free_tgas, default_free_tgas);
    EXPECT_EQ(op.m_converted_tgas, default_available_tgas - default_free_tgas);
    EXPECT_EQ(op.m_time, default_onchain_time);
    EXPECT_EQ(op.m_onchain_tgas_deposit, default_onchain_deposit_tgas);
}
#endif

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T6_check_deposit_not_enough) {
    default_tx_version = data::xtransaction_version_3;
    default_evm_gas_limit = 0;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.check(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_deposit_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T6_check_balance_not_enough) {
    default_balance = ASSET_uTOP(99999);
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.check(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T6_check_ok) {
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.check(ec);
}

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T6_init_ok) {
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_free_tgas, default_free_tgas);
    EXPECT_EQ(op.m_converted_tgas, default_available_tgas - default_free_tgas);
    EXPECT_EQ(op.m_time, default_onchain_time);
    EXPECT_EQ(op.m_onchain_tgas_deposit, default_onchain_deposit_tgas);
}

#if 0
TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T8_init_deposit_not_enough) {
    default_tx_version = data::xtransaction_version_3;
    default_v3_deposit = ASSET_uTOP(100000-1);
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_deposit_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T8_init_balance_not_enough) {
    default_balance = ASSET_uTOP(99999);
    default_tx_version = data::xtransaction_version_3;
    default_v3_deposit = default_deposit;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
}

TEST_F(xtest_gasfee_fixture_t, test_v3_transfer_T6_to_T8_init_ok) {
    default_tx_version = data::xtransaction_version_3;
    default_v3_deposit = default_deposit;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_free_tgas, default_free_tgas);
    EXPECT_EQ(op.m_converted_tgas, default_available_tgas - default_free_tgas);
    EXPECT_EQ(op.m_time, default_onchain_time);
    EXPECT_EQ(op.m_onchain_tgas_deposit, default_onchain_deposit_tgas);
}
#endif
TEST_F(xtest_gasfee_fixture_t, test_add_deposit_to_tgas_not_enough) {
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    auto new_usage = default_available_tgas + 1;
    op.add(new_usage, ec);
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_out_of_gas));
    EXPECT_EQ(op.m_converted_tgas_usage, default_deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(op.m_free_tgas_usage, default_free_tgas);
}

TEST_F(xtest_gasfee_fixture_t, test_add_use_all_free_tgas) {
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    uint64_t more_tgas = 1000;
    uint64_t new_usage = default_free_tgas + more_tgas;
    op.add(new_usage, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_converted_tgas_usage, more_tgas);
    EXPECT_EQ(op.m_free_tgas_usage, default_free_tgas);
}

TEST_F(xtest_gasfee_fixture_t, test_add_use_free_tgas) {
    default_tx_version = data::xtransaction_version_3;
    make_default();

    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    uint64_t less_tgas = 1000;
    uint64_t new_usage = default_free_tgas - less_tgas;
    op.add(new_usage, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(op.m_converted_tgas_usage, 0);
    EXPECT_EQ(op.m_free_tgas_usage, new_usage);
}
#if 0
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
#endif

TEST_F(xtest_gasfee_fixture_t, test_store_in_one_stage) {
    default_tx_version = data::xtransaction_version_3;
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.m_free_tgas_usage = op.m_free_tgas;
    op.m_converted_tgas_usage = uint64_t(op.tx_eth_limited_gasfee()) / 2 / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    op.store_in_one_stage();

    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, op.m_free_tgas);
    EXPECT_EQ(detail.m_tx_used_deposit, op.m_converted_tgas_usage * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account));
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, default_used_deposit);

    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), op.m_free_tgas);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), op.m_converted_tgas_usage * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_used_deposit));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(default_used_deposit));
}

TEST_F(xtest_gasfee_fixture_t, test_store_in_one_stage_over_limit) {
    default_tx_version = data::xtransaction_version_3;
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.m_free_tgas_usage = op.m_free_tgas;
    op.m_converted_tgas_usage = evm_common::u256(UINT64_MAX) * 10;
    op.m_eth_converted_tgas_usage = evm_common::u256(UINT64_MAX) * 10;
    op.store_in_one_stage();

    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, op.m_free_tgas);
    EXPECT_EQ(detail.m_tx_used_deposit, UINT64_MAX);
    EXPECT_EQ(detail.m_state_used_tgas, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account));
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, UINT64_MAX);
    EXPECT_EQ(detail.m_state_burn_eth_balance, op.utop_to_wei(op.tgas_to_balance(evm_common::u256(UINT64_MAX) * 10)));
}

#if 0
TEST_F(xtest_gasfee_fixture_t, test_store_send) {
    make_default();
    auto op = make_operator();
    std::error_code ec;
    op.init(ec);
    EXPECT_EQ(ec.value(), 0);
    op.m_free_tgas_usage = op.m_free_tgas;
    op.m_converted_tgas_usage = op.deposit() / 2 * 20;
    op.store_in_send_stage();

    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, op.m_free_tgas);
    EXPECT_EQ(detail.m_tx_used_deposit, op.m_converted_tgas_usage * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account));
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_lock_balance, default_deposit);
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), op.m_free_tgas);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), op.m_converted_tgas_usage * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_deposit));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(default_deposit));
}

TEST_F(xtest_gasfee_fixture_t, test_store_recv) {
    make_recv_default();
    auto op = make_recv_operator();
    std::error_code ec;
    op.store_in_recv_stage();
    EXPECT_EQ(ec.value(), 0);

    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_deposit, default_used_deposit);
    // EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), default_used_deposit);
}

TEST_F(xtest_gasfee_fixture_t, test_store_confirm) {
    make_confirm_default();
    auto op = make_confirm_operator();
    EXPECT_EQ(default_confirm_cons_tx->get_last_action_recv_tx_use_send_tx_tgas(), 0);
    EXPECT_EQ(default_confirm_cons_tx->get_last_action_used_deposit(), default_used_deposit);
    std::error_code ec;
    op.store_in_confirm_stage();
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, default_used_deposit);
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    // EXPECT_EQ(default_confirm_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)));
    // EXPECT_EQ(default_confirm_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), default_used_deposit);
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_transfer_not_use_deposit) {
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
    auto detail = op_send.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 525);
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    EXPECT_EQ(detail.m_state_used_tgas, 525 + 8842);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    default_cons_tx->set_current_used_tgas(525);
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 525);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 0);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(525 + 8842));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_recv.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    // EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), 0);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_confirm.gasfee_detail();
    // no exec
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(525 + 8842));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 0);
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_transfer_use_deposit) {
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
    auto detail = op_send.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    default_cons_tx->set_current_used_deposit(525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 525 * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_recv.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_deposit, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    default_recv_cons_tx->set_current_used_deposit(525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), 525 * 20);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_confirm.gasfee_detail();
    // no exec
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    EXPECT_EQ(detail.m_state_burn_balance, 0);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // // ignore transfer
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 0);
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_transfer_self) {
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
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 525 * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_transfer_inner_table) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    // send
    make_default();
    auto op = make_operator();
    default_cons_tx->set_inner_table_flag();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute self tx
    uint64_t supplement_gas = 0;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, 525 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 525 * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - 525 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(525 * 20));
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_run_contract) {
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
    auto tx_size = default_cons_tx->get_transaction()->get_tx_len();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute send tx
    uint64_t supplement_gas = 0;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
#ifndef XENABLE_MOCK_ZEC_STAKE
    EXPECT_EQ(detail.m_tx_used_deposit, (606 + tx_size) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
#endif
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_lock_balance, default_deposit);
#if defined(XENABLE_MOCK_ZEC_STAKE)
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
#else
    EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - ASSET_TOP(100)));
#endif
    default_cons_tx->set_current_used_deposit((606 + tx_size) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), (606 + tx_size) * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - default_deposit - ASSET_TOP(100)));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(default_deposit));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_recv.gasfee_detail();
    // empty
#ifndef XENABLE_MOCK_ZEC_STAKE
    EXPECT_EQ(detail.m_tx_used_deposit, (606 + tx_size) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
#endif
    default_recv_cons_tx->set_current_used_deposit((606 + tx_size) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));

    // EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), (606 + tx_size) * 20);
    // EXPECT_EQ(default_recv_cons_tx->get_current_recv_tx_use_send_tx_tgas(), 0);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_confirm.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
#ifndef XENABLE_MOCK_ZEC_STAKE
    EXPECT_EQ(detail.m_tx_used_deposit, (606 + tx_size) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
#endif
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    EXPECT_EQ(detail.m_state_unlock_balance, default_deposit);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - (606 + tx_size) * 20 - ASSET_TOP(100)));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), base::vtoken_t(0));
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), (606 + tx_size) * 20);
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v2_run_contract_self) {
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
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 0);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
}
#endif

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v3_deploy) {
    default_tx_version = data::xtransaction_version_3;
    default_tx_type = data::xtransaction_type_deploy_evm_contract;
    default_evm_gas_limit = 5000000;
    default_balance = ASSET_TOP(100000000);
    default_used_tgas = 1000000;
    default_last_time = 10000000;
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
    auto detail = op.gasfee_detail();
    auto tx_size = default_cons_tx->get_transaction()->get_tx_len();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, (1200000  + 3) * tx_size * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, (1200000 + 3) * tx_size * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), (200000 + 6) * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - (200000 + 6) * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t((200000 + 6) * 20));
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v3_transfer_inner_table_not_use_deposit) {
    default_evm_gas_limit = 5000000;
    default_balance = ASSET_TOP(100000000);
    default_tx_version = data::xtransaction_version_3;
    // send
    make_default();
    auto tx_size = default_cons_tx->get_transaction()->get_tx_len();
    default_cons_tx->set_inner_table_flag();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute tx
    uint64_t supplement_gas = 21000;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, default_free_tgas);
    EXPECT_EQ(detail.m_tx_used_deposit, (tx_size * 3 + supplement_gas * 80 - 991158) * 20);
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(),  tx_size * 3 + supplement_gas / 50);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), 0);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(8842 + tx_size * 3 + supplement_gas / 50));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance));
}

TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v3_transfer_inner_table_use_deposit) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    default_evm_gas_limit = 5000000;
    default_balance = ASSET_TOP(100000000);
    default_tx_version = data::xtransaction_version_3;
    // send
    make_default();
    auto tx_size = default_cons_tx->get_transaction()->get_tx_len();
    default_cons_tx->set_inner_table_flag();
    auto op = make_operator();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute tx
    uint64_t supplement_gas = 21000;
    op.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    auto used_deposit = supplement_gas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio) * 20 + tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, used_deposit);
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, used_deposit);
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), used_deposit);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - used_deposit));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(used_deposit));
}

#if 0
TEST_F(xtest_gasfee_fixture_t, gasfee_demo_v3_transfer_diff_table_use_deposit) {
    default_used_tgas = 1000000;
    default_last_time = 10000000;
    default_tx_version = data::xtransaction_version_3;
    default_v3_deposit = default_deposit * 2;
    // send
    make_default();
    auto tx_size = default_cons_tx->get_transaction()->get_tx_len();
    auto op_send = make_operator();
    std::error_code ec;
    op_send.preprocess(ec);
    EXPECT_EQ(op_send.m_converted_tgas, default_v3_deposit / 20);
    EXPECT_EQ(ec.value(), 0);
    // ... execute send tx
    uint64_t supplement_gas = 0;
    op_send.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    auto detail = op_send.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    EXPECT_EQ(detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    EXPECT_EQ(detail.m_state_burn_balance, tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    default_cons_tx->set_current_used_deposit(tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(default_cons_tx->get_current_used_deposit(), tx_size * 3 * 20);
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - tx_size * 3 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(tx_size * 3 * 20));

    // recv
    make_recv_cons_tx();
    auto op_recv = make_recv_operator();
    op_recv.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute recv tx
    op_recv.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_recv.gasfee_detail();
    EXPECT_EQ(detail.m_tx_used_deposit, tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    default_recv_cons_tx->set_current_used_deposit(tx_size * 3 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    // EXPECT_EQ(default_recv_cons_tx->get_current_used_deposit(), tx_size * 3 * 20);

    // confirm
    make_confirm_cons_tx();
    gasfee::xtop_gasfee op_confirm{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
    op_confirm.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    // ... execute confirm tx
    op_confirm.postprocess(supplement_gas, ec);
    EXPECT_EQ(ec.value(), 0);
    detail = op_confirm.gasfee_detail();
    // no exec
    EXPECT_EQ(detail.m_tx_used_tgas, 0);
    EXPECT_EQ(detail.m_tx_used_deposit, 0);
    EXPECT_EQ(detail.m_state_used_tgas, 0);
    EXPECT_EQ(detail.m_state_last_time, 0);
    EXPECT_EQ(detail.m_state_burn_balance, 0);

    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(1000000));
    // EXPECT_EQ(default_bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(default_onchain_time));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), base::vtoken_t(default_balance - tx_size * 3 * 20));
    // EXPECT_EQ(default_bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), base::vtoken_t(tx_size * 3 * 20));
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_tgas(), 0);
    // // ignore transfer
    // EXPECT_EQ(default_confirm_cons_tx->get_current_used_deposit(), 0);
}
#endif

TEST(test_xtvm_v2, xtvm2_demo_v3_T6_transfer_inner_table) {
    statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_t>();
    auto p_statectx = dynamic_cast<xmock_statectx_t *>(statectx.get());
    p_statectx->default_tx_version = data::xtransaction_version_3;
    p_statectx->default_evm_gas_limit = 5000000;
    p_statectx->default_balance = ASSET_TOP(100000000);
    p_statectx->default_used_tgas = 1000000;
    p_statectx->default_last_time = 10000000;
    p_statectx->default_sender = p_statectx->default_T6_sender;
    p_statectx->default_recver = p_statectx->default_T6_recver;
    p_statectx->build_default();
    base::xvaccount_t sender_vaccount{p_statectx->default_sender};
    base::xvaccount_t recver_vaccount{p_statectx->default_recver};
    auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
    auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
    sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, 20000);
    txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", p_statectx->default_onchain_deposit_tgas, 0};
    txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
    txexecutor::xatomictx_output_t output;

    EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute(p_statectx->default_cons_tx, output));

    auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(s_eth_balance, 20000 - p_statectx->default_eth_value);
    EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);

    auto tx_size = p_statectx->default_cons_tx->get_transaction()->get_tx_len();
    auto used_deposit = 21000 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio) * 20 + tx_size * 3 * 20;
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, used_deposit);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_tgas, 0);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, used_deposit);
    // EXPECT_EQ(p_statectx->default_cons_tx->get_current_used_tgas(), 0);
    // EXPECT_EQ(p_statectx->default_cons_tx->get_current_used_deposit(), used_deposit);
    // EXPECT_EQ(sender_unitstate->get_used_tgas(), 1000000);
    // EXPECT_EQ(sender_unitstate->balance(), base::vtoken_t(p_statectx->default_balance - used_deposit));
    // EXPECT_EQ(sender_unitstate->burn_balance(), base::vtoken_t(used_deposit));
}

TEST(test_xtvm_v2, xtvm2_demo_v3_T6_transfer_inner_table_use_eth) {
    statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_t>();
    auto p_statectx = dynamic_cast<xmock_statectx_t *>(statectx.get());
    p_statectx->default_tx_version = data::xtransaction_version_3;
    p_statectx->default_evm_gas_limit = 21000;
    p_statectx->default_balance = 10;
    p_statectx->default_used_tgas = 1000000;
    p_statectx->default_last_time = 10000000;
    p_statectx->default_sender = p_statectx->default_T6_sender;
    p_statectx->default_recver = p_statectx->default_T6_recver;
    p_statectx->build_default();
    base::xvaccount_t sender_vaccount{p_statectx->default_sender};
    base::xvaccount_t recver_vaccount{p_statectx->default_recver};
    auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
    auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
    sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, evm_common::u256(2000000000000000ULL));
    txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", p_statectx->default_onchain_deposit_tgas, UINT64_MAX};
    txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
    txexecutor::xatomictx_output_t output;

    EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute(p_statectx->default_cons_tx, output));

    auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(s_eth_balance, evm_common::u256(2000000000000000ULL) - p_statectx->default_eth_value);
    EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);

    uint64_t tx_size = p_statectx->default_cons_tx->get_transaction()->get_tx_len();
    uint64_t balance = 3 * tx_size + 21000 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio);
    evm_common::u256 eth_usage = gasfee::xgas_tx_operator_t::utop_to_wei(gasfee::xgas_tx_operator_t::tgas_to_balance(balance));
    // EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, 0);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_tgas, 0);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_used_tgas, 1000000);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, 0);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_eth_balance, eth_usage);
}

// TEST(test_xtvm_v2, xtvm2_demo_v3_transfer_diff_table) {
//     statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_t>();
//     auto p_statectx = dynamic_cast<xmock_statectx_t *>(statectx.get());
//     p_statectx->default_tx_version = data::xtransaction_version_3;
//     p_statectx->default_v3_deposit = p_statectx->default_deposit;
//     p_statectx->default_evm_gas_limit = 500000;
//     p_statectx->default_used_tgas = 1000000;
//     p_statectx->default_last_time = 10000000;
//     p_statectx->default_sender = p_statectx->default_T6_sender;
//     p_statectx->default_recver = p_statectx->default_T8_recver;
//     p_statectx->build_default();
//     base::xvaccount_t sender_vaccount{p_statectx->default_sender};
//     base::xvaccount_t recver_vaccount{p_statectx->default_recver};
//     auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
//     auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
//     sender_unitstate->tep_token_deposit(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH, 20000);
//     txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", p_statectx->default_onchain_deposit_tgas};
//     txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
//     txexecutor::xatomictx_output_t output;

//     EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute(p_statectx->default_cons_tx, output));
//     auto s_eth_balance = sender_unitstate->tep_token_balance(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH);
//     auto r_eth_balance = recver_unitstate->tep_token_balance(data::XPROPERTY_TEP1_BALANCE_KEY, data::XPROPERTY_ASSET_ETH);
//     EXPECT_EQ(s_eth_balance, 20000 - p_statectx->default_eth_value);
//     EXPECT_EQ(r_eth_balance, 0);

//     auto used_deposit = 21000 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio) * 20 + 6 * 20;
//     EXPECT_EQ(p_statectx->default_cons_tx->get_current_used_tgas(), 0);
//     EXPECT_EQ(p_statectx->default_cons_tx->get_current_used_deposit(), 6 * 20);
//     EXPECT_EQ(sender_unitstate->get_used_tgas(), 1000000);
//     EXPECT_EQ(sender_unitstate->balance(), base::vtoken_t(p_statectx->default_balance - 6 * 20));
//     // EXPECT_EQ(sender_unitstate->burn_balance(), base::vtoken_t(used_deposit));
// }

}  // namespace tests
}  // namespace top
