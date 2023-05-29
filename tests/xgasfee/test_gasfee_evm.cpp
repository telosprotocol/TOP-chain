#include "test_gasfee_evm.h"
#include "xdata/xnative_contract_address.h"
#include "xgasfee/xerror/xerror.h"
#include "xtxexecutor/xatomictx_executor.h"
#include "xgasfee/xgas_estimate.h"


namespace top {
namespace tests {

TEST_F(xtest_gasfee_evm_fixture_t, test_gasfee_transer_ok) {
    default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();;
    make_default();
    auto op = make_operator_evm();
    std::error_code ec;
    op.preprocess(ec); 
    EXPECT_EQ(ec.value(), 0);
    op.postprocess(default_evm_gas_limit, ec); 
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_state_burn_balance, get_eth_utop());
    EXPECT_EQ(detail.m_tx_used_deposit, get_eth_utop());
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
}

TEST_F(xtest_gasfee_evm_fixture_t, test_gasfee_v3_deploy_ok) {
    default_tx_version = data::xtransaction_version_3;
    default_evm_gas_limit = 5000000;
    default_balance = ASSET_TOP(100000000);
    default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
    default_last_time = 10000000;
    // send
    make_default();
    auto op = make_operator_evm();
    std::error_code ec;
    op.preprocess(ec);
    EXPECT_EQ(ec.value(), 0);
    op.postprocess(default_evm_gas_used, ec);
    EXPECT_EQ(ec.value(), 0);
    auto detail = op.gasfee_detail();
    EXPECT_EQ(detail.m_state_burn_balance, get_eth_utop());
    EXPECT_EQ(detail.m_tx_used_deposit, get_eth_utop());
    EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_init_ec_tx_out_of_gas) {
    make_default();
    auto op = make_operator_evm();
    std::error_code ec;
    op.init(ec); 
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_out_of_gas));
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_init_ec_tx_priority_fee_error) {
    default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
    default_user_set_max_priority_fee = default_user_set_max_per_gas +1;
    make_default();
    auto op = make_operator_evm();
    std::error_code ec;
    op.init(ec); 
    EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_priority_fee_error));
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_init_ec_account_balance_enough) {
    //blance limit test
    for (int i = -1; i < 2; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
        default_balance = get_eth_utop() + i;
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.init(ec);
        if (i < 0) {
            //balance < gasfee
            EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
        } else {
            //balance >= gasfee
            EXPECT_EQ(ec.value(), 0);
        }
    }

    //gas_price limit test
    for (int i = 0; i < 2; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
        default_balance = get_eth_utop();
        default_user_set_max_per_gas += i * 200;  // 200 is the gas price step for utop to wei
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.init(ec);
        if (i > 0) {
            //gas_price > max_gas_price
            EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
        } else {
            //gas_price <= max_gas_price
            EXPECT_EQ(ec.value(), 0);
        }
    }

    //priority_fee limit test
    for (int i = 0; i < 2; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price() + i * 200;
        default_balance = get_eth_utop();
        default_user_set_max_priority_fee += i * 200;
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.init(ec);
        if (i > 0) {
            EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::account_balance_not_enough));
        } else {
            EXPECT_EQ(ec.value(), 0);
        }
    }
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_tx_priority_0) {
    
   //test eth_fee is constant while priority_fee=0
    uint64_t top_fee = get_eth_utop();
    for (int i = 0; i < 20; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price() * (1+i);
        default_balance = get_eth_utop() * (1+i);
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.preprocess(ec);
        EXPECT_EQ(ec.value(), 0);
        op.postprocess(default_evm_gas_used, ec);
        EXPECT_EQ(ec.value(), 0);
        auto detail = op.gasfee_detail();
        EXPECT_EQ(detail.m_state_burn_balance, top_fee);
        EXPECT_EQ(detail.m_tx_used_deposit, top_fee);
        EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
    }
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_tx_priority_not_0) {
    
   //test eth_fee is variation while priority_fee change
    uint64_t top_fee = get_eth_utop();
    for (int i = 0; i < 20; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price() * (1+i);
        default_user_set_max_priority_fee = gasfee::xgas_estimate::base_price() * (i);
        default_balance = get_eth_utop() * (1+i);
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.preprocess(ec);
        EXPECT_EQ(ec.value(), 0);
        op.postprocess(default_evm_gas_used, ec);
        EXPECT_EQ(ec.value(), 0);
        auto detail = op.gasfee_detail();
        EXPECT_GE(detail.m_state_burn_balance, top_fee);
        EXPECT_GE(detail.m_tx_used_deposit, top_fee);
        EXPECT_EQ(detail.m_state_burn_balance, get_eth_utop());
        EXPECT_EQ(detail.m_tx_used_deposit, get_eth_utop());
        EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
        evm_common::u256 priority_fee_price = std::min(default_user_set_max_priority_fee, (default_user_set_max_per_gas - gasfee::xgas_estimate::base_price()));
        EXPECT_EQ(op.m_priority_fee_price, priority_fee_price);
    }
}

TEST_F(xtest_gasfee_evm_fixture_t, test_v3_postprocess_gas_used_limit) {
    
    //gas_used limit test
    for (int i = -1; i < 2; i++) {
        default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
        default_balance = get_eth_utop();
        make_default();
        auto op = make_operator_evm();
        std::error_code ec;
        op.preprocess(ec);
        EXPECT_EQ(ec.value(), 0);
        op.postprocess(default_evm_gas_used+i, ec);
        if (i > 0) {
            //gas used > gas limit
            EXPECT_EQ(ec, make_error_code(gasfee::error::xenum_errc::tx_out_of_gas));
        } else if( i == 0) {
            //gas used == gas limit
            EXPECT_EQ(ec.value(), 0);
            auto detail = op.gasfee_detail();
            EXPECT_EQ(detail.m_state_burn_balance, default_balance);
            EXPECT_EQ(detail.m_tx_used_deposit, default_balance);
        } else {
            //gas used < gas limit
            EXPECT_EQ(ec.value(), 0);
            auto detail = op.gasfee_detail();
            EXPECT_LE(detail.m_state_burn_balance,default_balance);
            EXPECT_LE(detail.m_tx_used_deposit, default_balance);
            EXPECT_EQ(detail.m_state_last_time, default_onchain_time);
        }
    }
}

TEST(test_xtvm_v2_evm, xtvm2_demo_v3_T6_transfer_inner_table) {
    
    statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
    auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
    p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
    p_statectx->default_tx_version = data::xtransaction_version_3;
    p_statectx->default_evm_gas_limit = 5000000;
    p_statectx->default_balance = ASSET_TOP(100000000);
    p_statectx->default_last_time = 10000000;
    p_statectx->default_sender = p_statectx->default_T6_sender;
    p_statectx->default_recver = p_statectx->default_T6_recver;
    p_statectx->build_default();
    common::xaccount_address_t sender_vaccount{p_statectx->default_sender};
    common::xaccount_address_t recver_vaccount{p_statectx->default_recver};
    auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
    auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
    sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, 20000);
    txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", 0, 0, 1, eth_miner_zero_address};
    txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
    txexecutor::xatomictx_output_t output;

    EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));

    auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
    EXPECT_EQ(s_eth_balance, 20000 - p_statectx->default_eth_value);
    EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, p_statectx->get_eth_utop());
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_tgas, 0);
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, p_statectx->get_eth_utop());
}

}  // namespace tests
}  // namespace top
