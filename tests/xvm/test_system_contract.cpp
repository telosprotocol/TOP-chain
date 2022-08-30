#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xvm/xcontract/xdata_stream.h"
#include <gtest/gtest.h>
#include "xbase/xobject_ptr.h"
#include "xbase/xcontext.h"

#include "xbase/xmem.h"
#include "xbase/xcontext.h"

using namespace top;
using namespace top::xvm;
using namespace top::base;

TEST(test_system_contract, function)
{
    // xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    // shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>("T-200", store.get());
    // account_ctx_ptr->start();
    // xvm_service m_vm_service;

    // data::xaction_t source_action;
    // data::xaction_t destination_action;
    // source_action.set_account_addr("T-1");
    // destination_action.set_account_addr("T-200");
    // destination_action.set_action_name("hi");
    // destination_action.set_action_param("");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_run_contract);
    // auto publish_trx = make_object_ptr<data::xtransaction_t>();
    // publish_trx->set_source_action(source_action);
    // publish_trx->set_target_action(destination_action);
    // xtransaction_trace_ptr trace = m_vm_service.deal_transaction(publish_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-1");
    // destination_action.set_account_addr("T-200");
    // destination_action.set_action_name("hi2");
    // destination_action.set_action_param("");

    // auto exec_trx = make_object_ptr<data::xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

}

TEST(test_system_contract, unpack_param)
{
    // xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    // shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>("T-400", store.get());
    // account_ctx_ptr->start();
    // xvm_service m_vm_service;

    // data::xaction_t source_action;
    // data::xaction_t destination_action;
    // source_action.set_account_addr("T-1");
    // destination_action.set_account_addr("T-400");
    // destination_action.set_action_name("hi");
    // base::xstream_t stream(base::xcontext_t::instance());
    // string param((char*)stream.data(), stream.size());
    // destination_action.set_action_param(param);
    // auto exec_trx = make_object_ptr<data::xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // xtransaction_trace_ptr trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-1");
    // destination_action.set_account_addr("T-400");
    // destination_action.set_action_name("hi2");
    // base::xstream_t stream2(base::xcontext_t::instance());
    // stream2 << 66;
    // string param2((char*)stream2.data(), stream2.size());
    // destination_action.set_action_param(param2);

    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-1");
    // destination_action.set_account_addr("T-400");
    // destination_action.set_action_name("hi3");
    // base::xstream_t stream4(base::xcontext_t::instance());
    // stream4 << 66;
    // const string str{"this is string"};
    // stream4 << str;
    // //std::map<uint32_t, string> test_map{{1, "1"}, {2, "2"}};
    // //stream4 << test_map;
    // string param4((char*)stream4.data(), stream4.size());
    // destination_action.set_action_param(param4);

    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

}
