// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include "xmbus/xevent.h"
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"

NS_BEG2(top, mbus)

class xevent_lack_t : public xevent_t {
public:

    enum _minor_type_ {
        type_block,
    };

    xevent_lack_t(_minor_type_ sub_type, const std::string &_reason, direction_type dir = to_listener, bool _sync = true)
    : xevent_t(xevent_major_type_lack, (int) sub_type, dir, _sync),
    reason(_reason) {
    }

    std::string reason;
};

DEFINE_SHARED_PTR(xevent_lack);

class xevent_lack_block_t : public xevent_lack_t {
public:

    xevent_lack_block_t(
            const std::string &_address,
            const std::set<uint64_t> &_set_heights,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true)
    : xevent_lack_t(type_block, _reason, dir, _sync),
    address(_address),
    set_heights(_set_heights) {
    }

    std::string address;
    std::set<uint64_t> set_heights;
};

DEFINE_SHARED_PTR(xevent_lack_block);

NS_END2
