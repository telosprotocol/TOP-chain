// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include "xdbmigrate.h"
#include "xvledger/xvdbfilter.h"

namespace top
{
    namespace base
    {
        class xtxsmigrate_t : public xtxsfilter_t
        {
        public:
            static const char* get_register_key(){return "/init/migrate/db/txsfilter";}
        protected:
            xtxsmigrate_t();
            virtual ~xtxsmigrate_t();
        private:
            xtxsmigrate_t(xtxsmigrate_t &&);
            xtxsmigrate_t(const xtxsmigrate_t &);
            xtxsmigrate_t & operator = (const xtxsmigrate_t &);
        public:
            virtual bool  is_valid(const uint32_t obj_ver) override;//check version
            virtual int   init(const xvconfig_t & config)  override;//init config
            virtual bool  close(bool force_async = false)  override;//close filter
            //caller respond to cast (void*) to related  interface ptr
            virtual void* query_interface(const int32_t _enum_xobject_type_) override;
            
            virtual enum_xfilter_handle_code  transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter) override;
            virtual enum_xfilter_handle_code  transfer_tx(xdbevent_t & event,xvfilter_t* last_filter) override;
        private:
            enum_xfilter_handle_code    transfer_tx_v2_to_v3(xdbevent_t & event,xvfilter_t* last_filter);
        };
    
        template<uint32_t migrate_version>
        class xtxsmigrate_ver : public xtxsmigrate_t,public xsyscreator<xtxsmigrate_ver<migrate_version> >
        {
            friend class xsyscreator<xtxsmigrate_ver<migrate_version> >;
        public:
            static const uint32_t get_register_version(){return migrate_version;}
        protected:
            xtxsmigrate_ver()
            {
                xsysobject_t::set_object_version(get_register_version());
            }
            virtual ~xtxsmigrate_ver(){};
        private:
            xtxsmigrate_ver(xtxsmigrate_ver &&);
            xtxsmigrate_ver(const xtxsmigrate_ver &);
            xtxsmigrate_ver & operator = (const xtxsmigrate_ver &);
        };
            
        #define DECLARE_TXS_MIGRATE(version) \
            class xtxsmigrate_ver##version : public xtxsmigrate_ver<version> \
            {\
            protected:\
                xtxsmigrate_ver##version(){};\
                virtual ~xtxsmigrate_ver##version(){};\
            };\
    
        //***************************DECLARE OBJECTS WITH VERSION***************************//
        DECLARE_TXS_MIGRATE(1);
    
    }//end of namespace of base
}//end of namespace top
