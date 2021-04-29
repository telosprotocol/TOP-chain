// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvexeunit.h"
#include "xvaccount.h"
#include "xvblock.h"
#include "xvaction.h"
#include "xvstate.h"
 
namespace top
{
    namespace base
    {
        //any module that follow consensus and execution on the chain, could be treat a virtual contract
        //here xvcontract is always bind to one account and the associated state,so create new one when need at everytime
        class xvcontract_t : public xobject_t
        {
        protected:
            typedef std::function< const xvalue_t (const xvaction_t & action) > xvfunc_t;
        public:
            xvcontract_t(const std::string & contract_name,xvbstate_t & bind_state);
        protected:
            virtual ~xvcontract_t();
        private:
            xvcontract_t();
            xvcontract_t(const xvcontract_t &);
            xvcontract_t(xvcontract_t &&);
            xvcontract_t & operator= (const xvcontract_t &);

        public:
            //caller cast (void*) to related ptr
            virtual void*               query_interface(const int32_t _enum_xobject_type_) override;
            const std::string           get_contract_address() const {return m_contract_addr;}
            const std::string           get_contract_name()    const {return m_contract_name;}
            const std::string           get_contract_uri()     const {return m_contract_uri;}
            
        public://might throw exception for error
            virtual const xvalue_t      execute( const xvaction_t & action);
            inline  const xvalue_t      nil_impl(const xvaction_t & action) const {return xvalue_t();}
            inline  const xvalue_t      not_impl(const xvaction_t & action) const {return xvalue_t();}
            
        protected:
            xvbstate_t*   get_state()  {return m_contract_state;}
            bool          register_method(const uint8_t method_type,const std::string & method_name,const xvfunc_t & api_function);
        private:
            xvbstate_t*                      m_contract_state;      //the binded state
            std::string                      m_contract_addr;       //which contract.contract addr is same as account address
            std::string                      m_contract_name;       //contract name could be "user",or like "TEP0","TEP1"
            std::string                      m_contract_uri;        //full uri that combine m_contract_addr.m_contract_name
            std::map<std::string, xvfunc_t>  m_contract_functions;  //for dynamic & runtime methods
        };
    
        //system-wide contract
        class xvsyscontract_t : public xvcontract_t
        {
        protected:
            xvsyscontract_t(const std::string & contract_name,xvbstate_t & bind_state)
                :xvcontract_t(contract_name,bind_state)
            {
            }
            virtual ~xvsyscontract_t()
            {
            }
        private:
            xvsyscontract_t();
            xvsyscontract_t(const xvsyscontract_t &);
            xvsyscontract_t(xvsyscontract_t &&);
            xvsyscontract_t & operator= (const xvsyscontract_t &);
        };
        
        //the user-deployed contract that is a wrap/proxy and ineternally link to raw contract and VM(like WASM)
        class xvusercontract_t: public xvcontract_t
        {
        };
    
        //default implementation for TEP0(manage for TOP Main Token)
        class xvcontract_TEP0 : public xvsyscontract_t
        {
        public:
            static const std::string  get_main_token_name()        {return "@$$";}
            static const std::string  get_deposit_function_name()  {return "deposit";}
            static const std::string  get_withdraw_function_name() {return "withdraw";}

        public:
            xvcontract_TEP0(xvbstate_t & bind_state);
        protected:
            virtual ~xvcontract_TEP0();
        private:
            xvcontract_TEP0();
            xvcontract_TEP0(const xvcontract_TEP0 &);
            xvcontract_TEP0(xvcontract_TEP0 &&);
            xvcontract_TEP0 & operator= (const xvcontract_TEP0 &);
            
        public://construct related xvmethod_t for specific function
            
        private: //functions execute
            const xvalue_t  do_deposit(const xvaction_t & action);
            const xvalue_t  do_withdraw(const xvaction_t & action);
        private:
            BEGIN_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
                IMPL_XVIFUNC_NAME_API(get_deposit_function_name(),do_deposit)
                IMPL_XVIFUNC_NAME_API(get_withdraw_function_name(),do_withdraw)
            END_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
        };
    
        //default implementation for TEP1(manage for TOP Native Token)
        class xvcontract_TEP1 : public xvsyscontract_t
        {
        public:
            static const std::string  get_tep1_tokens_name()       {return "@$T1";}
            static const std::string  get_deposit_function_name()  {return "deposit";}
            static const std::string  get_withdraw_function_name() {return "withdraw";}
            
        public:
            xvcontract_TEP1(xvbstate_t & bind_state);
        protected:
            virtual ~xvcontract_TEP1();
        private:
            xvcontract_TEP1();
            xvcontract_TEP1(const xvcontract_TEP1 &);
            xvcontract_TEP1(xvcontract_TEP1 &&);
            xvcontract_TEP1 & operator= (const xvcontract_TEP1 &);
            
        public://construct related xvmethod_t for specific function
            
        private: //functions to modify value actually
            const xvalue_t  do_deposit(const xvaction_t & action);
            const xvalue_t  do_withdraw(const xvaction_t & action);
        private:
            BEGIN_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
                IMPL_XVIFUNC_NAME_API(get_deposit_function_name(),do_deposit)
                IMPL_XVIFUNC_NAME_API(get_withdraw_function_name(),do_withdraw)
            END_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
        };
        
        //default implementation for TEP2(manage for TOP TRC20 Token)
        class xvcontract_TEP2 : public xvsyscontract_t
        {
        public:
            static const std::string  get_tep2_tokens_name()            {return "@$T2";}
            static const std::string  get_transfer_function_name()      {return "transfer";}
            static const std::string  get_approve_function_name()       {return "approve";}
            static const std::string  get_transferFrom_function_name()  {return "transferFrom";}
            static const std::string  get_burn_function_name()          {return "burn";}
            
        public:
            xvcontract_TEP2(xvbstate_t & bind_state);
        protected:
            virtual ~xvcontract_TEP2();
        private:
            xvcontract_TEP2();
            xvcontract_TEP2(const xvcontract_TEP2 &);
            xvcontract_TEP2(xvcontract_TEP2 &&);
            xvcontract_TEP2 & operator= (const xvcontract_TEP2 &);
            
        public://construct related xvmethod_t for specific function
            
        private: //functions to modify value actually
            const xvalue_t  do_transfer(const xvaction_t & action);//transfer(address _to, int64_t _value)
            const xvalue_t  do_approve(const xvaction_t & action);
            const xvalue_t  do_transferfrom(const xvaction_t & action);
            const xvalue_t  do_burn(const xvaction_t & action);
        private:
            BEGIN_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
                IMPL_XVIFUNC_NAME_API(get_transfer_function_name(),do_transfer)
                IMPL_XVIFUNC_NAME_API(get_approve_function_name(),do_approve)
                IMPL_XVIFUNC_NAME_API(get_transferFrom_function_name(),do_transferfrom)
                IMPL_XVIFUNC_NAME_API(get_burn_function_name(),do_burn)
            END_DECLARE_XVIFUNC_NAME_API(enum_xvinstruct_class_contract_function)
        };

    }//end of namespace of base

}//end of namespace top
