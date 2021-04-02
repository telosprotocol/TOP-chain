// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser/xlua/xengine_context.h"

NS_BEG3(top, contract_runtime, lua)

xtop_engine_context::xtop_engine_context(data::xtransaction_ptr_t const & tx,
                                         observer_ptr<contract_common::xcontract_execution_context_t> contract_context,
                                         observer_ptr<xtransaction_execution_result_t> trace_ptr)
  : current_action{tx->get_target_action()}
  , contract_account{tx->get_target_addr()}
  , source_account{tx->get_source_addr()}
  //, m_contract_helper(std::make_shared<xcontract_helper>(account_context, m_contract_account, m_exec_account))
  , execution_status{std::move(trace_ptr)}
  , contract_context{contract_context} {
}

//void xtop_engine_context::exec()
//{
//    if (m_current_action.get_action_type() == data::enum_xaction_type::xaction_type_create_contract_account) {
//        publish_code();
//        return;
//    }
//
//    //todo check white and black contract and action list
//    //auto native_contract = contract::xcontract_manager_t::instance().get_contract(m_contract_account);
//    //if (native_contract) {
//    //    // due to contract now is NOT non-state, just clone for every calling
//    //    std::unique_ptr<xcontract::xcontract_base> _contract{native_contract->clone()};
//    //    assert(_contract != nullptr);
//    //    if (_contract != nullptr) {
//    //        _contract->exec(this);
//    //    } else {
//    //        xwarn("[xtop_engine_context::exec] clone contract instance failed");
//    //    }
//    //    return;
//    //}
//
//    //get account code and check the function is native
//    //auto native = m_vm_service.get_native_handler(m_current_action.get_action_name());
//    //if (native) {
//    //    //todo check the code is validate
//    //    (*native)(this);
//    //    return;
//    //}
//
//    //check the cache vm is exist
//    std::shared_ptr<xengine_t> engine;
//    std::string code;
//    if (!m_vm_service->m_vm_cache.get(m_contract_account, engine)) {
//        engine = std::make_shared<lua::xlua_engine>();
//        m_contract_helper->get_contract_code(code);
//        engine->load_script(code, *this);
//        m_vm_service->m_vm_cache.put(m_contract_account, engine);
//    }
//    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//    engine->process(m_contract_account, code, *this);
//    m_trace_ptr->duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
//}

//void xtop_engine_context::publish_code() {
//    if (m_contract_helper->string_exist(data::XPROPERTY_CONTRACT_CODE)) {
//        throw xvm_error{enum_xvm_error_code::enum_vm_code_is_exist, "code is exist"};
//    }
//    try {
//        uint64_t tgas_limit{0};
//        string code;
//        xstream_t stream(xcontext_t::instance(), (uint8_t*)m_current_action.get_action_param().data(), m_current_action.get_action_param().size());
//        stream >> tgas_limit;
//        stream >> code;
//        shared_ptr<xengine> engine = make_shared<xlua_engine>();
//        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//        engine->publish_script(code, *this);
//        m_trace_ptr->m_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
//        m_vm_service.m_vm_cache.put(m_contract_account, engine);
//        m_contract_helper->set_contract_code(code);
//        m_contract_helper->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, std::to_string(tgas_limit), true);
//    } catch(const xvm_error& e) {
//        throw e;
//    } catch(enum_xerror_code& e) {
//        throw xvm_error{enum_xvm_error_code::enum_lua_abi_input_error, "action_param stream code is not valid"};
//    } catch(...) {
//        throw xvm_error{enum_xvm_error_code::enum_lua_abi_input_error, "unkown exception"};
//    }
//}

NS_END3
