#pragma once

#include "xbase/xobject_ptr.h"
#include "xvledger/xvblock.h"

NS_BEG4(top, vnode, components, sniffing)

enum class enum_sniffer_event_type: uint8_t {
    invalid,
    broadcast,
    timer,
    block,
};
using xsniffer_event_type_t = enum_sniffer_event_type;

enum class enum_sniffer_block_type: uint8_t {
    invalid,
    full_block,
    all
};
using xsniffer_block_type_t = enum_sniffer_block_type;

struct xtop_sniffer_event_config {
    xsniffer_block_type_t type;
    std::function<bool(xobject_ptr_t<base::xvblock_t>)> function;
};
using xsniffer_event_config_t = xtop_sniffer_event_config;
using xsniffer_config_t = std::map<xsniffer_event_type_t, xsniffer_event_config_t>;

NS_END4
