// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "../xvstate.h"
#include "../xvcontract.h"

namespace top
{
    namespace base
    {
        const std::string xvcontract_t::create_contract_uri(const std::string & contract_addr,const std::string & contract_name,const uint32_t version)
        {
            if(contract_name.empty()) //user contract
                return contract_addr;
            
            if(0 == version) //save sapce for default version(0)
            {
                return (contract_addr + "/" + contract_name);
            }
            else
            {
                return (contract_addr + "/" + contract_name + "/" + xstring_utl::tostring(version));
            }
        }
    
        const std::string  xvcontract_t::get_contract_address(const std::string & contract_uri)
        {
            std::string out_contract_addr;
            std::string out_contract_name;
            uint32_t out_version = 0;
            parse_contract_uri(contract_uri,out_contract_addr,out_contract_name,out_version);
            return out_contract_addr;
        }
    
        bool  xvcontract_t::parse_contract_uri(const std::string & in_uri,std::string & out_contract_addr,std::string & out_contract_name,uint32_t & out_version)//return false if uri is invalid
        {
            if(in_uri.empty())
            {
                xerror("parse_contract_uri,nil contract uri");
                return false;
            }
            std::vector<std::string>  domains;
            if(xstring_utl::split_string(in_uri,'/',domains) > 0)
            {
                if(domains.size() >= 1)
                    out_contract_addr = domains[0];
                if(domains.size() >= 2)
                    out_contract_name = domains[1];
                if(domains.size() >= 3)
                    out_version = xstring_utl::touint32(domains[2]);
                
                return true;
            }
            xerror("parse_contract_uri,invalid contract uri(%s)",in_uri.c_str());
            return false;
        }
    
        xvcontract_t::xvcontract_t(const std::string & contract_addr,const std::string & contract_name,const uint32_t version,const int _obj_type)
            :xobject_t(_obj_type)
        {
            m_contract_version  = version;
            m_contract_addr     = contract_addr;
            m_contract_name     = contract_name;
            m_contract_uri      = create_contract_uri(contract_addr,contract_name,version);
        }
    
        xvcontract_t::xvcontract_t(const xvcontract_t & obj)
            :xobject_t(obj.get_obj_type())
        {
            m_contract_uri      = obj.m_contract_uri;
            m_contract_addr     = obj.m_contract_addr;
            m_contract_name     = obj.m_contract_name;
            m_contract_version  = obj.m_contract_version;
        }
        
        xvcontract_t::~xvcontract_t()
        {
        }
        
        void*  xvcontract_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vcontract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
         
        bool  xvcontract_t::register_function(const uint8_t method_type,const std::string & method_name,const xexefunction_t & api_function)
        {
            if(method_name.empty())
            {
                xassert(0);
                return false;
            }
            const std::string method_key = xstring_utl::tostring(method_type) + "." + method_name;
            m_contract_functions[method_key] = api_function;
            return true;
        }
    
        const xvalue_t      xvcontract_t::nil_impl(const xvaction_t & op,xvexecontxt_t & context) const
        {
            return xvalue_t();
        }
    
        const xvalue_t      xvcontract_t::not_impl(const xvaction_t & op,xvexecontxt_t & context) const
        {
            return xvalue_t();
        }
    
