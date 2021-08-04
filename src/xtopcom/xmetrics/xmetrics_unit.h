// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xbase.h"
#include "xbase/xns_macro.h"
#include "xmetrics/Variant.h"
#include <assert.h>

#include <memory>
#include <set>
#include <chrono>
#include <vector>
#include <string>
NS_BEG2(top, metrics)

using time_point = std::chrono::system_clock::time_point;
using microseconds = std::chrono::microseconds;
auto const DEFAULT_TIMED_OUT = std::chrono::microseconds{5000000}; // todo make it configurable.

using metrics_appendant_info = Variant<std::string, int64_t, time_point>;
/* counter_unit:
 * recorder one metric_name of its trigged time or recorder some variable nums that
 * might add/minus
 */
struct metrics_counter_unit {
    std::string name;
    int64_t inner_val;
    uint64_t count;
    int64_t last_dump_val;

    metrics_counter_unit(std::string _name, int64_t _val) : name{_name}, inner_val{_val}, count{1}, last_dump_val{0} {};
};
using metrics_counter_unit_ptr = std::shared_ptr<metrics_counter_unit>;

/* timer_unit:
 * recorder one function or some operation used timer by endtime minus starttime (ms)
 * calc min_time \ max_time \ avg_time(by sum_time divide count) automatically.
 */
struct metrics_timer_unit {
    std::set<std::pair<std::string, time_point>> name_start_timestamp_set;
    std::string name;
    uint64_t count;
    uint64_t last_dump_count;
    microseconds min_time;
    microseconds max_time;
    microseconds sum_time;
    metrics_appendant_info info;
    microseconds timed_out;

    // for init:
    metrics_timer_unit(std::string _name, std::string r_name, time_point _val)
      : name{r_name}, count{0}, last_dump_count{0}, min_time{microseconds::max()}, max_time{0}, sum_time{0} {
        name_start_timestamp_set.insert(std::make_pair(_name, _val));
    };
};
using metrics_timer_unit_ptr = std::shared_ptr<metrics_timer_unit>;

/* flow_unit:
 * recorder one metrics_name of its nums each time,
 * calc min_flow \ max_flow \ avg_flow(by sum_flow divide count) automatically.
 */
struct metrics_flow_unit {
    std::string name;
    uint64_t count;
    uint64_t last_dump_count;
    int64_t min_flow;
    int64_t max_flow;
    int64_t sum_flow;

    time_point last_dump_time;
    int64_t tps_flow;
    // uint64_t tps;

    metrics_flow_unit(std::string _name, int64_t _val, time_point _start_time)
      : name{_name}, count{1}, last_dump_count{0}, min_flow{_val}, max_flow{_val}, sum_flow{_val}, last_dump_time{_start_time}, tps_flow{_val} /*, tps{0}*/ {
    }
};
using metrics_flow_unit_ptr = std::shared_ptr<metrics_flow_unit>;

/* array_unit
 * recorder one metrics_name of its sub_array's each count(might add/minus/set each solely)
 * record all_count && calc sum when dump.
 */
struct metrics_array_unit{
    std::string name;
    uint64_t all_count;
    std::vector<uint64_t> array_count;
    std::vector<int64_t> array_value;
    uint64_t last_dump_count;

    metrics_array_unit(std::string _name, std::size_t _arr_size, int64_t _val)
      : name{_name}, all_count{0}, array_count(_arr_size, 0), array_value(_arr_size, 0), last_dump_count{0} {
    }
};
using metrics_array_unit_ptr = std::shared_ptr<metrics_array_unit>;

// ! the Variant order must be as same as e_metrics_major_id
// ! that is : [0-invalid], 1-counter 2-timer 3-flow
// ! or Variant.GetType() might be wrong.
using metrics_variant_ptr = Variant<metrics_counter_unit_ptr, metrics_timer_unit_ptr, metrics_flow_unit_ptr, metrics_array_unit_ptr>;

NS_END2