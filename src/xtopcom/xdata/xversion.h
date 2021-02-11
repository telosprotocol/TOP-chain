// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include "xbasic/xns_macro.h"

NS_BEG2(top, data)

union xversion_t {
    struct {
        uint8_t features_ptr;
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
    };
    uint32_t version;
};

#define CURRENT_VERSION 0x00000100

NS_END2