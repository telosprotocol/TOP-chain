// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvledger/xvsysobj.h"
#include "xvledger/xvblockstore.h"

namespace top
{
    namespace base
    {
        class xvmigrate_t : public xvmodule_t
        {
        public:
            xvmigrate_t();
            virtual ~xvmigrate_t();
        private:
            xvmigrate_t(xvmigrate_t &&);
            xvmigrate_t(const xvmigrate_t &);
            xvmigrate_t & operator = (const xvmigrate_t &);
        };

        bool init_migrate();//just placehold to link this static lib
        bool db_migrate_v2_to_v3(const std::string & src_db_path, const std::string & dst_db_path);
        bool db_delta_migrate_v2_to_v3(const std::string & src_db_path, base::xvblockstore_t* dst_blockstore);
    }//end of namespace of base
}//end of namespace top
