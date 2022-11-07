// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, network)

struct xtop_transmission_property final
{
    xtop_transmission_property()                                               = default;
    xtop_transmission_property(xtop_transmission_property const &)             = default;
    xtop_transmission_property & operator=(xtop_transmission_property const &) = default;
    xtop_transmission_property(xtop_transmission_property &&)                  = default;
    xtop_transmission_property & operator=(xtop_transmission_property &&)      = default;
    ~xtop_transmission_property()                                              = default;
};
using xtransmission_property_t = xtop_transmission_property;

NS_END2
