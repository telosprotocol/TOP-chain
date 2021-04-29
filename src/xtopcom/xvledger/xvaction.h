// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "xbase/xvmethod.h"
#include "xbase/xutl.h"

namespace top
{
    namespace base
    {
        //placehold declare temporary
        class xvtransaction_t
        {
            //XTODO,move current definition to here and simply them
        };
    
        class xvaction_t
        {
            friend class xvconract_t;
        public:
            xvaction_t(const std::string & caller_addr,xvmethod_t & contract_function,xvtransaction_t * ref_transaction,const int64_t max_tgas)
            {
                m_used_tgas         = 0;
                m_max_tgas          = max_tgas;
                m_caller_addr       = caller_addr;
                m_contract_function = contract_function;
                m_org_transaction   = ref_transaction;
                //XTODO,add reference for ref_transaction
                
                std::vector<std::string>  domains;
                if(xstring_utl::split_string(contract_function.get_method_uri(),'.',domains) >= 2)
                {
                    m_contract_addr = domains[0];
                    m_contract_name = domains[1];
                }
            }
            
            ~xvaction_t()
            {
            }
            
        public:
            const std::string   get_caller()             const {return m_caller_addr;}
            const std::string   get_contract_address()   const {return m_contract_addr;}
            const std::string   get_contract_name()      const {return m_contract_name;}
            const std::string   get_contract_uri()       const {return m_contract_function.get_method_uri();}
            const xvmethod_t &  get_contract_function()  const {return m_contract_function;}
            
            const int64_t       get_used_tgas()          const {return m_used_tgas;}
            const int64_t       get_max_tgas()           const {return m_max_tgas;}
            xvtransaction_t*    get_transaction()   const {return m_org_transaction;}
            
        private: //just open for xvcontract_t
            void                set_userd_tags(const int64_t tags) { m_used_tgas = tags;}
            
        private:
            std::string     m_caller_addr;       //who fire the call to the contract
            std::string     m_contract_addr;     //which contract. contract addr is same as account address
            std::string     m_contract_name;     //contract name could be "default",or like "TEP0","TEP1"
            xvmethod_t      m_contract_function; //what function be execution
            xvtransaction_t*m_org_transaction;   //orginal transaction is fired by user
            int64_t         m_max_tgas;          //max tGAS allow used
            int64_t         m_used_tgas;         //return how many tgas used for caller
        };

    }//end of namespace of base

}//end of namespace top
