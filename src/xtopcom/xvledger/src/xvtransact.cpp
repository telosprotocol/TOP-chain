// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvtransact.h"

namespace top
{
    namespace base
    {
        xvtransact_t::xvtransact_t()
            :xdataunit_t((enum_xdata_type)enum_xobject_type_vtransact)
        {
        }

        xvtransact_t::~xvtransact_t()
        {
        }




    };//end of namespace of base
};//end of namespace of top
