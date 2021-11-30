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
        class xblkmigrate_t : public xblkfilter_t
        {
        public:
            static const char* get_register_key(){return "/init/migrate/db/blkfilter";}
        protected:
            xblkmigrate_t();
            virtual ~xblkmigrate_t();
        private:
            xblkmigrate_t(xblkmigrate_t &&);
            xblkmigrate_t(const xblkmigrate_t &);
            xblkmigrate_t & operator = (const xblkmigrate_t &);
        public:
            virtual bool  is_valid(const uint32_t obj_ver) override;//check version
            virtual int   init(const xvconfig_t & config)  override;//init config
            virtual bool  close(bool force_async = false)  override;//close filter
            //caller respond to cast (void*) to related  interface ptr
            virtual void* query_interface(const int32_t _enum_xobject_type_) override;
            
            virtual enum_xfilter_handle_code    transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter) override;
            virtual enum_xfilter_handle_code    transfer_block_index(xdbevent_t & event,xvfilter_t* last_filter) override;
            virtual enum_xfilter_handle_code    transfer_block_object(xdbevent_t & event,xvfilter_t* last_filter) override;
        };
    
        template<uint32_t migrate_version>
        class xblkmigrate_ver : public xblkmigrate_t,public xsyscreator<xblkmigrate_ver<migrate_version> >
        {
            friend class xsyscreator<xblkmigrate_ver<migrate_version> >;
        public:
            static const uint32_t get_register_version(){return migrate_version;}
        protected:
            xblkmigrate_ver()
            {
                xsysobject_t::set_object_version(get_register_version());
            }
            virtual ~xblkmigrate_ver(){};
        private:
            xblkmigrate_ver(xblkmigrate_ver &&);
            xblkmigrate_ver(const xblkmigrate_ver &);
            xblkmigrate_ver & operator = (const xblkmigrate_ver &);
        };
            
        #define DECLARE_BLK_MIGRATE(version) \
            class xblkmigrate_ver##version : public xblkmigrate_ver<version> \
            {\
            protected:\
                xblkmigrate_ver##version(){};\
                virtual ~xblkmigrate_ver##version(){};\
            };\
    
        //***************************DECLARE OBJECTS WITH VERSION***************************//
        DECLARE_BLK_MIGRATE(1);
    
    }//end of namespace of base
}//end of namespace top
