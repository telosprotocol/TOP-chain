#include "test_gasfee_evm.h"
#include "xdata/xnative_contract_address.h"
#include "xgasfee/xerror/xerror.h"
#include "xgasfee/xgas_estimate.h"
#include "xtxexecutor/xatomictx_executor.h"

namespace top {
namespace tests {

TEST(test_tx_execute, test_tx_self_to_self_normal)
{    
    for(int send_value = 0; send_value < 2; send_value++) {
        statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
        auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
        p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
        p_statectx->default_tx_version = data::xtransaction_version_3;
        p_statectx->default_evm_gas_limit = 5000000;
        p_statectx->default_balance = ASSET_TOP(100000000);
        p_statectx->default_last_time = 10000000;
        p_statectx->default_sender = p_statectx->default_T6_sender;
        p_statectx->default_recver = p_statectx->default_T6_sender;
        p_statectx->default_eth_value = send_value;
        p_statectx->build_default();
        common::xaccount_address_t sender_vaccount{p_statectx->default_sender};
        common::xaccount_address_t recver_vaccount{p_statectx->default_sender};
        auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
        auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
        sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, 20000);
        txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", 0, 0, 1, eth_miner_zero_address};
        txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
        txexecutor::xatomictx_output_t output;

        EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));

        auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
        auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
        EXPECT_EQ(s_eth_balance, 20000);
        EXPECT_EQ(r_eth_balance, 20000);
        EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, p_statectx->get_eth_utop());
        EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, p_statectx->get_eth_utop());
        EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
    }
}

TEST(test_tx_execute, test_tx_self_to_other)
{
    statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
    auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
    p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price();
    p_statectx->default_tx_version = data::xtransaction_version_3;
    p_statectx->default_evm_gas_limit = 5000000;
    p_statectx->default_balance = ASSET_TOP(100000000);
    p_statectx->default_last_time = 10000000;
    p_statectx->default_sender = p_statectx->default_T6_sender;
    p_statectx->default_recver = p_statectx->default_T6_recver;
    p_statectx->default_eth_value = 100;
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
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, p_statectx->get_eth_utop());
    EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
}

TEST(test_tx_execute, test_tx_max_gas_price)
{    
    for(int i = -1; i < 3; i++) {
        statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
        auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
        p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price() + i * 200; 
        p_statectx->default_tx_version = data::xtransaction_version_3;
        p_statectx->default_evm_gas_limit = 5000000;
        p_statectx->default_balance =  ASSET_TOP(100000000);
        p_statectx->default_last_time = 10000000;
        p_statectx->default_sender = p_statectx->default_T6_sender;
        p_statectx->default_recver = p_statectx->default_T6_recver;
        p_statectx->default_eth_value = 100;
        p_statectx->build_default();
        common::xaccount_address_t sender_vaccount{p_statectx->default_sender};
        common::xaccount_address_t recver_vaccount{p_statectx->default_recver};
        auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
        auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
        //set eth balance
        sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, 20000);
        txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", 0, 0, 1, eth_miner_zero_address};
        txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
        txexecutor::xatomictx_output_t output;


        if(-1 == i) {
            // max-gas-price < base-price
            EXPECT_EQ(txexecutor::enum_exec_error_estimate_gas, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, 20000);
            EXPECT_EQ(r_eth_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
        } else {
            // max-gas-price >= base-price
            EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, 20000 - p_statectx->default_eth_value);
            EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, p_statectx->get_eth_utop());
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, p_statectx->get_eth_utop());
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
        }

    }
}

TEST(test_tx_execute, test_tx_priority_fee_price)
{    
    for(int i = 0; i < 3; i++) {
        statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
        auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
        p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price(); 
        p_statectx->default_user_set_max_priority_fee = gasfee::xgas_estimate::base_price() * (i);
        p_statectx->default_tx_version = data::xtransaction_version_3;
        p_statectx->default_evm_gas_limit = 5000000;
        p_statectx->default_balance =  ASSET_TOP(100000000);
        p_statectx->default_last_time = 10000000;
        p_statectx->default_sender = p_statectx->default_T6_sender;
        p_statectx->default_recver = p_statectx->default_T6_recver;
        p_statectx->default_eth_value = 100;
        p_statectx->build_default();
        common::xaccount_address_t sender_vaccount{p_statectx->default_sender};
        common::xaccount_address_t recver_vaccount{p_statectx->default_recver};
        auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
        auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
        sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, 20000);
        txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", 0, 0, 1, eth_miner_zero_address};
        txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
        txexecutor::xatomictx_output_t output;


        if(i < 2) {
            //  priority_fee_price <= max-gas-price 
            EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, 20000 - p_statectx->default_eth_value);
            EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, p_statectx->get_eth_utop());
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, p_statectx->get_eth_utop());
            EXPECT_LE(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, gasfee::xgas_estimate::base_price() * (i));
        } else {
            // priority_fee_price > max-gas-price 
            EXPECT_EQ(txexecutor::enum_exec_error_estimate_gas, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, 20000);
            EXPECT_EQ(r_eth_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
        }
    }
}

TEST(test_tx_execute, test_tx_balance)
{    
    for(int i = -1; i < 3; i++) {
        statectx::xstatectx_face_ptr_t statectx = std::make_shared<xmock_statectx_evm_t>();
        auto p_statectx = dynamic_cast<xmock_statectx_evm_t *>(statectx.get());
        p_statectx->default_user_set_max_per_gas = gasfee::xgas_estimate::base_price(); 
        p_statectx->default_tx_version = data::xtransaction_version_3;
        p_statectx->default_evm_gas_limit = 5000000;
        p_statectx->default_balance =  ASSET_TOP(100000000);
        p_statectx->default_last_time = 10000000;
        p_statectx->default_sender = p_statectx->default_T6_sender;
        p_statectx->default_recver = p_statectx->default_T6_recver;
        p_statectx->default_eth_value = 100;
        p_statectx->build_default();
        common::xaccount_address_t sender_vaccount{p_statectx->default_sender};
        common::xaccount_address_t recver_vaccount{p_statectx->default_recver};
        auto sender_unitstate = statectx->load_unit_state(sender_vaccount);
        auto recver_unitstate = statectx->load_unit_state(recver_vaccount);
        sender_unitstate->tep_token_deposit(common::xtoken_id_t::eth, p_statectx->default_eth_value + i);
        txexecutor::xvm_para_t param{p_statectx->default_onchain_time, "0000", 0, 0, 1, eth_miner_zero_address};
        txexecutor::xatomictx_executor_t atomictx_executor{statectx, param};
        txexecutor::xatomictx_output_t output;


        if(0 > i) {
            //  eth_balance < eth_send_value 
            EXPECT_EQ(txexecutor::enum_exec_error_estimate_gas, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, 99); //p_statectx->default_eth_value -1 
            EXPECT_EQ(r_eth_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance, 0);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
        } else {
            // priority_fee_price > max-gas-price 
            EXPECT_EQ(txexecutor::enum_exec_success, atomictx_executor.vm_execute_forked(p_statectx->default_cons_tx, output));
            auto s_eth_balance = sender_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            auto r_eth_balance = recver_unitstate->tep_token_balance(common::xtoken_id_t::eth);
            EXPECT_EQ(s_eth_balance, i);
            EXPECT_EQ(r_eth_balance, p_statectx->default_eth_value);
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_used_deposit, p_statectx->get_eth_utop());
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_state_burn_balance,p_statectx->get_eth_utop());
            EXPECT_EQ(output.m_vm_output.m_gasfee_detail.m_tx_priority_fee_price, 0);
        }
    }
}


} // namespace tests
} // namespace top