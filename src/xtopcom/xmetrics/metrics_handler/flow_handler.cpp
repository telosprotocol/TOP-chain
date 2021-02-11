// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "flow_handler.h"

NS_BEG3(top, metrics, handler)

metrics_variant_ptr flow_handler_t::init_new_metrics(event_message const & msg) {
    return metrics_variant_ptr(std::make_shared<metrics_flow_unit>(msg.metrics_name, msg.metrics_value.GetConstRef<int64_t>(), msg.appendant_infomation.GetConstRef<time_point>()));
}

void flow_handler_t::dump_metrics_info(metrics_variant_ptr const & metrics_ptr) {
    bool dump_json_format{false};
    XMETRICS_CONFIG_GET("dump_json_format", dump_json_format);
    auto const & ptr = metrics_ptr.GetConstRef<metrics_flow_unit_ptr>();
    // calc tps:
    time_point _now = std::chrono::system_clock::now();
    microseconds time_interval = std::chrono::duration_cast<microseconds>(_now - ptr->last_dump_time);
    double tps{0.00};
    if (time_interval != microseconds{0}) {
        tps = static_cast<double>(ptr->tps_flow * 1000) / (static_cast<double>(time_interval.count()) / 1000);
    }
    if (dump_json_format) {
        json res, cont;
        res["category"] = get_category(ptr->name);
        res["tag"] = get_tag(ptr->name);
        res["type"] = "flow";
        cont["count"] = ptr->count;
        cont["max_flow"] = ptr->max_flow;
        cont["min_flow"] = ptr->min_flow;
        cont["sum_flow"] = ptr->sum_flow;
        if (ptr->count) {
            cont["avg_flow"] = ptr->sum_flow / ptr->count;
        } else {
            cont["avg_flow"] = ptr->sum_flow;
        }
        cont["tps_flow"] = ptr->tps_flow;
        std::stringstream tps_ss;
        tps_ss << std::fixed << std::setprecision(2) << tps;
        cont["tps"] = tps_ss.str();
        res["content"] = cont;
        std::stringstream ss;
        ss << res;
        dump(ss.str(), ptr->last_dump_count != ptr->count);
    } else {
        std::stringstream ss;
        ss.setf(std::ios::left, std::ios::adjustfield);
        ss.fill(' ');
        ss << std::setw(calc_dump_width(ptr->name.size())) << ptr->name << ": [count:" << ptr->count << ",max_flow:" << ptr->max_flow << ",min_flow:" << ptr->min_flow
           << ",avg_flow:";
        if (ptr->count) {
            ss << ptr->sum_flow / ptr->count;
        } else {
            ss << ptr->sum_flow << "(0 count)";
        }
        ss << " ,elpase(us):" << time_interval.count();
        ss << ",tps_flow:" << ptr->tps_flow << ",tps:" << std::fixed << std::setprecision(2) << tps << "/s]";
        dump(ss.str(), ptr->last_dump_count != ptr->count);
    }
    ptr->last_dump_count = ptr->count;
    ptr->last_dump_time = _now;
    ptr->tps_flow = 0;
}

void flow_handler_t::process_message_event(metrics_variant_ptr & metrics_ptr, event_message const & msg) {
    auto ptr = metrics_ptr.GetRef<metrics_flow_unit_ptr>();
    assert(ptr);
    switch (msg.minor_id) {
    case e_metrics_minor_id::flow_count: {
        ptr->count++;
        ptr->sum_flow += msg.metrics_value.GetConstRef<int64_t>();
        ptr->max_flow = std::max(ptr->max_flow, msg.metrics_value.GetConstRef<int64_t>());
        ptr->min_flow = std::min(ptr->min_flow, msg.metrics_value.GetConstRef<int64_t>());
        ptr->tps_flow += msg.metrics_value.GetConstRef<int64_t>();
        break;
    }
    default:
        xwarn("XMETRICS_BUG: name: %s %d - %d, type: %d",
              msg.metrics_name.c_str(),
              static_cast<int>(msg.major_id),
              static_cast<int>(msg.minor_id),
              msg.appendant_infomation.GetType());
        // assert(0);
        break;
    }
    return;
}

NS_END3