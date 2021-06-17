// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xbase.h"
#include "xvpropertyrules.h"

namespace top
{
    namespace base
    {
        bool xvpropertyrules_t::is_valid_native_property(const std::string & name)
        {
            if (name.empty()) {
                xassert(false);
                return false;
            }
            if (name[0] != NATIVE_PROPERTY_PREFIX_CHAR) {
                return false;
            }
            if (name.size() > NATIVE_PROPERTY_MAX_LEN) {
                xassert(false);
                return false;
            }
            return true;
        }

        bool xvpropertyrules_t::is_valid_sys_contract_property(const std::string & name)
        {
            if (name.empty()) {
                xassert(false);
                return false;
            }
            if (name[0] != SYS_CONTRACT_PROPERTY_PREFIX_CHAR) {
                return false;
            }
            if (name.size() > SYS_CONTRACT_PROPERTY_MAX_LEN) {
                xassert(false);
                return false;
            }
            return true;
        }
    }//end of namespace of base

}//end of namespace top
