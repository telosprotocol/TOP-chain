#include "gtest/gtest.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"
#include "xdata/xaction.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xtxexecutor/xtransaction_context.h"

using namespace top::txexecutor;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

#if 0  // TODO(hank)

class create_contract_sub_account : public testing::Test {
 protected:
    void SetUp() override {

        //m_account_context = std::make_shared<xaccount_context_t>("T-1", store.get());
        m_trans = make_object_ptr<xtransaction_t>();
        xgas_service::init();
    }

    void TearDown() override {
    }
 public:
    std::shared_ptr<xaccount_context_t> m_account_context;
    xtransaction_ptr_t                  m_trans;
};

TEST_F(create_contract_sub_account, contract_account) {
    xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    uint16_t network_id = utl::MAIN_NETWORK_ID;
    uint8_t  address_type = utl::SEPARATE_ACCOUNT;
    utl::xecprikey_t parent_pri_key_obj;
    utl::xecpubkey_t parent_pub_key_obj = parent_pri_key_obj.get_public_key();
    auto parent_addr = parent_pub_key_obj.to_address(address_type, network_id);

    address_type = (uint8_t) data::enum_addr_type_t::custom_sc;
    utl::xecprikey_t sub_pri_key_obj;
    utl::xecpubkey_t sub_pub_key_obj = sub_pri_key_obj.get_public_key();
    auto sub_addr = sub_pub_key_obj.to_address(parent_addr, address_type, network_id);

    m_trans->set_tx_type(enum_xtransaction_type::xtransaction_type_create_contract_account);

    m_account_context = std::make_shared<xaccount_context_t>(parent_addr, store.get());
    m_account_context->get_account()->m_account_balance = 1000;
    xstream_t stream_p(xcontext_t::instance());
    stream_p << (uint64_t)XGET_CONFIG(min_account_deposit);
    string str_p((char*)stream_p.data(), stream_p.size());
    xtransaction_create_contract_account parent_account(m_account_context.get(), m_trans);


    m_trans->get_source_action().set_account_addr(parent_addr);
    m_trans->get_source_action().set_action_param(str_p);
    m_trans->get_source_action().set_action_type(enum_xaction_type::xaction_type_asset_out);

    xstream_t stream_s(xcontext_t::instance());
    string code("function init()\n\
	create_key('hello')\n\
end\n\
function set_property()\n\
	set_key('hello', 'world')\n\
	local value = get_key('hello')\n\
	print('value:' .. value or '')\n\
end\n\
");
    stream_s << code;
    string str_s((char*)stream_s.data(), stream_s.size());
    m_trans->get_target_action().set_account_addr(sub_addr);
    m_trans->get_target_action().set_action_param(str_s);
    m_trans->get_target_action().set_action_type(enum_xaction_type::xaction_type_create_contract_account);
    m_trans->set_deposit(10000);
    uint256_t target_hash = m_trans->get_target_action().sha2();
    utl::xecdsasig_t signature = sub_pri_key_obj.sign(target_hash);
    string target_sign((char*)signature.get_compact_signature(), signature.get_compact_signature_size());
    m_trans->get_target_action().set_action_authorization(target_sign);

    parent_account.parse();
    parent_account.check();
    int32_t ret = parent_account.source_action_exec();
    EXPECT_EQ(0, ret);

    m_account_context = std::make_shared<xaccount_context_t>(sub_addr, store.get());
    m_account_context->get_account()->m_account_balance = 1000;
    xtransaction_create_contract_account sub_account(m_account_context.get(), m_trans);
    sub_account.parse();
    sub_account.check();
    ret = sub_account.target_action_exec();
    EXPECT_EQ(0, ret);

    // m_trans->m_deposit = 1;
    // int32_t ret = create_account.source_action_exec();
    // EXPECT_EQ(enum_xunit_service_error_type::xconsensus_service_error_gas_limit_not_enough, ret);

    // m_trans->set_deposit(10000);
    // ret = create_account.source_action_exec();
    // EXPECT_EQ(enum_xunit_service_error_type::xconsensus_service_error_balance_not_enough, ret);

    // m_account_context->get_account()->m_account_balance = 100;
    // ret = create_account.source_action_exec();
    // EXPECT_EQ(0, ret);
}
#endif
