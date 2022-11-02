// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/data.h"

namespace top {
namespace evm_common {

xbytes_t subData(const xbytes_t & indata, size_t index, size_t length) {
    size_t subLength = length;
    if (index + subLength > indata.size()) {
        subLength = indata.size() - index;
    }
    return data(indata.data() + index, subLength);
}

xbytes_t subData(const xbytes_t & indata, size_t startIndex) {
    if (startIndex >= indata.size()) {
        return xbytes_t();
    }
    size_t subLength = indata.size() - startIndex;
    return data(indata.data() + startIndex, subLength);
}


}  // namespace evm_common
}  // namespace top