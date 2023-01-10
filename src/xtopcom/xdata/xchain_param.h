// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <set>

#include "xcommon/xnode_id.h"
#include "xbasic/xrange.hpp"
#include "xcommon/xsharding_info.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xdata_common.h"
#include "xconfig/xpredefined_configurations.h"
#include "json/value.h"
#include "xverifier/xverifier_utl.h"

namespace top { namespace data {

// todo try remove this instance
class xuser_params {
public:
    xuser_params(xuser_params const &)             = delete;
    xuser_params & operator=(xuser_params const &) = delete;
    xuser_params(xuser_params &&)                  = delete;
    xuser_params & operator=(xuser_params &&)      = delete;
    static xuser_params& get_instance() {
        static xuser_params instance;
        return instance;
    }
    // bool is_valid();

    common::xnode_id_t                    account;                   // account for the node
    std::string                           publickey;                 // node public key(hex)
    // std::string                           signkey;                 // node private key(hex)
#if defined XENABLE_MOCK_ZEC_STAKE
    common::xminer_type_t                  node_role_type;
#endif
private:
    xuser_params() = default;
};

}  // namespace data
}  // namespace top
