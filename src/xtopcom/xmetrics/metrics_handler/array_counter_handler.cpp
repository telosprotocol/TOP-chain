// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "array_counter_handler.h"

NS_BEG3(top, metrics, handler)

metrics_variant_ptr array_counter_handler_t::init_new_metrics(event_message const & msg) {
    assert(false);
    // return metrics_variant_ptr(std::make_shared<metrics_array_unit_ptr>(msg.metrics_name, msg.size, msg.metrics_value.GetConstRef<int64_t>()));
    return nullptr;
}

void array_counter_handler_t::dump_metrics_info(metrics_variant_ptr const & metrics_ptr) {
    bool dump_json_format{false};
    XMETRICS_CONFIG_GET("dump_json_format", dump_json_format);
    auto const & ptr = metrics_ptr.GetConstRef<metrics_array_unit_ptr>();
    if (dump_json_format) {
        json res, cont;
        res["category"] = get_category(ptr->name);
        res["tag"] = get_tag(ptr->name);
        res["type"] = "array_counter";
        cont["count"] = ptr->all_count;
        cont["each_value"] = ptr->array_value;
        cont["each_count"] = ptr->array_count;
        // cont["each_value"] = json::parse(ptr->array_value.begin(), ptr->array_value.end());
        // cont["each_count"] = json::parse(ptr->array_count.begin(), ptr->array_count.end());
        res["content"] = cont;
        std::stringstream ss;
        ss << res;
        dump(ss.str(), ptr->all_count != ptr->last_dump_count);
    } else {
        std::stringstream ss;
        ss.setf(std::ios::left, std::ios::adjustfield);
        ss.fill(' ');
        // todo charles lack of each_value && each_count dump.
        ss << std::setw(calc_dump_width(ptr->name.size())) << ptr->name << ": [counter:" << ptr->all_count << "]";
        dump(ss.str(), ptr->all_count != ptr->last_dump_count);
    }
    ptr->last_dump_count = ptr->all_count;
}

void array_counter_handler_t::process_message_event(metrics_variant_ptr & metrics_ptr, event_message const & msg) {
    // Temporarily, array counter use atomic interger rathan than metrics queue.
    assert(false);
    return;
}

NS_END3