// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(XENABLE_MOCK_ZEC_STAKE)
#include "xcommon/xrole_type.h"
#endif
#include "xcommon/xaccount_address.h"

#include <string>

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

    common::xaccount_address_t            account;                   // account for the node
    std::string                           publickey;                 // node public key(hex)
    // std::string                           signkey;                 // node private key(hex)
#if defined(XENABLE_MOCK_ZEC_STAKE)
    common::xminer_type_t                  node_role_type;
#endif
private:
    xuser_params() = default;
};

}  // namespace data
}  // namespace top
