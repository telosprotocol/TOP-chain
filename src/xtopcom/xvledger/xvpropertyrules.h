// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

namespace top
{
    namespace base
    {
        class xvpropertyrules_t
        {
        private:
            static const uint32_t   NATIVE_PROPERTY_MAX_LEN   = 4;  // include prefix char $xxx
            static const char       NATIVE_PROPERTY_PREFIX_CHAR   = '$';
            static const uint32_t   SYS_CONTRACT_PROPERTY_MAX_LEN      = 8;  // include prefix char @
            static const char       SYS_CONTRACT_PROPERTY_PREFIX_CHAR      = '@';
        public:
            static bool is_valid_native_property(const std::string & name);
            static bool is_valid_sys_contract_property(const std::string & name);
        };
    }//end of namespace of base

}//end of namespace top
