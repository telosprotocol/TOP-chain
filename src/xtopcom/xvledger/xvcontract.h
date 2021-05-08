// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvaccount.h"
#include "xvaction.h"
#include "xvexecontxt.h"
#include "xvinstruction.h"
 
namespace top
{
    namespace base
    {
        //convenient macro to register xvfunc_t/api for contract
        #define BEGIN_DECLARE_XVCONTRACT_API(_category) template<typename _T,enum_xvinstruct_class category=_category> void register_xvcontract_name_api_internal##_category(_T * pThis){
        #define IMPL_XVCONTRACT_API(funcname,entry,mini_tgas) register_function((const uint8_t)category,funcname, std::bind(&_T::entry,pThis,std::placeholders::_1,std::placeholders::_2,mini_tgas));
        #define END_DECLARE_XVCONTRACT_API(_category) }
        #define REGISTER_XVCONTRACT_API(_category) register_xvcontract_name_api_internal##_category(this)
    
        //any module that follow consensus and execution on the chain, could be treat a virtual contract
        //xvcontract like CPU that just involve computing without own memory/disk
        //note: xvcontract_t is multiple-thread safe since it not allow to modify anymore after constructed
        class xvcontract_t : public xobject_t
        {
        public:
            static const std::string  create_contract_uri(const std::string & contract_addr,const std::string & contract_name,const uint32_t version);
            static const std::string  get_contract_address(const std::string & contract_uri);
            static bool               parse_contract_uri(const std::string & in_contract_uri,std::string & out_contract_addr,std::string & out_contract_name,uint32_t & out_version);//return false if uri is invalid
            
            typedef std::function< const xvalue_t (const xvaction_t & op,xvexecontxt_t & context) > xexefunction_t;
        protected:
            xvcontract_t(const std::string & contract_addr,const std::string & contract_name,const uint32_t version,const int _obj_type = enum_xobject_type_vcontract);
            xvcontract_t(const xvcontract_t & obj);
            virtual ~xvcontract_t();
        private:
            xvcontract_t();
            xvcontract_t(xvcontract_t &&);
            xvcontract_t & operator= (xvcontract_t &&);
            xvcontract_t & operator= (const xvcontract_t &);
        public:
            //caller cast (void*) to related ptr
            virtual void*               query_interface(const int32_t _enum_xobject_type_) override;
            virtual const xvalue_t      execute(const xvaction_t & op,xvexecontxt_t & context);//might throw exception for error
            inline  const xvalue_t      nil_impl(const xvaction_t & op,xvexecontxt_t & context) const;
            inline  const xvalue_t      not_impl(const xvaction_t & op,xvexecontxt_t & context) const;
            
            const std::string           get_contract_uri()     const {return m_contract_uri;}
            const std::string           get_contract_addr()    const {return m_contract_addr;}
            const std::string           get_contract_name()    const {return m_contract_name;}
            const uint32_t              get_contract_version() const {return m_contract_version;}
            
        protected:
            bool                        register_function(const uint8_t method_type,const std::string & method_name,const xexefunction_t & api_function);
        private:
            std::string                 m_contract_uri;   //m_contract_uri = m_contract_addr/m_contract_name
            std::string                 m_contract_addr;  //which contract.contract addr is same as account address
            std::string                 m_contract_name;  //contract name could be "user",or like "TEP0","TEP1"
            uint32_t                    m_contract_version;//system contract may variouse version code
            std::map<std::string, xexefunction_t>  m_contract_functions; //for dynamic & runtime methods
        };
    
        //the user-deployed contract that is a wrap/proxy and ineternally link to raw contract and VM(like WASM)
        class xvusercontract_t: public xvcontract_t
        {
        protected:
            xvusercontract_t(const std::string & contract_addr);
            xvusercontract_t(const xvusercontract_t & obj);
            virtual ~xvusercontract_t();
        private:
            xvusercontract_t();
            xvusercontract_t(xvusercontract_t &&);
            xvusercontract_t & operator= (xvusercontract_t &&);
            xvusercontract_t & operator= (const xvusercontract_t &);
        public:
            //caller cast (void*) to related ptr
            virtual void*       query_interface(const int32_t _enum_xobject_type_) override;
        };
    
        //system-wide contract
        class xvsyscontract_t : public xvcontract_t
        {
        protected:
            xvsyscontract_t(const std::string & contract_addr,const std::string & contract_name,const uint32_t version,const int _obj_type);
            xvsyscontract_t(const xvsyscontract_t & obj);
            virtual ~xvsyscontract_t();
        private:
            xvsyscontract_t();
            xvsyscontract_t(xvsyscontract_t &&);
            xvsyscontract_t & operator= (xvsyscontract_t &&);
            xvsyscontract_t & operator= (const xvsyscontract_t &);
        };

        //default implementation for TEP0(manage for TOP Main Token)
        #define  const_TEP0_contract_name "TEP0"
        class xvcontract_TEP0 : public xvsyscontract_t
        {
        public:
            static const std::string  get_main_token_name()        {return "@$$";}
            static const std::string  get_deposit_function_name()  {return "deposit";}
            static const std::string  get_withdraw_function_name() {return "withdraw";}

            static const xvaction_t   new_deposit(const vtoken_t add_token,const std::string & contract_addr,const uint32_t contract_version = 0);//simple version
            static const xvaction_t   new_deposit(const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t add_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version = 0);
            
