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
function init()\n
lcreate('hello')\n
end\n
function set_property()\n
lpush('hello', 'world1')\n
lpush('hello', 'world2')\n
rpush('hello', 'world3')\n
local llen = llen('hello')\n
if llen ~= 3 then\n
    error('llen not 3')\n
end\n
local hello_list = lall('hello')\n
local hello_list_llen = #hello_list\n
if llen ~= hello_list_llen or hello_list[1] ~= 'world2' or hello_list[2] ~= 'world1' or hello_list[3] ~= 'world3' then\n
    error('hello_list error')\n
end\n
local lpopret = lpop('hello')\n
local rpopret = rpop('hello')\n
--print('pop:'.. lpopret .. ',' .. rpopret)\n
local hello_list_pop = lall('hello')\n
--print('hello_list_pop len:'.. #hello_list_pop ..',' .. hello_list_pop[1] or '')\n
if #hello_list_pop ~= 1 or hello_list_pop[1] ~= 'world1' then\n
    error('hello_list_pop error')\n
end\n
local lpopret = lpop('hello')\n
end

{
	"code": "function set_property()\nlpush('hello', 'world1')\nlpush('hello', 'world2')\nrpush('hello', 'world3')\nlocal llen = llen('hello')\nif llen ~= 3 then\n    return -1, 'llen not 3'\nend\nlocal hello_list = lall('hello')\nlocal hello_list_llen = #hello_list\nif llen ~= hello_list_llen or hello_list[1] ~= 'world2' or hello_list[2] ~= 'world1' or hello_list[3] ~= 'world3' then\n    return -1, 'hello_list error'\nend\nlocal hello_list_range = lrange('hello', 0, 3)\n--print('hello_list_range[1]:' .. hello_list_range[1] or '')\nif llen ~= #hello_list_range or hello_list_range[1] ~= 'world2' or hello_list_range[2] ~= 'world1' or hello_list_range[3] ~= 'world3' then\n    return -1, 'hello_list_range error'\nend\nlocal lpopret = lpop('hello')\nlocal rpopret = rpop('hello')\n--print('pop:'.. lpopret .. ',' .. rpopret)\nlocal hello_list_pop = lall('hello')\n--print('hello_list_pop len:'.. #hello_list_pop ..',' .. hello_list_pop[1] or '')\nif #hello_list_pop ~= 1 or hello_list_pop[1] ~= 'world1' then\n    return -1, 'hello_list_pop error'\nend\nend",
	"abi": [{
		"name": "set_property",
		"input": []
	}]
}
*/
TEST(xvm_list_property, list_property)
{
    // xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    // shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>("T-6", store.get());
    // account_ctx_ptr->start();
    // xvm_service m_vm_service;

    // xaction_t source_action;
    // xaction_t destination_action;
    // source_action.set_account_addr("T-5");
    // destination_action.set_account_addr("T-6");
    // destination_action.set_action_name("publish_code");

    // xstream_t stream(xcontext_t::instance());
    // std::string code = "function init()\nlcreate('hello')\nend\nfunction set_property()\nlpush('hello', 'world1')\nlpush('hello', 'world2')\nrpush('hello', 'world3')\nlocal llen = llen('hello')\nif llen ~= 3 then\n    error('llen not 3')\nend\nlocal hello_list = lall('hello')\nlocal hello_list_llen = #hello_list\nif llen ~= hello_list_llen or hello_list[1] ~= 'world2' or hello_list[2] ~= 'world1' or hello_list[3] ~= 'world3' then\n    error('hello_list error')\nend\nlocal lpopret = lpop('hello')\nlocal rpopret = rpop('hello')\n--print('pop:'.. lpopret .. ',' .. rpopret)\nlocal hello_list_pop = lall('hello')\n--print('hello_list_pop len:'.. #hello_list_pop ..',' .. hello_list_pop[1] or '')\nif #hello_list_pop ~= 1 or hello_list_pop[1] ~= 'world1' then\n    error('hello_list_pop error')\nend\nlocal lpopret = lpop('hello')\nend";
    // stream << (uint64_t)0; //tgas_limit
    // stream << code;
    // std::string code_stream((char*)stream.data(), stream.size());
    // destination_action.set_action_param(code_stream);
    // //destination_action.set_action_param("{\"code\":\"function init()\nlcreate('hello')\nend\nfunction set_property()\nlpush('hello', 'world1')\nlpush('hello', 'world2')\nrpush('hello', 'world3')\nlocal llen = llen('hello')\nif llen ~= 3 then\n    return -1, 'llen not 3'\nend\nlocal hello_list = lall('hello')\nlocal hello_list_llen = #hello_list\nif llen ~= hello_list_llen or hello_list[1] ~= 'world2' or hello_list[2] ~= 'world1' or hello_list[3] ~= 'world3' then\n    return -1, 'hello_list error'\nend\n\nend\",\"abi\":[{\"name\":\"set_property\",\"input\":[]}]}");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_create_contract_account);
    // xtransaction_ptr_t publish_trx = make_object_ptr<xtransaction_t>();
    // publish_trx->set_source_action(source_action);
    // publish_trx->set_target_action(destination_action);
    // publish_trx->set_deposit(10000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // xtransaction_trace_ptr trace = m_vm_service.deal_transaction(publish_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-5");
    // destination_action.set_account_addr("T-6");
    // destination_action.set_action_name("set_property";
    // destination_action.set_action_param("");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_run_contract);
    // xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // exec_trx->set_deposit(10000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // uint32_t tgas_use1 = trace->m_instruction_usage;
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-5");
    // destination_action.set_account_addr("T-6");
    // destination_action.set_action_name("set_property");
    // destination_action.set_action_param("");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_run_contract);
    // exec_trx = make_object_ptr<xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // exec_trx->set_deposit(10000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // uint32_t tgas_use2 = trace->m_instruction_usage;
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));
    // EXPECT_EQ(tgas_use1, tgas_use2);
}

