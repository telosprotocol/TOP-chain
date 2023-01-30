// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatistic/xbasic_size.hpp"

NS_BEG1(top)

size_t get_size(const std::string & str) {
    // todo: compatibility of different gcc versions
    if (str.capacity() == 0) {
        return 0;
    }
    return str.capacity() + 25; // for gcc version 4.8.5
}

NS_END1
