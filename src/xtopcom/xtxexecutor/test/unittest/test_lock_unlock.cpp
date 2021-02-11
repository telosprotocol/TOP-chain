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

class test_lock_unlock : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_source_context->set_lock_token_sum(XGET_CONFIG(min_account_deposit));
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());
        m_trans =  make_object_ptr<xtransaction_t>();
        m_trans->set_deposit(100000);
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    std::string m_source_account{"T0000011111111111111111"};
    std::string m_target_account{"T0000011111111111111111"};
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    xtransaction_ptr_t                  m_trans;
};

TEST_F(test_lock_unlock, valid_time) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T0000011111111111111111"};
    std::string target_account{"T0000011111111111111111"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());

    source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(tx->get_source_action(), asset_out);

    base::xstream_t stream(base::xcontext_t::instance());
    stream << 0; // version
    uint64_t amount = 100;
    stream << amount; // amount
    stream << xaction_lock_account_token::UT_time; // unlock_type
    stream << 1; // size
    std::string sign("10");
    stream << sign; // lock duration

    tx->get_target_action().set_account_addr(target_account);
    tx->get_target_action().set_action_type(xaction_type_lock_token);
    tx->get_target_action().set_action_param(std::string((char*)stream.data(), stream.size()));

    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_lock_token lock(source_context.get(), trans);
    int32_t ret = lock.parse();
    EXPECT_EQ(0, ret);
    ret = lock.target_action_exec();
    EXPECT_EQ(0, ret);


    std::string hash_str((char *)tx->digest().data(), tx->digest().size());
    base::xstream_t stream1(base::xcontext_t::instance());
    stream1 << 0; // version
    stream1 << hash_str;
    stream1 << 1; // size
    stream1 << sign;
    tx->get_target_action().set_action_type(xaction_type_unlock_token);
    tx->get_target_action().set_action_param(std::string((char*)stream1.data(), stream1.size()));

    xcons_transaction_ptr_t trans2 = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_unlock_token unlock(source_context.get(), trans2);
    ret = unlock.parse();
    EXPECT_EQ(0, ret);
    ret = unlock.target_action_exec();
    EXPECT_EQ(xaccount_property_unlock_token_time_not_reach, ret);
}

TEST_F(test_lock_unlock, invalid_time) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T0000011111111111111111"};
    std::string target_account{"T0000011111111111111111"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());

    source_context->get_blockchain()->set_balance(ASSET_TOP(1000));

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{100};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(tx->get_source_action(), asset_out);

    base::xstream_t stream(base::xcontext_t::instance());
    stream << 0; // version
    uint64_t amount = 100;
    stream << amount; // amount
    stream << xaction_lock_account_token::UT_time; // unlock_type
    stream << 1; // size
    std::string sign("abc");
    stream << sign; // lock duration

    tx->get_target_action().set_account_addr(target_account);
    tx->get_target_action().set_action_type(xaction_type_lock_token);
    tx->get_target_action().set_action_param(std::string((char*)stream.data(), stream.size()));
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(ASSET_TOP(0.4));
    tx->set_digest();
    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_lock_token lock(source_context.get(), trans);
    int32_t ret = lock.parse();
    EXPECT_EQ(0, ret);
    ret = lock.target_action_exec();
    EXPECT_EQ(0, ret);


    std::string hash_str((char *)tx->digest().data(), tx->digest().size());
    base::xstream_t stream1(base::xcontext_t::instance());
    stream1 << 0; // version
    stream1 << hash_str;
    stream1 << 1; // size
    stream1 << sign;
    tx->get_target_action().set_action_type(xaction_type_unlock_token);
    tx->get_target_action().set_action_param(std::string((char*)stream1.data(), stream1.size()));

    xcons_transaction_ptr_t trans2 = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_unlock_token unlock(source_context.get(), trans2);
    ret = unlock.parse();
    EXPECT_EQ(0, ret);
    ret = unlock.target_action_exec();
    EXPECT_EQ(0, ret);
}
