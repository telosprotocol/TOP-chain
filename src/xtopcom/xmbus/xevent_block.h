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

class xevent_block_t : public xbus_event_t {
public:

    enum _minor_type_ {
        none
    };

    xevent_block_t(
            data::xblock_ptr_t _block,
            bool _new_block,
            direction_type dir = to_listener,
            bool _sync = true) :
    xbus_event_t(xevent_major_type_block,
    (int) none,
    dir,
    _sync),
    block(_block),
    new_block(_new_block) {
    }

    data::xblock_ptr_t block;
    bool new_block{false};
};

using xevent_block_ptr_t = xobject_ptr_t<xevent_block_t>;

NS_END2
