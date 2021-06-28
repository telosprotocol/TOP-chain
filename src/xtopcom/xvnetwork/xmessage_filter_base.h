// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xvnetwork_message.h"

NS_BEG2(top, vnetwork)

class xtop_message_filter_base {
public:
    xtop_message_filter_base(xtop_message_filter_base const &) = delete;
    xtop_message_filter_base(xtop_message_filter_base &&) = default;
    xtop_message_filter_base & operator=(xtop_message_filter_base const &) = delete;
    xtop_message_filter_base & operator=(xtop_message_filter_base &&) = delete;
    virtual ~xtop_message_filter_base() = default;

    virtual bool filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const = 0;

protected:
    xtop_message_filter_base() = default;
};
using xmessage_filter_base_t = xtop_message_filter_base;

NS_END2
