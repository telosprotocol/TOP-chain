// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmetrics_unit.h"
NS_BEG2(top, metrics)

enum class e_metrics_major_id : size_t {
    count = 1,
    timer = 2,
    flow = 3,
    array = 4, // only support directly used with atomic interger for now (lack of metrics queue relevant code (todo charles))
};

enum class e_metrics_minor_id : size_t {
    increase = 1,
    decrease,
    set,
    timestart,
    timeend,
    flow_count,
    flow_timestamp,
};

struct event_message {
    e_metrics_major_id major_id;
    e_metrics_minor_id minor_id;
    std::string metrics_name;
    Variant<int64_t, time_point> metrics_value;
    microseconds timed_out;

    metrics_appendant_info appendant_infomation;

    event_message(){};
    event_message(e_metrics_major_id _major_id, e_metrics_minor_id _minor_id, std::string _name, Variant<int64_t, time_point> _val) {
        major_id = _major_id;
        minor_id = _minor_id;
        metrics_name = _name;
        metrics_value = _val;
    };
    event_message(e_metrics_major_id _major_id,
                  e_metrics_minor_id _minor_id,
                  std::string _name,
                  Variant<int64_t, time_point> _val,
                  metrics_appendant_info _info,
                  microseconds _time_out = DEFAULT_TIMED_OUT) {
        major_id = _major_id;
        minor_id = _minor_id;
        metrics_name = _name;
        metrics_value = _val;
        appendant_infomation = _info;
        timed_out = _time_out;
    };
};

NS_END2