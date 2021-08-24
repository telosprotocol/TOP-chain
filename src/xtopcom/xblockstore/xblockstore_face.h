// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvdbstore.h"
#include "xvledger/xvblockstore.h"

namespace top
{
    namespace store
    {
        base::xvblockstore_t*  get_vblockstore();
        base::xvblockstore_t*  create_vblockstore(base::xvdbstore_t* xvdb_ptr);
    };//end of namespace of xledger
};//end of namespace of top