            static const xvaction_t   new_withdraw(const vtoken_t sub_token,const std::string & contract_addr,const uint32_t contract_version = 0);//simple version
            static const xvaction_t   new_withdraw(const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t sub_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version = 0);
            
        public:
            xvcontract_TEP0(const std::string & contract_addr,const uint32_t version = 0);
        protected:
            xvcontract_TEP0(const xvcontract_TEP0 & obj);
            virtual ~xvcontract_TEP0();
        private:
            xvcontract_TEP0();
            xvcontract_TEP0(xvcontract_TEP0 &&);
            xvcontract_TEP0 & operator= (const xvcontract_TEP0 &);
            
        public://construct related xvmethod_t for specific function
            //caller cast (void*) to related ptr
            virtual void*       query_interface(const int32_t _enum_xobject_type_) override;
            
            const xvalue_t      deposit(const vtoken_t add_token,xvexecontxt_t& context);//return the updated balance
            const xvalue_t      withdraw(const vtoken_t sub_token,xvexecontxt_t& context);//return the updated balance
            
        private: //functions execute
            const xvalue_t      do_deposit(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
            const xvalue_t      do_withdraw(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
        private:
            BEGIN_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
                IMPL_XVCONTRACT_API(get_deposit_function_name(),do_deposit,0)
                IMPL_XVCONTRACT_API(get_withdraw_function_name(),do_withdraw,0)
            END_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
        };
    
        //default implementation for TEP1(manage for TOP Native Token)
        #define  const_TEP1_contract_name  "TEP1"
        class xvcontract_TEP1 : public xvsyscontract_t
        {
        public:
            static const std::string  get_tep1_tokens_name()       {return "@$T1";}
            static const std::string  get_deposit_function_name()  {return "deposit";}
            static const std::string  get_withdraw_function_name() {return "withdraw";}
            
            static const xvaction_t   new_deposit(const std::string & token_name,const vtoken_t add_token,const std::string & contract_addr,const uint32_t contract_version = 0);//simple version
            static const xvaction_t   new_deposit(const std::string & token_name,const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t add_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version = 0);
            
            static const xvaction_t   new_withdraw(const std::string & token_name,const vtoken_t sub_token,const std::string & contract_addr,const uint32_t contract_version = 0);//simple version
            static const xvaction_t   new_withdraw(const std::string & token_name,const std::string & org_tx_hash,const std::string & caller_addr,const vtoken_t sub_token,const uint64_t max_tgas,const std::string & contract_addr,const uint32_t contract_version = 0);
            
        public:
            xvcontract_TEP1(const std::string & contract_addr,const uint32_t version = 0);
        protected:
            xvcontract_TEP1(const xvcontract_TEP1 & obj);
            virtual ~xvcontract_TEP1();
        private:
            xvcontract_TEP1();
            xvcontract_TEP1(xvcontract_TEP1 &&);
            xvcontract_TEP1 & operator= (const xvcontract_TEP1 &);
            
        public://construct related xvmethod_t for specific function
            //caller cast (void*) to related ptr
            virtual void*       query_interface(const int32_t _enum_xobject_type_) override;
            
        private: //functions to modify value actually
            //do_create create new native token
            const xvalue_t      do_deposit(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
            const xvalue_t      do_withdraw(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
        private:
            BEGIN_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
                IMPL_XVCONTRACT_API(get_deposit_function_name(),do_deposit,0)
                IMPL_XVCONTRACT_API(get_withdraw_function_name(),do_withdraw,0)
            END_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
        };
        
        //default implementation for TEP2(manage for TOP TRC20 Token)
        #define  const_TEP2_contract_name "TEP2"
        class xvcontract_TEP2 : public xvsyscontract_t
        {
        public:
            static const std::string  get_tep2_tokens_name()            {return "@$T2";}
            static const std::string  get_transfer_function_name()      {return "transfer";}
            static const std::string  get_approve_function_name()       {return "approve";}
            static const std::string  get_transferFrom_function_name()  {return "transferFrom";}
            static const std::string  get_burn_function_name()          {return "burn";}
            
        public:
            xvcontract_TEP2(const std::string & contract_addr,const uint32_t version = 0);
        protected:
            xvcontract_TEP2(const xvcontract_TEP2 & obj);
            virtual ~xvcontract_TEP2();
        private:
            xvcontract_TEP2();
            xvcontract_TEP2(xvcontract_TEP2 &&);
            xvcontract_TEP2 & operator= (const xvcontract_TEP2 &);
            
        public://construct related xvmethod_t for specific function
            //caller cast (void*) to related ptr
            virtual void*       query_interface(const int32_t _enum_xobject_type_) override;
            
        private: //functions to modify value actually
            const xvalue_t      do_transfer(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);//transfer(address _to, int64_t _value)
            const xvalue_t      do_approve(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
            const xvalue_t      do_transferfrom(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
            const xvalue_t      do_burn(const xvaction_t & op,xvexecontxt_t & context,const uint64_t minimal_tgas);
        private:
            BEGIN_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
                IMPL_XVCONTRACT_API(get_transfer_function_name(),do_transfer,0)
                IMPL_XVCONTRACT_API(get_approve_function_name(),do_approve,0)
                IMPL_XVCONTRACT_API(get_transferFrom_function_name(),do_transferfrom,0)
                IMPL_XVCONTRACT_API(get_burn_function_name(),do_burn,0)
            END_DECLARE_XVCONTRACT_API(enum_xvinstruct_class_contract_function)
        };

    }//end of namespace of base

}//end of namespace top
