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
        class xkeymigrate_t : public xkeyvfilter_t
        {
        public:
            static const char* get_register_key(){return "/init/migrate/db/kvfilter";}
        protected:
            xkeymigrate_t();
            virtual ~xkeymigrate_t();
        private:
            xkeymigrate_t(xkeymigrate_t &&);
            xkeymigrate_t(const xkeymigrate_t &);
            xkeymigrate_t & operator = (const xkeymigrate_t &);
        public:
            virtual bool  is_valid(const uint32_t obj_ver) override;//check version
            virtual int   init(const xvconfig_t & config)  override;//init config
            virtual bool  close(bool force_async = false)  override;//close filter
            //caller respond to cast (void*) to related  interface ptr
            virtual void*  query_interface(const int32_t _enum_xobject_type_) override;
            
        protected: //triggered by push_event_back or push_event_front
            virtual enum_xfilter_handle_code  transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter) override;
            virtual enum_xfilter_handle_code  fire_event(const xvevent_t & event,xvfilter_t* last_filter) override;

            enum_xfilter_handle_code transfer_db_v2_to_v3(xdbevent_t & event,xvfilter_t* last_filter) const;
            enum_xfilter_handle_code transfer_db_v2_to_v3_transaction(xdbevent_t & event,xvfilter_t* last_filter) const;
            enum_xfilter_handle_code transfer_db_v2_to_v3_bindex(xdbevent_t & event,xvfilter_t* last_filter) const;
            enum_xfilter_handle_code transfer_db_v2_to_v3_meta(xdbevent_t & event,xvfilter_t* last_filter) const;
            enum_xfilter_handle_code transfer_db_v2_to_v3_span_height(xdbevent_t & event,xvfilter_t* last_filter) const;
            enum_xfilter_handle_code transfer_db_v2_to_v3_span(xdbevent_t & event,xvfilter_t* last_filter)const ;
            std::string              get_addr_by_xvid(const std::string & xvid) const;

        private:
            std::map<std::string, std::string>  m_tableaddr_vids;
        };
    
        template<uint32_t migrate_version>
        class xkeymigrate_ver : public xkeymigrate_t,public xsyscreator<xkeymigrate_ver<migrate_version> >
        {
            friend class xsyscreator<xkeymigrate_ver<migrate_version> >;
        public:
            static const uint32_t get_register_version(){return migrate_version;}
        protected:
            xkeymigrate_ver()
            {
                xsysobject_t::set_object_version(get_register_version());
            }
            virtual ~xkeymigrate_ver(){};
        private:
            xkeymigrate_ver(xkeymigrate_ver &&);
            xkeymigrate_ver(const xkeymigrate_ver &);
            xkeymigrate_ver & operator = (const xkeymigrate_ver &);
        };
            
        #define DECLARE_KEY_MIGRATE(version) \
            class xkeymigrate_ver##version : public xkeymigrate_ver<version> \
            {\
            protected:\
                xkeymigrate_ver##version(){};\
                virtual ~xkeymigrate_ver##version(){};\
            };\
    
        //***************************DECLARE OBJECTS WITH VERSION***************************//
        DECLARE_KEY_MIGRATE(1);
    
    }//end of namespace of base
}//end of namespace top