        //[contract].function
        const xvalue_t   xvcontract_t::execute(const xvaction_t & op,xvexecontxt_t & context)//might throw exception for error
        {
            if(is_close())
            {
                xerror("xvcontract_t::execute,contract has been closed,dump(%s)",dump().c_str());
                return xvalue_t(enum_xerror_code_closed);
            }
            
            if(op.is_name_method() == false) //must name-based call for contract
            {
                xerror("xvcontract_t::execute,is NOT a named call with name(%s) of uri(%s)",op.get_method_name().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_method);
            }
            if(get_contract_uri() != op.get_contract_uri())
            {
                xerror("xvcontract_t::execute,not match contract uri(%s) vs ask(%s) of function(%s)",get_contract_uri().c_str(),op.get_contract_uri().c_str(), op.get_method_name().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }

            //now safe to dispatch it
            const std::string method_key = xstring_utl::tostring(op.get_method_type()) + "." + op.get_method_name();
            auto it = m_contract_functions.find(method_key);
            if(it != m_contract_functions.end())
            {
                const xvalue_t res(it->second(op,context));
                if( (res.get_type() != xvalue_t::enum_xvalue_type_error) || (res.get_error() == enum_xcode_successful) )
                {
                    if(context.get_input_canvas() != nullptr)
                        context.get_input_canvas()->record(op); //record after successful
                }
                else
                {
                    xerror("xvexeunit_t::execute,catch error(%d) for method_name(%s) of uri(%s)",res.get_error(),method_key.c_str(),op.get_method_uri().c_str());
                }
                return res;
            }
            return not_impl(op,context);
        }
        
        xvusercontract_t::xvusercontract_t(const std::string & contract_addr)
            :xvcontract_t(contract_addr,"",0,enum_xobject_type_usr_contract)
        {
        }
        
        xvusercontract_t::xvusercontract_t(const xvusercontract_t & obj)
            :xvcontract_t(obj)
        {
        }
        
        xvusercontract_t::~xvusercontract_t()
        {
        }
    
        void*  xvusercontract_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_usr_contract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }

        xvsyscontract_t::xvsyscontract_t(const std::string & contract_addr,const std::string & contract_name,const uint32_t version,const int _obj_type)
            :xvcontract_t(contract_addr,contract_name,version,_obj_type)
        {
        }
    
        xvsyscontract_t::xvsyscontract_t(const xvsyscontract_t & obj)
            :xvcontract_t(obj)
        {
        }
        
        xvsyscontract_t::~xvsyscontract_t()
        {
        }
    
