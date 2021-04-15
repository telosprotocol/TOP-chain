#include "gtest/gtest.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"

using namespace top::txexecutor;
using namespace top::store;

class test_transaction_context : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_transaction_context, transaction_context1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    xtransaction_ptr_t tx = datamock.create_tx_create_user_account("T0000011111111111111111");
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    std::shared_ptr<store::xaccount_context_t> account_context{nullptr};
    xtransaction_context_t tc(account_context.get(), trans);

    auto ret = tc.parse();

    EXPECT_EQ(0, ret);
}

TEST_F(test_transaction_context, transaction_context2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    xtransaction_ptr_t tx = datamock.create_tx_transfer("T0000011111111111111111", "T0000011111111111111112", 1);
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    std::shared_ptr<store::xaccount_context_t> account_context{nullptr};
    xtransaction_context_t tc(account_context.get(), trans);

    auto ret = tc.parse();

    EXPECT_EQ(0, ret);
}

TEST_F(test_transaction_context, transaction_context3) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    xtransaction_ptr_t tx = datamock.create_tx_create_user_account("T0000011111111111111111");
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    std::shared_ptr<store::xaccount_context_t> account_context{nullptr};
    xtransaction_context_t tc(account_context.get(), trans);

    tx->set_tx_type(xtransaction_type_create_contract_account);
    tc = xtransaction_context_t(account_context.get(), trans);
    auto ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_run_contract);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_set_account_keys);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_lock_token);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_unlock_token);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_alias_name);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_create_sub_account);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_vote);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_abolish_vote);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_pledge_token_tgas);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_redeem_token_tgas);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_pledge_token_disk);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_redeem_token_disk);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_pledge_token_vote);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);

    tx->set_tx_type(xtransaction_type_redeem_token_vote);
    tc = xtransaction_context_t(account_context.get(), trans);
    ret = tc.parse();
    EXPECT_NE(0, ret);
}

TEST_F(test_transaction_context, transaction_context4) {
    xobject_ptr_t<xstore_face_t> g_store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(g_store.get());
    xtransaction_ptr_t tx = datamock.create_tx_transfer("T0000011111111111111111", "T0000011111111111111112", 1);
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    tx->set_tx_type(xtransaction_type_abolish_vote);
    tx->set_tx_subtype(enum_transaction_subtype_self);

    std::string addr = "T0000011111111111111111";
    xaccount_context_t* ac = new xaccount_context_t(addr, g_store.get());
    xtransaction_context_t tc(ac, trans);

    xtransaction_result_t tr;
    auto ret = tc.exec(tr);

    EXPECT_NE(0, ret);
}

// TODO(jimmy)
TEST_F(test_transaction_context, DISABLED_transaction_create_contract) {
    uint16_t network_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t  address_type = (uint8_t) base::enum_vaccount_addr_type_secp256k1_user_account;
    utl::xecprikey_t parent_pri_key_obj;
    utl::xecpubkey_t parent_pub_key_obj = parent_pri_key_obj.get_public_key();
    std::string source_account = parent_pub_key_obj.to_address(address_type, network_id);

    address_type = (uint8_t) base::enum_vaccount_addr_type_custom_contract;
    utl::xecprikey_t sub_pri_key_obj;
    utl::xecpubkey_t sub_pub_key_obj = sub_pri_key_obj.get_public_key();
    std::string target_account = sub_pub_key_obj.to_address(source_account, address_type, network_id);

    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    auto genesis_block = xblocktool_t::create_genesis_lightunit(source_account, 1000000000);
    xassert(genesis_block != nullptr);
    xassert(true == store->set_vblock(genesis_block));
    xassert(true == store->execute_block(genesis_block));

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    std::string code("function init()\n\
  create_key('hello')\n\
end\n\
function set_property()\n\
  set_key('hello', 'world')\n\
  local value = get_key('hello')\n\
  print('value:' .. value or '')\n\
end\n\
");
    // tx->make_tx_create_contract_account(asset_out, 0, code);
    tx->get_source_action().set_action_type(data::xaction_type_asset_out);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    tx->get_target_action().set_action_authorization(std::string((char*)sub_pub_key_obj.data(), sub_pub_key_obj.size()));

    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());
    std::shared_ptr<xaccount_context_t> ac = std::make_shared<xaccount_context_t>(source_account, store.get());
    xtransaction_result_t result;
    xtransaction_context_t tc(ac.get(), trans);
    int32_t exec_ret = tc.exec(result);
    EXPECT_EQ(0, exec_ret);
}

TEST_F(test_transaction_context, transaction_create_invalid_contract) {
    uint16_t network_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    uint8_t  address_type = (uint8_t) base::enum_vaccount_addr_type_secp256k1_user_account;
    utl::xecprikey_t parent_pri_key_obj;
    utl::xecpubkey_t parent_pub_key_obj = parent_pri_key_obj.get_public_key();
    std::string source_account = parent_pub_key_obj.to_address(address_type, network_id);

    address_type = (uint8_t) base::enum_vaccount_addr_type_custom_contract;
    utl::xecprikey_t sub_pri_key_obj;
    utl::xecpubkey_t sub_pub_key_obj = sub_pri_key_obj.get_public_key();
    std::string target_account = sub_pub_key_obj.to_address(source_account, address_type, network_id);

    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    auto genesis_block = xblocktool_t::create_genesis_lightunit(source_account, 1000000000);
    xassert(genesis_block != nullptr);
    xassert(true == store->set_vblock(genesis_block));
    xassert(true == store->execute_block(genesis_block));

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    std::string code("function init()\n\
  create_key('hello')\n\
function set_property()\n\
  set_key('hello', 'world')\n\
  local value = get_key('hello')\n\
  print('value:' .. value or '')\n\
end\n\
");
    // tx->make_tx_create_contract_account(asset_out, 0, code);
    tx->get_source_action().set_action_type(data::xaction_type_asset_out);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    tx->get_target_action().set_action_authorization(std::string((char*)sub_pub_key_obj.data(), sub_pub_key_obj.size()));

    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());
    std::shared_ptr<xaccount_context_t> ac = std::make_shared<xaccount_context_t>(source_account, store.get());
    xtransaction_result_t result;
    xtransaction_context_t tc(ac.get(), trans);
    int32_t exec_ret = tc.exec(result);
    EXPECT_NE(0, exec_ret);
}

