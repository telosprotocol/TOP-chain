#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include <gtest/gtest.h>
#include "xbase/xobject_ptr.h"

#include "xbase/xmem.h"
#include "xbase/xcontext.h"

using namespace top;
using namespace top::xvm;
using namespace top::base;
using namespace top::data;
/*
function set_property()
    create_key('hello')
	set_key('hello', 'world')
	local value = get_key('hello')
		print('value:' .. value or '')
	end
function get_property()
	local value = get_key('hello')
	print('value:' .. value or '')
	if value ~= 'world' then
		return -1,'hello not find'
	end
end

{
	"code": "function set_property()\nset_key('hello', 'world')\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nend\nfunction get_property()\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nif value ~= 'world' then\nreturn -1,'hello not find'\nend\nend",
	"abi": [{
		"name": "set_property",
		"input": []
	},
	{
		"name": "get_property",
		"input": []
	}]
}

*/

TEST(xvm_string_property, string_property)
{
    // xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    // shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>("T-4", store.get());
    // account_ctx_ptr->start();
    // xvm_service m_vm_service;


    // xaction_t source_action;
    // xaction_t destination_action;
    // source_action.set_account_addr("T-3");
    // destination_action.set_account_addr("T-4");
    // destination_action.set_action_name("publish_code");

    // xstream_t stream(xcontext_t::instance());
    // std::string code = "function set_property()\ncreate_key('hello')\nset_key('hello', 'world')\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nend\nfunction get_property()\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nif value ~= 'world' then\nreturn -1,'hello not find'\nend\nend";
    // stream << (uint64_t)0; //tgas_limit
    // stream << code;
    // std::string code_stream((char*)stream.data(), stream.size());
    // destination_action.set_action_param(code_stream);

    // //destination_action.set_action_param("{\"code\":\"function set_property()\ncreate_key('hello')\nset_key('hello', 'world')\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nend\nfunction get_property()\nlocal value = get_key('hello')\nprint('value:' .. value or '')\nif value ~= 'world' then\nreturn -1,'hello not find'\nend\nend\",\"abi\":[{\"name\":\"set_property\",\"input\":[]},{\"name\":\"get_property\",\"input\":[]}]}");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_create_contract_account);
    // xtransaction_ptr_t publish_trx = make_object_ptr<xtransaction_t>();
    // publish_trx->set_source_action(source_action);
    // publish_trx->set_target_action(destination_action);
    // publish_trx->set_deposit(100000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // xtransaction_trace_ptr trace = m_vm_service.deal_transaction(publish_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-3");
    // destination_action.set_account_addr("T-4");
    // destination_action.set_action_name("set_property");
    // destination_action.set_action_param("");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_run_contract);
    // xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // exec_trx->set_deposit(100000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-3");
    // destination_action.set_account_addr("T-4");
    // destination_action.set_action_name("get_property");
    // destination_action.set_action_param("");
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));
}