        //---------------------------------xvcontract_TEP0---------------------------------//
        const xvaction_t   xvcontract_TEP0::new_deposit(const vtoken_t add_token,const std::string & contract_addr,const uint32_t contract_version)//simple version
        {
            base::xvalue_t add_token_val(add_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(std::string(),std::string(),contract_uri,xvcontract_TEP0::get_deposit_function_name(),add_token_val);
        }
        
        const xvaction_t   xvcontract_TEP0::new_deposit(const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t add_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version)
        {
            base::xvalue_t add_token_val(add_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(org_tx_hash,caller_addr,contract_uri,xvcontract_TEP0::get_deposit_function_name(),add_token_val);
        }
    
        const xvaction_t   xvcontract_TEP0::new_withdraw(const vtoken_t sub_token,const std::string & contract_addr,const uint32_t contract_version)//simple version
        {
            base::xvalue_t sub_token_val(sub_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(std::string(),std::string(),contract_uri,xvcontract_TEP0::get_withdraw_function_name(),sub_token_val);
        }
        
        const xvaction_t   xvcontract_TEP0::new_withdraw(const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t sub_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version)
        {
            base::xvalue_t sub_token_val(sub_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(org_tx_hash,caller_addr,contract_uri,xvcontract_TEP0::get_withdraw_function_name(),sub_token_val);
        }
        
        xvcontract_TEP0::xvcontract_TEP0(const std::string & contract_addr,const uint32_t version)
            :xvsyscontract_t(contract_addr,const_TEP0_contract_name,version,enum_xobject_type_sys_tep0_contract)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
    
        xvcontract_TEP0::xvcontract_TEP0(const xvcontract_TEP0 & obj)
            :xvsyscontract_t(obj)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
    
        xvcontract_TEP0::~xvcontract_TEP0()
        {
        }
    
        void*  xvcontract_TEP0::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_sys_tep0_contract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
    
        const xvalue_t  xvcontract_TEP0::deposit(const vtoken_t add_token,xvexecontxt_t& context)//return the updated balance
        {
            base::xvalue_t add_token_val(add_token);
            xvaction_t deposit_func(std::string(),std::string(),get_contract_uri(),get_deposit_function_name(),add_token_val);

            return execute(deposit_func, context);
        }
        
        const xvalue_t  xvcontract_TEP0::withdraw(const vtoken_t sub_token,xvexecontxt_t& context)//return the updated balance
        {
            base::xvalue_t sub_token_val(sub_token);
            xvaction_t withdraw_func(std::string(),std::string(),get_contract_uri(),get_withdraw_function_name(),sub_token_val);
            
            return execute(withdraw_func, context);
        }
 
        const xvalue_t  xvcontract_TEP0::do_deposit(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP0::do_deposit,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
            if(op.get_method_name() != get_deposit_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 1) || (op.get_method_params().size() != 1) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & deposit_value = op.get_method_params().at(0);
            if(deposit_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Token must be operated at right state
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP0::do_deposit,fail to load state for address(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
        
            if(target_state->find_property(get_main_token_name()) == false)
            {
                if(deposit_value.get_token() > 0)
                {
                    target_state->new_token_var(get_main_token_name(),context.get_output_canvas());//create first
                }
                else
                {
                    xerror("xvcontract_TEP0::do_deposit,fail to create main-token for address(%s) with uri(%s) as deposit 0 token",get_contract_addr().c_str(),op.get_method_uri().c_str());
                    return xvalue_t(enum_xerror_code_not_found);
                }
            }
            
            //load var of MAIN-TOKEN
            xauto_ptr<xtokenvar_t> main_token(target_state->load_token_var(get_main_token_name()));
            if(!main_token)
            {
                xerror("xvcontract_TEP0::do_deposit,fail to load token_var(%s) of account(%s) for uri(%s)",get_main_token_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual deposit finally
            const vtoken_t new_balance = main_token->deposit(deposit_value.get_token(),context.get_output_canvas());
            xinfo("xvcontract_TEP0::do_deposit,deposited account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",get_contract_addr().c_str(),new_balance, deposit_value.get_token());
            return new_balance;
        }
    
        const xvalue_t  xvcontract_TEP0::do_withdraw(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP0::do_deposit,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
            if(op.get_method_name() != get_withdraw_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 1) || (op.get_method_params().size() != 1) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & withdraw_value = op.get_method_params().at(0);
            if(withdraw_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Token must be operated at right state
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP0::do_withdraw,fail to load state for address(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            
            //load var of MAIN-TOKEN
            xauto_ptr<xtokenvar_t> main_token(target_state->load_token_var(get_main_token_name()));
            if(!main_token)
            {
                xerror("xvcontract_TEP0::do_withdraw,fail to load token_var(%s) of account(%s) for uri(%s)",get_main_token_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual deposit finally
            const vtoken_t new_balance = main_token->withdraw(withdraw_value.get_token(),context.get_output_canvas());
            xinfo("xvcontract_TEP0::do_withdraw,drawed account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",get_contract_addr().c_str(),new_balance, withdraw_value.get_token());
            return new_balance;
        }
        
        //---------------------------------xvcontract_TEP1---------------------------------//
        const xvaction_t   xvcontract_TEP1::new_deposit(const std::string & token_name,const vtoken_t add_token,const std::string & contract_addr,const uint32_t contract_version)//simple version
        {
            base::xvalue_t token_name_val(token_name);
            base::xvalue_t add_token_val(add_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(std::string(),std::string(),contract_uri,xvcontract_TEP0::get_deposit_function_name(),token_name_val,add_token_val);
        }
        
        const xvaction_t   xvcontract_TEP1::new_deposit(const std::string & token_name,const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t add_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version)
        {
            base::xvalue_t token_name_val(token_name);
            base::xvalue_t add_token_val(add_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(org_tx_hash,caller_addr,contract_uri,xvcontract_TEP0::get_deposit_function_name(),token_name_val,add_token_val);
        }
        
        const xvaction_t   xvcontract_TEP1::new_withdraw(const std::string & token_name,const vtoken_t sub_token,const std::string & contract_addr,const uint32_t contract_version)//simple version
        {
            base::xvalue_t token_name_val(token_name);
            base::xvalue_t sub_token_val(sub_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(std::string(),std::string(),contract_uri,xvcontract_TEP0::get_withdraw_function_name(),token_name_val,sub_token_val);
        }
        
        const xvaction_t   xvcontract_TEP1::new_withdraw(const std::string & token_name,const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t sub_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version)
        {
            base::xvalue_t token_name_val(token_name);
            base::xvalue_t sub_token_val(sub_token);
            const std::string contract_uri = xvcontract_t::create_contract_uri(contract_addr, const_TEP0_contract_name, contract_version);
            return xvaction_t(org_tx_hash,caller_addr,contract_uri,xvcontract_TEP0::get_withdraw_function_name(),token_name_val,sub_token_val);
        }
    
        xvcontract_TEP1::xvcontract_TEP1(const std::string & contract_addr,const uint32_t version)
            :xvsyscontract_t(contract_addr,const_TEP1_contract_name,version,enum_xobject_type_sys_tep1_contract)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
        
        xvcontract_TEP1::xvcontract_TEP1(const xvcontract_TEP1 & obj)
            :xvsyscontract_t(obj)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
    
        xvcontract_TEP1::~xvcontract_TEP1()
        {
        }
    
        void*  xvcontract_TEP1::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_sys_tep1_contract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
    
        const xvalue_t  xvcontract_TEP1::do_deposit(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP1::do_deposit,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
            if(op.get_method_name() != get_deposit_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
    
            const xvalue_t & token_symbol_value = op.get_method_params().at(0);
            if(token_symbol_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & deposit_value = op.get_method_params().at(1);
            if(deposit_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Native Token must be operated at right state
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP1::do_deposit,fail to load state for address(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            
            //load manager of native tokens
            xauto_ptr<xmtokens_t> TEP1_Tokens(target_state->load_multiple_tokens_var(get_tep1_tokens_name()));
            if(!TEP1_Tokens)
            {
                xerror("xvcontract_TEP1::do_deposit,fail to load xmtokens(%s) of account(%s) for uri(%s) and token(%s)",get_tep1_tokens_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //XTODO, first we need check global contract whether it has issued this native token
            {
                //load global contract'state to check
            }
            
            if(deposit_value.get_token() == 0) //prevent zero attack
            {
                if(false == TEP1_Tokens->has_token(token_symbol_value.get_string()))
                {
                    xwarn("xvcontract_TEP1::do_deposit,no enough fund to create token symbol(%s) of account(%s) ",token_symbol_value.get_string().c_str(),get_contract_addr().c_str());
                    return xvalue_t(enum_xerror_code_not_found);
                }
            }
            
            //do actual withdraw for target token
            const int64_t new_balance = TEP1_Tokens->deposit(token_symbol_value.get_string(), deposit_value.get_token(),context.get_output_canvas());
            xinfo("xvcontract_TEP1::do_deposit,deposited symbol(%s) of account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",token_symbol_value.get_string().c_str(),get_contract_addr().c_str(), new_balance,deposit_value.get_token());
            return xvalue_t(enum_xcode_successful);
        }
    
        const xvalue_t  xvcontract_TEP1::do_withdraw(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP1::do_withdraw,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
            if(op.get_method_name() != get_withdraw_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & token_symbol_value = op.get_method_params().at(0);
            if(token_symbol_value.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & withdraw_value = op.get_method_params().at(1);
            if(withdraw_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //safety check by account address,TOP Native Token must be operated at right state
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP1::do_withdraw,fail to load state for address(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
            
            //load manager of native tokens
            xauto_ptr<xmtokens_t> TEP1_Tokens(target_state->load_multiple_tokens_var(get_tep1_tokens_name()));
            if(!TEP1_Tokens)
            {
                xerror("xvcontract_TEP1::do_withdraw,fail to load xmtokens(%s) of account(%s) for uri(%s) and token(%s)",get_tep1_tokens_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str(),token_symbol_value.get_string().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //do actual withdraw for target token
            const int64_t new_balance = TEP1_Tokens->withdraw(token_symbol_value.get_string(), withdraw_value.get_token(),context.get_output_canvas());
            xinfo("xvcontract_TEP1::do_withdraw,drawed symbol(%s) of account(%s) to balance(%" PRIu64 ") by amount(%" PRIu64 ")",token_symbol_value.get_string().c_str(),get_contract_addr().c_str(), new_balance,withdraw_value.get_token());
            return xvalue_t(enum_xcode_successful);
        }
    
        //---------------------------------xvcontract_TEP2---------------------------------//
        xvcontract_TEP2::xvcontract_TEP2(const std::string & contract_addr,const uint32_t version)
            :xvsyscontract_t(contract_addr,const_TEP2_contract_name,version,enum_xobject_type_sys_tep2_contract)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
    
        xvcontract_TEP2::xvcontract_TEP2(const xvcontract_TEP2 & obj)
            :xvsyscontract_t(obj)
        {
            REGISTER_XVCONTRACT_API(enum_xvinstruct_class_contract_function);
        }
        
        xvcontract_TEP2::~xvcontract_TEP2()
        {
        }
    
        void*  xvcontract_TEP2::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_sys_tep2_contract)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
    
        //transfer(address _to, int64_t _value)
        const xvalue_t  xvcontract_TEP2::do_transfer(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP2::do_transfer,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
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
            const std::string  sender_addr   = op.get_caller_uri();
            const std::string  receiver_addr = receiver_account_value.get_string();
            const int64_t amount_to_transfer = amount_value.get_int64();
            if(amount_to_transfer == 0)
                return xvalue_t(enum_xcode_successful);
            
            //negative check for amount
            if(amount_to_transfer < 0)
            {
                xerror("xvcontract_TEP2::do_transfer,negative amount while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",get_contract_addr().c_str(), op.get_caller_uri().c_str(),receiver_account_value.get_string().c_str(),amount_to_transfer);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            //empty receiver check
            if(receiver_addr.empty())
            {
                xerror("xvcontract_TEP2::do_transfer,nil receiver address while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",get_contract_addr().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_value.get_int64());
                return xvalue_t(enum_xerror_code_bad_param);
            }
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP2::do_transfer,fail to load state for contract(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
 
            //load manager of native tokens
            xauto_ptr<xmapvar_t<int64_t>> TEP2_Tokens(target_state->load_int64_map_var(get_tep2_tokens_name()));
            if(!TEP2_Tokens)
            {
                xerror("xvcontract_TEP2::do_transfer,fail to load var(%s) of account(%s) for uri(%s)",get_tep2_tokens_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //copy to local var first as safety reason
            const int64_t sender_balance     = TEP2_Tokens->query(sender_addr);
            const int64_t receiver_balance   = TEP2_Tokens->query(receiver_addr);
            //check whether has enough fund to transfer
            if(sender_balance < amount_to_transfer)
            {
                xwarn("xvcontract_TEP2::do_transfer,NO enough balance(%" PRIu64 ") while contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",sender_balance,get_contract_addr().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_to_transfer);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            TEP2_Tokens->insert(sender_addr, sender_balance - amount_to_transfer,context.get_output_canvas());
            TEP2_Tokens->insert(receiver_addr, receiver_balance + amount_to_transfer,context.get_output_canvas());
            #ifdef DEBUG
            xassert(TEP2_Tokens->query(sender_addr) == (sender_balance - amount_to_transfer));
            xassert(TEP2_Tokens->query(receiver_addr) == (receiver_balance + amount_to_transfer));
            #endif
            if(TEP2_Tokens->query(sender_addr) == 0)
                TEP2_Tokens->erase(sender_addr,context.get_output_canvas()); //erase it to save space
            
            xinfo("xvcontract_TEP2::do_transfer,done for contract(%s) transfer from(%s) to (%s) by amount(%" PRIu64 ")",sender_balance,get_contract_addr().c_str(), sender_addr.c_str(),receiver_addr.c_str(),amount_to_transfer);
            return xvalue_t(enum_xcode_successful);
        }
    
        const xvalue_t  xvcontract_TEP2::do_approve(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            return enum_xerror_code_not_implement;
        }
        const xvalue_t  xvcontract_TEP2::do_transferfrom(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            return enum_xerror_code_not_implement;
        }
    
        const xvalue_t  xvcontract_TEP2::do_burn(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas)
        {
            assert(false); // XTODO no use now
            //counting tags cost start here,note: base-class has verified it must be a xvaction_t object
            // if( (false == ((xvaction_t &)op).withdraw_tgas(minimal_tgas)) || (false == context.withdraw_tgas(minimal_tgas)) )
            // {
            //     xwarn("xvcontract_TEP2::do_burn,failed to withdraw tgas of amount(%" PRIu64 ") at contract(%s)",minimal_tgas,get_contract_addr().c_str());
            //     return xvalue_t(enum_xerror_code_no_resource);
            // }
            
            if(op.get_method_name() != get_burn_function_name())
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 1) || (op.get_method_params().size() != 1) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & amount_value = op.get_method_params().at(0);
            if(amount_value.get_type() != xvalue_t::enum_xvalue_type_int64)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            //copy to local var first as safety reason
            const std::string  caller_addr = op.get_caller_uri();
            const int64_t amount_to_burn = amount_value.get_int64();
            if(amount_to_burn == 0)
                return xvalue_t(enum_xcode_successful);
            
            //negative check for amount
            if(amount_to_burn < 0)
            {
                xerror("xvcontract_TEP2::do_burn,negative amount while contract(%s) burn addr(%s) by amount(%" PRIu64 ")",get_contract_addr().c_str(), caller_addr.c_str(),amount_to_burn);
                return xvalue_t(enum_xerror_code_bad_param);
            }
            xauto_ptr<xvexestate_t> target_state(context.get_state(get_contract_addr()));
            if(!target_state)
            {
                xerror("xvcontract_TEP2::do_burn,fail to load state for contract(%s) with uri(%s)",get_contract_addr().c_str(),op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_bad_address);
            }
 
            //load manager of native tokens
            xauto_ptr<xmapvar_t<int64_t>> TEP2_Tokens(target_state->load_int64_map_var(get_tep2_tokens_name()));
            if(!TEP2_Tokens)
            {
                xerror("xvcontract_TEP2::do_burn,fail to load var(%s) of account(%s) for uri(%s)",get_tep2_tokens_name().c_str(),get_contract_addr().c_str(), op.get_method_uri().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //copy to local var first as safety reason
            const int64_t caller_balance     = TEP2_Tokens->query(caller_addr);
            //check whether has enough fund to transfer
            if(caller_balance < amount_to_burn)
            {
                TEP2_Tokens->insert(caller_addr, 0,context.get_output_canvas()); //set first as safety
                TEP2_Tokens->erase(caller_addr,context.get_output_canvas()); //remove key to save space
                xwarn("xvcontract_TEP2::do_burn,done for contract(%s) burn addr(%s) by rest balance(%" PRIu64 ")",get_contract_addr().c_str(), caller_addr.c_str(),caller_balance);
            }
            else
            {
                TEP2_Tokens->insert(caller_addr, caller_balance - amount_to_burn,context.get_output_canvas());
                xinfo("xvcontract_TEP2::do_burn,done for contract(%s) burn addr(%s) by full amount(%" PRIu64 ")",get_contract_addr().c_str(), caller_addr.c_str(),amount_to_burn);
            }
            return xvalue_t(enum_xcode_successful);
        }
    
    };//end of namespace of base
};//end of namespace of top
