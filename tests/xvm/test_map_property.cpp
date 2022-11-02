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
function init()\n\
hcreate('hello')
end\n\
function set_property()\n\
local hset1 = hset('hello','111','222')\n\
local hset2 = hset('hello','222','333')\n\
print('hset '.. hset1 .. ',' .. hset2)\n\
local hvalue = hget('hello','222')\n\
if hvalue ~= '333' then\n\
    return -1, 'error hvalue'\n\
end\n\
local hlen = hlen('hello')\n\
if hlen ~= 2 then\n\
    return -1, 'error hlen'\n\
end\n\
local hdel = hdel('hello', '222')\n\
local hdel_value = hget('hello','222')\n\
if hdel_value == '333' then\n\
    return -1, 'error hdel_value'\n\
end\n\
end

{
	"code": "function set_property()\nlocal hset1 = hset('hello','111','222')\nlocal hset2 = hset('hello','222','333')\n--print('hset '.. hset1 .. ',' .. hset2)\nlocal hvalue = hget('hello','222')\nif hvalue ~= '333' then\n    return -1, 'error hvalue'\nend\nlocal hlen = hlen('hello')\nif hlen ~= 2 then\n    return -1, 'error hlen'\nend\nlocal hdel = hdel('hello', '222')\n\nend",
	"abi": [{
		"name": "set_property",
		"input": []
	}]
}
*/


TEST(xvm_map_property, map_property)
{

    // xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    // shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>("T-8", store.get());
    // account_ctx_ptr->start();
    // xvm_service m_vm_service;


    // xaction_t source_action;
    // xaction_t destination_action;
    // source_action.set_account_addr("T-7");
    // destination_action.set_account_addr("T-8");
    // destination_action.set_action_name("publish_code");

    // xstream_t stream(xcontext_t::instance());
    // std::string code = "function init()\nhcreate('hello')\nend\nfunction set_property()\nlocal hset1 = hset('hello','111','222')\nlocal hset2 = hset('hello','222','333')\n--print('hset '.. hset1 .. ',' .. hset2)\nlocal hvalue = hget('hello','222')\nif hvalue ~= '333' then\n    return -1, 'error hvalue'\nend\nlocal hlen = hlen('hello')\nif hlen ~= 2 then\n    return -1, 'error hlen'\nend\nlocal hdel = hdel('hello', '222')\n\nend";
    // stream << (uint64_t)0; //tgas_limit
    // stream << code;
    // std::string code_stream((char*)stream.data(), stream.size());
    // destination_action.set_action_param(code_stream);

    // //destination_action.set_action_param("{\"code\":\"function init()\nhcreate('hello')\nend\nfunction set_property()\nlocal hset1 = hset('hello','111','222')\nlocal hset2 = hset('hello','222','333')\n--print('hset '.. hset1 .. ',' .. hset2)\nlocal hvalue = hget('hello','222')\nif hvalue ~= '333' then\n    return -1, 'error hvalue'\nend\nlocal hlen = hlen('hello')\nif hlen ~= 2 then\n    return -1, 'error hlen'\nend\nlocal hdel = hdel('hello', '222')\n\nend\",\"abi\":[{\"name\":\"set_property\",\"input\":[]}]}");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_create_contract_account);
    // xtransaction_ptr_t publish_trx = make_object_ptr<xtransaction_t>();
    // publish_trx->set_source_action(source_action);
    // publish_trx->set_target_action(destination_action);
    // publish_trx->set_deposit(10000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // xtransaction_trace_ptr trace = m_vm_service.deal_transaction(publish_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));

    // //exec trx
    // source_action.set_account_addr("T-7");
    // destination_action.set_account_addr("T-8");
    // destination_action.set_action_name("set_property");
    // destination_action.set_action_param("");
    // destination_action.set_action_type(data::enum_xaction_type::xaction_type_run_contract);
    // xtransaction_ptr_t exec_trx = make_object_ptr<xtransaction_t>();
    // exec_trx->set_source_action(source_action);
    // exec_trx->set_target_action(destination_action);
    // exec_trx->set_deposit(10000);
    // account_ctx_ptr->set_contract_parent_account(10000, source_action.get_account_addr());
    // trace = m_vm_service.deal_transaction(exec_trx, account_ctx_ptr.get());
    // EXPECT_EQ(0, static_cast<uint32_t>(trace->m_errno));
}
