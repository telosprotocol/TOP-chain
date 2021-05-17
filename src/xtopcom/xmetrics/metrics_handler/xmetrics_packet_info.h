// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "basic_handler.h"
#include "xbase/xns_macro.h"
#include "xmetrics/Variant.h"

NS_BEG3(top, metrics, handler)

using val_unit = Variant<std::string, int64_t>;

struct metrics_pack_unit {
    std::string name;
    std::vector<std::pair<std::string, val_unit>> pack_content;
    metrics_pack_unit(std::string _name) : name{_name} {
    }
};

void metrics_packet_impl(metrics_pack_unit & unit);

template <class K, class V, class... Args>
void metrics_packet_impl(metrics_pack_unit & unit, K key, V value, Args... rest) {
    static_assert(sizeof...(rest) % 2 == 0, "key-value should come in pairs");
    unit.pack_content.push_back(std::make_pair(key, value));
    metrics_packet_impl(unit, rest...);
}

NS_END3