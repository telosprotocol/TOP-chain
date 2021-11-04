#pragma once

#include "xbase/xobject_ptr.h"
#include "xcontract_runtime/xblock_sniff_config.h"
#include "xvledger/xvblock.h"

NS_BEG4(top, vnode, components, sniffing)

enum class enum_sniffer_event_type: uint8_t {
    invalid,
    broadcast,
    timer,
    block,
};
using xsniffer_event_type_t = enum_sniffer_event_type;

using xsniffer_block_type_t = contract_runtime::xsniff_block_type_t;

struct xtop_sniffer_event_config {
    xsniffer_block_type_t type;
    std::function<bool(xobject_ptr_t<base::xvblock_t>)> function;
};
using xsniffer_event_config_t = xtop_sniffer_event_config;
using xsniffer_config_t = std::map<xsniffer_event_type_t, xsniffer_event_config_t>;

NS_END4
