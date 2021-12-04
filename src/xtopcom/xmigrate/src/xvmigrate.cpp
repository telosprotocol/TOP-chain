// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include "xvmigrate.h"
#include "xdbmigrate.h"

namespace top
{
    namespace base
    {
        xvmigrate_t::xvmigrate_t()
        {
        }
    
        xvmigrate_t::~xvmigrate_t()
        {
            
        }
    
        #define ENABLE_DB_MIGRATE_FROM_V2_TO_V3
        bool db_migrate_v2_to_v0_3_0_0(const std::string & db_path)
        {    
        #ifndef ENABLE_DB_MIGRATE_FROM_V2_TO_V3
            return true;
        #endif
            const std::string src_db_path = db_path;
            const std::string dst_db_path = db_path;

            xkinfo("db_migrate_v2_to_v0_3_0_0 begin.src_db_path=%s,dst_db_path=%s",src_db_path.c_str(),dst_db_path.c_str());
            std::cout << "db_migrate_v2_to_v0_3_0_0 begin" << std::endl;
            const std::string dst_db_version = "0.3.0.0";
            base::xauto_ptr<top::base::xvconfig_t> sys_config_ptr = new top::base::xvconfig_t();
            //configure bootstrap
            sys_config_ptr->set_config("system.version", "0.0.0.1");
            sys_config_ptr->set_config("system.boot.size", "1");
            //configure db migrate as bootstrap
            sys_config_ptr->set_config("system.boot.0.object_key", "/init/migrate/db" );
            sys_config_ptr->set_config("system.boot.0.object_version","0.0.0.1");
            //configu db filter options
            sys_config_ptr->set_config("/init/migrate/db/src_path", src_db_path );
            sys_config_ptr->set_config("/init/migrate/db/dst_path", dst_db_path );
            sys_config_ptr->set_config("/init/migrate/db/dst_version", dst_db_version );
            sys_config_ptr->set_config("/init/migrate/db/size", "3" );
            sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/blkfilter");
            sys_config_ptr->set_config("/init/migrate/db/1/object_key","/init/migrate/db/txsfilter");
            sys_config_ptr->set_config("/init/migrate/db/2/object_key","/init/migrate/db/kvfilter");

            top::base::init_migrate();
            top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
            if (enum_xcode_successful != init_module->init(*sys_config_ptr.get()))
            {
                xerror("db_migrate_v2_to_v0_3_0_0 fail-module init");
                return false;
            }
            if (false == init_module->start())
            {
                xerror("db_migrate_v2_to_v0_3_0_0 fail-module start");
                return false;
            }
            xkinfo("db_migrate_v2_to_v0_3_0_0 finish.");
            return true;
        }

        bool init_migrate()
        {
            return true;
        }
    
    }//end of namespace of base
}//end of namespace top
