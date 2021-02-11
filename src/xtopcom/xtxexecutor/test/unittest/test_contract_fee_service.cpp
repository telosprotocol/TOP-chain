#include "gtest/gtest.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xdata/xaction_parse.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xloader/xconfig_onchain_loader.h"
#include "test_xtxexecutor_util.hpp"

using namespace top::txexecutor;
using namespace top::data;
using namespace top;
using namespace top::store;
using namespace top::base;
using std::string;

class test_contract_fee_service : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        uint16_t network_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        uint8_t  address_type = (uint8_t) base::enum_vaccount_addr_type_secp256k1_user_account;
        utl::xecprikey_t parent_pri_key_obj;
        utl::xecpubkey_t parent_pub_key_obj = parent_pri_key_obj.get_public_key();
        m_source_account = parent_pub_key_obj.to_address(address_type, network_id);

        address_type = (uint8_t) base::enum_vaccount_addr_type_custom_contract;
        //utl::xecprikey_t sub_pri_key_obj;
        utl::xecpubkey_t sub_pub_key_obj = sub_pri_key_obj.get_public_key();
        m_target_account = sub_pub_key_obj.to_address(m_source_account, address_type, network_id);

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_source_context->set_lock_token_sum(XGET_CONFIG(min_account_deposit));
        m_source_context->set_context_para(1, "111", 1, 1);  // set demo para
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());

        xstream_t stream_s(xcontext_t::instance());
        stream_s << uint64_t(0);
        string code("function init()\n\
        create_key('hello')\n\
    end\n\
    function set_property()\n\
        set_key('hello', 'world')\n\
        local value = get_key('hello')\n\
        print('value:' .. value or '')\n\
    end\n\
    ");

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset_out{ XGET_CONFIG(min_account_deposit) };
        tx->make_tx_create_contract_account(asset_out, 0, code);
        tx->set_different_source_target_address(m_source_account, m_target_account);
        tx->set_deposit(100000);
        tx->set_digest();

        uint256_t target_hash = tx->get_target_action().sha2();
        utl::xecdsasig_t signature = sub_pri_key_obj.sign(target_hash);
        string target_sign((char*)signature.get_compact_signature(), signature.get_compact_signature_size());
        tx->get_target_action().set_action_authorization(target_sign);

        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    std::string m_source_account;
    std::string m_target_account;
    utl::xecprikey_t sub_pri_key_obj;
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    xcons_transaction_ptr_t             m_trans;
};

TEST_F(test_contract_fee_service, source_no_tgas) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit));
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    parent_account.parse();
    parent_account.check();
    int32_t ret = parent_account.source_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_contract_fee_service, source_use_deposit_money) {
	uint32_t account_min_deposit = XGET_CONFIG(min_account_deposit);
    m_source_context->get_blockchain()->set_balance(account_min_deposit + 120000);
    //m_source_context->set_timer_height(1024);  // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);
	m_source_context->set_lock_token_sum(ASSET_TOP(300));

    m_trans->get_transaction()->set_deposit(120000);

    xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    parent_account.parse();
    parent_account.check();
    int32_t ret = parent_account.source_action_exec();
    EXPECT_EQ(0, ret);

    // xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    // sub_account.parse();
    // sub_account.check();
    // ret = sub_account.target_action_exec();
    // EXPECT_EQ(0, ret);
}
// TODO(jimmy)
TEST_F(test_contract_fee_service, DISABLED_source_deposit_not_enough) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + 100000);
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    m_trans->get_transaction()->set_deposit(100000);

    xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    parent_account.parse();
    parent_account.check();
    int32_t ret = parent_account.source_fee_exec();
    EXPECT_NE(0, ret);

    // xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    // sub_account.parse();
    // sub_account.check();
    // ret = sub_account.target_action_exec();
    // EXPECT_EQ(0, ret);
}


TEST_F(test_contract_fee_service, target_no_tgas) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit));
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    // xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    // parent_account.parse();
    // parent_account.check();
    // int32_t ret = parent_account.source_action_exec();

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(0);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account sub_account(m_target_context.get(), recvtx);
    m_target_context->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, "0", true);
    sub_account.parse();
    sub_account.check();
    auto ret = sub_account.target_action_exec();
    // TODO comment out temporarily
    // EXPECT_EQ(consensus_service::xtransaction_not_enough_pledge_token_tgas, ret);

    // xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    // sub_account.parse();
    // sub_account.check();
    // ret = sub_account.target_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_contract_fee_service, target_use_source_tgas) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit));
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(10000);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    // xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    // parent_account.parse();
    // parent_account.check();
    // int32_t ret = parent_account.source_action_exec();


    xtransaction_create_contract_account sub_account(m_target_context.get(), recvtx);
    m_target_context->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, "500", true);
    sub_account.parse();
    sub_account.check();
    auto ret = sub_account.target_action_exec();
    // TODO comment out temporarily
    // EXPECT_EQ(consensus_service::xtransaction_not_enough_pledge_token_tgas, ret);

    // xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    // sub_account.parse();
    // sub_account.check();
    // ret = sub_account.target_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_contract_fee_service, target_use_deposit_tgas) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit));
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);


    // xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    // parent_account.parse();
    // parent_account.check();
    // int32_t ret = parent_account.source_action_exec();

    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(500);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account sub_account(m_target_context.get(), recvtx);
    m_target_context->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, "500", true);
    sub_account.parse();
    sub_account.check();
    auto ret = sub_account.target_action_exec();
    // TODO comment out temporarily
    // EXPECT_EQ(consensus_service::xtransaction_not_enough_pledge_token_tgas, ret);

    // xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    // sub_account.parse();
    // sub_account.check();
    // ret = sub_account.target_action_exec();
    EXPECT_EQ(0, ret);
}



// TODO(jimmy)
TEST_F(test_contract_fee_service, DISABLED_target_disk_not_enough) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + 120000);
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    m_trans->get_transaction()->set_deposit(120000);

    // xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    // parent_account.parse();
    // parent_account.check();
    // int32_t ret = parent_account.source_action_exec();
    // EXPECT_EQ(0, ret);
    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(100000);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account sub_account(m_target_context.get(), recvtx);
    m_target_context->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, "500", true);
    m_target_context->string_set(XPROPERTY_USED_DISK_KEY, "409500", true);
    sub_account.parse();
    sub_account.check();
    auto ret = sub_account.target_action_exec();
    EXPECT_NE(0, ret);
}

TEST_F(test_contract_fee_service, target_disk_enough) {
    m_source_context->get_blockchain()->set_balance(XGET_CONFIG(min_account_deposit) + 120000);
    //m_source_context->set_timer_height(1024); // chain timer height
    m_source_context->string_set(XPROPERTY_USED_TGAS_KEY, "25000", true);
    m_source_context->string_set(XPROPERTY_LAST_TX_HOUR_KEY, "1024", true);

    m_trans->get_transaction()->set_deposit(120000);

    // xtransaction_create_contract_account parent_account(m_source_context.get(), m_trans);

    // parent_account.parse();
    // parent_account.check();
    // int32_t ret = parent_account.source_action_exec();
    // EXPECT_EQ(0, ret);
    xtransaction_exec_state_t state;
    state.set_send_tx_lock_tgas(100000);
    xcons_transaction_ptr_t recvtx = test_xtxexecutor_util_t::create_mock_recv_tx(m_trans->get_transaction(), state);

    xtransaction_create_contract_account sub_account(m_target_context.get(), m_trans);
    m_target_context->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, "500", true);
    sub_account.parse();
    sub_account.check();
    auto ret = sub_account.target_action_exec();
    EXPECT_EQ(0, ret);
}
