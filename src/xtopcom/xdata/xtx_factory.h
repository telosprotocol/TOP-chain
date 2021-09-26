// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtransaction_v1.h"
#include "xtransaction_v2.h"
namespace top { namespace data {

class xtx_factory {
public:
    static xtransaction_ptr_t create_tx(const enum_xtransaction_version tx_version = xtransaction_version_2) {
        switch (tx_version)
        {
        case xtransaction_version_1:
            return make_object_ptr<xtransaction_v1_t>();
            break;
        
        default:
            return make_object_ptr<xtransaction_v2_t>();
            break;
        }
    }
};

}  // namespace data
}  // namespace top
