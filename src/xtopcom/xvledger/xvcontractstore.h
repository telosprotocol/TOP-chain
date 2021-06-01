// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvcontract.h"

namespace top
{
    namespace base
    {
        class xvcontractstore_t : public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvcontractstore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}
            
        protected:
            xvcontractstore_t();
            virtual ~xvcontractstore_t();
        private:
            xvcontractstore_t(xvcontractstore_t &&);
            xvcontractstore_t(const xvcontractstore_t &);
            xvcontractstore_t & operator = (xvcontractstore_t &&);
            xvcontractstore_t & operator = (const xvcontractstore_t &);
            
        public:
            //caller need to cast (void*) to related ptr
            virtual void*            query_interface(const int32_t _enum_xobject_type_) override;
            
            static const std::string         get_sys_tep0_contract_uri(const std::string & contract_addr,const uint32_t contract_version);
            static const std::string         get_sys_tep1_contract_uri(const std::string & contract_addr,const uint32_t contract_version);
            static const std::string         get_sys_tep2_contract_uri(const std::string & contract_addr,const uint32_t contract_version);
        public:

            virtual xauto_ptr<xvcontract_t>  get_sys_tep0_contract(const std::string & contract_addr,const uint32_t contract_version);
            virtual xauto_ptr<xvcontract_t>  get_sys_tep1_contract(const std::string & contract_addr,const uint32_t contract_version);
            virtual xauto_ptr<xvcontract_t>  get_sys_tep2_contract(const std::string & contract_addr,const uint32_t contract_version);
            
            virtual xauto_ptr<xvcontract_t>  get_sys_contract(const std::string & contract_uri);
            virtual xauto_ptr<xvcontract_t>  get_sys_contract(const std::string & contract_addr,const std::string & contract_name,const uint32_t version);
            
            virtual xauto_ptr<xvcontract_t>  get_usr_contract(const std::string & contract_addr);
            
            //universal api
            virtual xauto_ptr<xvcontract_t>  get_contract(const std::string & contract_uri);
        };
        
    }//end of namespace of base
}//end of namespace top
