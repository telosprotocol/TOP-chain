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

class test_service_fee : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
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

TEST_F(test_service_fee, registerNode) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    data::xproperty_asset asset_out{10000};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    m_trans->get_transaction()->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
    data::xaction_run_contract run_con;
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_source_account;
    auto role = common::xminer_type_t::edge;
    stream << static_cast<uint64_t>(role);
    std::string value = std::string((const char*)stream.data(), stream.size());
    run_con.serialze_to(m_trans->get_transaction()->get_target_action(), "registerNode", value);

    xtransaction_run_contract node_reg(m_source_context.get(), m_trans);

    node_reg.parse();
    node_reg.check();

    int32_t ret = node_reg.source_service_fee_exec();
    EXPECT_EQ(0, ret);

    ret = node_reg.source_fee_exec();
    EXPECT_EQ(0, ret);

    ret = node_reg.source_action_exec();
    EXPECT_EQ(0, ret);

    auto balance_change = m_source_context->get_balance_change();
#ifndef XENABLE_MOCK_ZEC_STAKE
    uint64_t beacon_tx_service_fee = ASSET_TOP(100);
#else
    uint64_t beacon_tx_service_fee = 0;
#endif
    EXPECT_EQ(-balance_change, beacon_tx_service_fee + m_trans->get_transaction()->get_deposit() + asset_out.m_amount);
}

TEST_F(test_service_fee, node_register_not_enough_balance) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(100));

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    data::xproperty_asset asset_out{10000};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    m_trans->get_transaction()->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
    data::xaction_run_contract run_con;
    base::xstream_t stream(base::xcontext_t::instance());
    stream << m_source_account;
    auto role = common::xminer_type_t::edge;
    stream << static_cast<uint64_t>(role);
    std::string value = std::string((const char*)stream.data(), stream.size());
    run_con.serialze_to(m_trans->get_transaction()->get_target_action(), "registerNode", value);

    xtransaction_run_contract node_reg(m_source_context.get(), m_trans);

    node_reg.parse();
    node_reg.check();

    int32_t ret = node_reg.source_service_fee_exec();
    EXPECT_EQ(0, ret);

    ret = node_reg.source_fee_exec();
    EXPECT_EQ(0, ret);

    ret = node_reg.source_action_exec();
#ifndef XENABLE_MOCK_ZEC_STAKE
    uint64_t out_top = 0;
    EXPECT_EQ(xaccount_balance_not_enough, ret);
#else
    uint64_t out_top = asset_out.m_amount;
    EXPECT_EQ(0, ret);
#endif

    auto balance_change = m_source_context->get_balance_change();
#ifndef XENABLE_MOCK_ZEC_STAKE
    uint64_t beacon_tx_service_fee = ASSET_TOP(100);
#else
    uint64_t beacon_tx_service_fee = 0;
#endif
    EXPECT_EQ(-balance_change, beacon_tx_service_fee + m_trans->get_transaction()->get_deposit() + out_top);
}
