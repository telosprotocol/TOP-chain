#include <gtest/gtest.h>

#include "tests/xcontract_vm/xaccount_vm_fixture.h"
#include "tests/xcontract_vm/xdemo_contract/xdummy_contract.h"
#include "xbase/xmem.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_vm/xaccount_vm.h"
#include "xcrypto/xckey.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_v2.h"
#include "xpbase/base/top_utils.h"
#include "xvledger/xvcanvas.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvtxindex.h"

#include <string>

NS_BEG3(top, tests, contract_vm)

static const std::string user_address{"T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks"};
static const std::string public_key{"BFqS6Al19LkycuHhrHMuI/E1G6+rZi4NJTQ1w1U55UnMjhBnb8/ey4pj+Mn69lyVB0+r6GR6M6eett9Tv/yoizI="};
static const std::string sign_key{"NzjQLs3K3stpskP8j1VG5DKwZF2vvBJNLDaHAvxsFQA="};

TEST_F(test_contract_vm, account_vm_BENCH) {
    uint64_t last_nonce{10};
    const uint256_t last_hash{12345678};
    const uint64_t recv_num{1};
    const uint64_t unconfirm_num{1};
    const uint64_t start_balance{std::numeric_limits<int64_t>::max() - 10};


    m_manager->deploy_system_contract<tests::system_contracts::xdummy_contract_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));

    // tx
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->make_tx_run_contract("do_nothing", "");
    tx->set_source_action_name("send_only");
    tx->set_different_source_target_address(user_address, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
    tx->set_digest();
    utl::xecprikey_t pri_key_obj((uint8_t *)(DecodePrivateString(sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    auto cons_tx = make_object_ptr<top::data::xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(base::enum_transaction_subtype_send);

    const uint256_t tx_hash = cons_tx->get_tx_hash_256();

    std::vector<data::xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    auto bstate = make_object_ptr<base::xvbstate_t>(user_address, 1, 0, "", "", 0, 0, 0);
    auto bstate_cmp = make_object_ptr<base::xvbstate_t>(user_address, 1, 0, "", "", 0, 0, 0);
    {
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        if (bstate->find_property(data::XPROPERTY_TX_INFO) == false) {
            bstate->new_string_map_var(data::XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_cmp->find_property(data::XPROPERTY_TX_INFO) == false) {
            bstate_cmp->new_string_map_var(data::XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE) == false) {
            bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
            bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(start_balance), canvas.get());
        }
        if (bstate_cmp->find_property(data::XPROPERTY_BALANCE_AVAILABLE) == false) {
            bstate_cmp->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
            bstate_cmp->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(start_balance), canvas.get());
        }
        if (bstate->find_property(data::XPROPERTY_BALANCE_BURN) == false) {
            bstate->new_token_var(data::XPROPERTY_BALANCE_BURN, canvas.get());
        }
        if (bstate_cmp->find_property(data::XPROPERTY_BALANCE_BURN) == false) {
            bstate_cmp->new_token_var(data::XPROPERTY_BALANCE_BURN, canvas.get());
        }
        if (bstate->find_property(data::XPROPERTY_BALANCE_LOCK) == false) {
            bstate->new_token_var(data::XPROPERTY_BALANCE_LOCK, canvas.get());
        }
        if (bstate_cmp->find_property(data::XPROPERTY_BALANCE_LOCK) == false) {
            bstate_cmp->new_token_var(data::XPROPERTY_BALANCE_LOCK, canvas.get());
        }
        auto map_property = bstate->load_string_map_var(data::XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_cmp->load_string_map_var(data::XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<std::string>(top::to_string(last_nonce));
            map_property->insert(data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(data::XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(data::XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<std::string>(top::to_string(recv_num));
            map_property->insert(data::XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(data::XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<std::string>(top::to_string(unconfirm_num));
            map_property->insert(data::XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(data::XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }
    for (auto i = 0; i < 10000; ++i) {
        tx->set_last_nonce(last_nonce++);
        tx->set_digest();
        tx->set_len();

        top::contract_vm::xaccount_vm_t vm(make_observer(m_manager));
        auto result = vm.execute(input_txs, make_observer(bstate.get()), cs_para);

        EXPECT_EQ(result.status.ec.value(), 0);
        EXPECT_EQ(result.success_tx_assemble.size(), 1);
        EXPECT_EQ(result.success_tx_assemble[0]->get_transaction(), tx.get());
        EXPECT_EQ(result.success_tx_assemble[0]->get_current_exec_status(), data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
        EXPECT_EQ(result.failed_tx_assemble.size(), 0);
        EXPECT_EQ(result.delay_tx_assemble.size(), 0);
    }
}

NS_END3
