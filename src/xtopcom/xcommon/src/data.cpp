// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/data.h"

namespace top {
namespace evm_common {

xbytes_t sub_data(xbytes_t const & data, size_t const index, size_t const length) {
    size_t sub_length = length;
    if (index + sub_length > data.size()) {
        sub_length = data.size() - index;
    }
    return evm_common::data(data.data() + index, sub_length);
}

xbytes_t sub_data(xbytes_t const & data, size_t const start_index) {
    if (start_index >= data.size()) {
        return xbytes_t{};
    }
    size_t const sub_length = data.size() - start_index;
    return evm_common::data(data.data() + start_index, sub_length);
}


}  // namespace evm_common
}  // namespace top