// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvcontract.h"
#include "xbase/xutl.h"
 
namespace top
{
    namespace base
    {
        xvcontract_t::xvcontract_t(const std::string & contract_name,xvbstate_t & bind_state)
        {
            bind_state.add_ref();
            m_contract_addr  = bind_state.get_account_addr();
            m_contract_name  = contract_name;
            m_contract_uri   = m_contract_addr + "." + contract_name;
            m_contract_state = &bind_state;
        }
        
        xvcontract_t::~xvcontract_t()
        {
            m_contract_state->release_ref();
        }
        
        void*  xvcontract_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vcontract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
        
        bool  xvcontract_t::register_method(const uint8_t method_type,const std::string & method_name,const xvfunc_t & api_function)
        {
            const std::string method_key = xstring_utl::tostring(method_type) + "." + method_name;
            m_contract_functions[method_key] = api_function;
            return true;
        }
        
        //[contract].function
        const xvalue_t xvcontract_t::execute(const xvaction_t & action)//might throw exception for error
        {
            if(is_close())
            {
                xerror("xvcontract_t::execute,contract has been closed,dump(%s)",dump().c_str());
                return xvalue_t(enum_xerror_code_closed);
            }
            
            const xvmethod_t & op = action.get_contract_function();
            if(op.is_name_method() == false) //must name-based call for contract
            {
                xerror("xvcontract_t::execute,is NOT a named call with name(%s) of uri(%s)",op.get_method_name().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_method);
            }
            if(get_contract_address() != action.get_contract_address())
            {
                xerror("xvcontract_t::execute,not match contract address(%s) vs ask(%s) of function(%s)",get_contract_address().c_str(),action.get_contract_address().c_str(), op.get_method_name().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            if(get_contract_name() != action.get_contract_name())
            {
                xerror("xvcontract_t::execute,not match contract name(%s) vs ask(%s) of function(%s)",get_contract_name().c_str(),action.get_contract_name().c_str(), op.get_method_name().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            if(get_contract_uri() != op.get_method_uri())
            {
                xerror("xvcontract_t::execute,not match uri(%s) vs ask(%s) of function(%s)",get_contract_uri().c_str(),op.get_method_uri().c_str(), op.get_method_name().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            
            const std::string method_key = xstring_utl::tostring(op.get_method_type()) + "." + op.get_method_name();
            auto it = m_contract_functions.find(method_key);
            if(it != m_contract_functions.end())
            {
                const xvalue_t res(it->second(action));
                if( (res.get_type() == xvalue_t::enum_xvalue_type_error) && (res.get_error() != enum_xcode_successful) )
                {
                    xerror("xvcontract_t::execute,catch error(%d) for method_name(%s) of uri(%s)",res.get_error(),method_key.c_str(),op.get_method_uri().c_str());
                }
                return res;
            }
            return not_impl(action);
        }
        
        //---------------------------------xvcontract_TEP0---------------------------------//
        xvcontract_TEP0::xvcontract_TEP0(xvbstate_t & bind_state)
            :xvsyscontract_t("TEP0",bind_state)
        {
            REGISTER_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function);
        }
    
        xvcontract_TEP0::~xvcontract_TEP0()
        {
        }
    
        const xvalue_t  xvcontract_TEP0::do_deposit(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_deposit_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & target_account_value = op.get_method_params().at(0);
            if(target_account_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & deposit_value = op.get_method_params().at(1);
            if(deposit_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Token must be operated at right state
            if(target_account_value.get_string() != get_contract_address())
            {
                xerror("xvcontract_TEP0::do_deposit,not match account address(%s) vs ask(%s) for uri(%s)",target_account_value.get_string().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            //load var of MAIN-TOKEN
            xauto_ptr<xtokenvar_t> main_token(get_state()->load_token_var(get_main_token_name()));
            if(!main_token)
            {
                xerror("xvcontract_TEP0::do_deposit,fail to load token_var(%s) of account(%s) for uri(%s)",get_main_token_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual deposit finally
            const vtoken_t new_balance = main_token->deposit(deposit_value.get_token());
            xinfo("xvcontract_TEP0::do_deposit,deposited account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",get_contract_address().c_str(),new_balance, deposit_value.get_token());
            return new_balance;
        }
    
        const xvalue_t  xvcontract_TEP0::do_withdraw(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_withdraw_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & target_account_value = op.get_method_params().at(0);
            if(target_account_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & withdraw_value = op.get_method_params().at(1);
            if(withdraw_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Token must be operated at right state
            if(target_account_value.get_string() != get_contract_address())
            {
                xerror("xvcontract_TEP0::do_withdraw,not match account address(%s) vs ask(%s) for uri(%s)",target_account_value.get_string().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            //load var of MAIN-TOKEN
            xauto_ptr<xtokenvar_t> main_token(get_state()->load_token_var(get_main_token_name()));
            if(!main_token)
            {
                xerror("xvcontract_TEP0::do_withdraw,fail to load token_var(%s) of account(%s) for uri(%s)",get_main_token_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual deposit finally
            const vtoken_t new_balance = main_token->withdraw(withdraw_value.get_token());
            xinfo("xvcontract_TEP0::do_withdraw,drawed account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",get_contract_address().c_str(),new_balance, withdraw_value.get_token());
            return new_balance;
        }
        
        //---------------------------------xvcontract_TEP1---------------------------------//
        xvcontract_TEP1::xvcontract_TEP1(xvbstate_t & bind_state)
            :xvsyscontract_t("TEP1",bind_state)
        {
            REGISTER_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function);
        }
        
        xvcontract_TEP1::~xvcontract_TEP1()
        {
        }
    
        const xvalue_t  xvcontract_TEP1::do_deposit(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_deposit_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 3) || (op.get_method_params().size() != 3) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & target_account_value = op.get_method_params().at(0);
            if(target_account_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & token_symbol_value = op.get_method_params().at(1);
            if(token_symbol_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & deposit_value = op.get_method_params().at(2);
            if(deposit_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address
            if(target_account_value.get_string() != get_contract_address())
            {
                xerror("xvcontract_TEP1::do_deposit,not match account address(%s) vs ask(%s) for uri(%s) and token(%s)",target_account_value.get_string().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            //load manager of native tokens
            xauto_ptr<xmtokens_t> TEP1_Tokens(get_state()->load_multiple_tokens_var(get_tep1_tokens_name()));
            if(!TEP1_Tokens)
            {
                xerror("xvcontract_TEP1::do_deposit,fail to load xmtokens(%s) of account(%s) for uri(%s) and token(%s)",get_tep1_tokens_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual withdraw for target token
            const int64_t new_balance = TEP1_Tokens->deposit(token_symbol_value.get_string(), deposit_value.get_token());
            xinfo("xvcontract_TEP1::do_deposit,deposited symbol(%s) of account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",token_symbol_value.get_string().c_str(),get_contract_address().c_str(), new_balance,deposit_value.get_token());
            return xvalue_t(enum_xcode_successful);
        }
    
        const xvalue_t  xvcontract_TEP1::do_withdraw(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_withdraw_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 3) || (op.get_method_params().size() != 3) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & target_account_value = op.get_method_params().at(0);
            if(target_account_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & token_symbol_value = op.get_method_params().at(1);
            if(token_symbol_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & withdraw_value = op.get_method_params().at(2);
            if(withdraw_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address
            if(target_account_value.get_string() != get_contract_address())
            {
                xerror("xvcontract_TEP1::do_withdraw,not match account address(%s) vs ask(%s) for uri(%s) and token(%s)",target_account_value.get_string().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            //load manager of native tokens
            xauto_ptr<xmtokens_t> TEP1_Tokens(get_state()->load_multiple_tokens_var(get_tep1_tokens_name()));
            if(!TEP1_Tokens)
            {
                xerror("xvcontract_TEP1::do_withdraw,fail to load xmtokens(%s) of account(%s) for uri(%s) and token(%s)",get_tep1_tokens_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual withdraw for target token
            const int64_t new_balance = TEP1_Tokens->withdraw(token_symbol_value.get_string(), withdraw_value.get_token());
            xinfo("xvcontract_TEP1::do_withdraw,drawed symbol(%s) of account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",token_symbol_value.get_string().c_str(),get_contract_address().c_str(), new_balance,withdraw_value.get_token());
            return xvalue_t(enum_xcode_successful);
        }
    
        //---------------------------------xvcontract_TEP2---------------------------------//
        xvcontract_TEP2::xvcontract_TEP2(xvbstate_t & bind_state)
            :xvsyscontract_t("TEP2",bind_state)
        {
            REGISTER_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function);
        }
        
        xvcontract_TEP2::~xvcontract_TEP2()
        {
        }
    
        //transfer(address _to, int64_t _value)
        const xvalue_t  xvcontract_TEP2::do_transfer(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_transfer_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & receiver_account_value = op.get_method_params().at(0);
            if(receiver_account_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & amount_value = op.get_method_params().at(1);
            if(amount_value.get_type() != xvalue_t::enum_xvalue_type_int64)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //copy to local var first as safety reason
            const std::string  sender_addr   = action.get_caller();
            const std::string  receiver_addr = receiver_account_value.get_string();
            const int64_t amount_to_transfer = amount_value.get_int64();
            if(amount_to_transfer == 0)
                return xvalue_t(enum_xcode_successful);
            
            //negative check for amount
            if(amount_to_transfer < 0)
            {
                xerror("xvcontract_TEP2::do_transfer,negative amount while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",get_contract_address().c_str(), action.get_caller().c_str(),receiver_account_value.get_string().c_str(),amount_to_transfer);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            //empty receiver check
            if(receiver_addr.empty())
            {
                xerror("xvcontract_TEP2::do_transfer,nil receiver address while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",get_contract_address().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_value.get_int64());
                return xvalue_t(enum_xerror_code_bad_param);
            }
            //load manager of native tokens
            xauto_ptr<xmapvar_t<int64_t>> TEP2_Tokens(get_state()->load_int64_map_var(get_tep2_tokens_name()));
            if(!TEP2_Tokens)
            {
                xerror("xvcontract_TEP2::do_transfer,fail to load var(%s) of account(%s) for uri(%s)",get_tep2_tokens_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //copy to local var first as safety reason
            const int64_t sender_balance     = TEP2_Tokens->query(sender_addr);
            const int64_t receiver_balance   = TEP2_Tokens->query(receiver_addr);
            //check whether has enough fund to transfer
            if(sender_balance < amount_to_transfer)
            {
                xwarn("xvcontract_TEP2::do_transfer,NO enough balance(%" PRIu64 ") while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",sender_balance,get_contract_address().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_to_transfer);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            TEP2_Tokens->insert(sender_addr, sender_balance - amount_to_transfer);
            TEP2_Tokens->insert(receiver_addr, receiver_balance + amount_to_transfer);
            #ifdef DEBUG
            xassert(TEP2_Tokens->query(sender_addr) == (sender_balance - amount_to_transfer));
            xassert(TEP2_Tokens->query(receiver_addr) == (receiver_balance + amount_to_transfer));
            #endif
            if(TEP2_Tokens->query(sender_addr) == 0)
                TEP2_Tokens->erase(sender_addr); //erase it to save space
            
            xinfo("xvcontract_TEP2::do_transfer,done for contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",sender_balance,get_contract_address().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_to_transfer);
            return xvalue_t(enum_xcode_successful);
        }
    
        const xvalue_t  xvcontract_TEP2::do_burn(const xvaction_t & action)
        {
            const xvmethod_t & op = action.get_contract_function();
            if(op.get_method_name() != get_burn_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 1) || (op.get_method_params().size() != 1) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & amount_value = op.get_method_params().at(0);
            if(amount_value.get_type() != xvalue_t::enum_xvalue_type_int64)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //copy to local var first as safety reason
            const std::string  caller_addr   = action.get_caller();
            const int64_t amount_to_burn = amount_value.get_int64();
            if(amount_to_burn == 0)
                return xvalue_t(enum_xcode_successful);
            
            //negative check for amount
            if(amount_to_burn < 0)
            {
                xerror("xvcontract_TEP2::do_burn,negative amount while contract(%s) burn addr(%s) by amount(%" PRIu64 ")",get_contract_address().c_str(), caller_addr.c_str(),amount_to_burn);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            //load manager of native tokens
            xauto_ptr<xmapvar_t<int64_t>> TEP2_Tokens(get_state()->load_int64_map_var(get_tep2_tokens_name()));
            if(!TEP2_Tokens)
            {
                xerror("xvcontract_TEP2::do_burn,fail to load var(%s) of account(%s) for uri(%s)",get_tep2_tokens_name().c_str(),get_contract_address().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //copy to local var first as safety reason
            const int64_t caller_balance     = TEP2_Tokens->query(caller_addr);
            //check whether has enough fund to transfer
            if(caller_balance < amount_to_burn)
            {
                TEP2_Tokens->insert(caller_addr, 0); //set first as safety
                TEP2_Tokens->erase(caller_addr); //remove key to save space
                xwarn("xvcontract_TEP2::do_burn,done for contract(%s) burn addr(%s) by rest balance(%" PRIu64 ")",get_contract_address().c_str(), caller_addr.c_str(),caller_balance);
            }
            else
            {
                TEP2_Tokens->insert(caller_addr, caller_balance - amount_to_burn);
                xinfo("xvcontract_TEP2::do_burn,done for contract(%s) burn addr(%s) by full amount(%" PRIu64 ")",get_contract_address().c_str(), caller_addr.c_str(),amount_to_burn);
            }
            return xvalue_t(enum_xcode_successful);
        }
    
    };//end of namespace of base
};//end of namespace of top
