// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvledger/xvsysobj.h"

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
    }//end of namespace of base
}//end of namespace top
