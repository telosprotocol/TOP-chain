// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/fixed_hash.h"

// #include <boost/algorithm/string.hpp>

namespace top {
namespace evm_common {

std::random_device s_fixedHashEngine;

h128 fromUUID(std::string const & _uuid) {
    // try {
    //     return h128(boost::replace_all_copy(_uuid, "-", ""));
    // } catch (...) {
    //     return h128();
    // }
    return h128();
}

std::string toUUID(h128 const & _uuid) {
    // std::string ret = toHex(_uuid.ref());
    // for (unsigned i : {20, 16, 12, 8})
    //     ret.insert(ret.begin() + i, '-');
    // return ret;
    return "";
}

}  // namespace evm_common
}  // namespace top