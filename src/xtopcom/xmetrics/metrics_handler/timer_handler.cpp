// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "timer_handler.h"

NS_BEG3(top, metrics, handler)

metrics_variant_ptr timer_handler_t::init_new_metrics(event_message const & msg) {
    auto metrics_real_name = msg.metrics_name.substr(0, msg.metrics_name.find("&0x", 0));
    return metrics_variant_ptr(std::make_shared<metrics_timer_unit>(msg.metrics_name, metrics_real_name, msg.metrics_value.GetConstRef<time_point>()));
}

void timer_handler_t::dump_metrics_info(metrics_variant_ptr const & metrics_ptr) {
    bool dump_json_format{false};
    XMETRICS_CONFIG_GET("dump_json_format", dump_json_format);
    auto const & ptr = metrics_ptr.GetConstRef<metrics_timer_unit_ptr>();

    if (dump_json_format) {
        json res, cont;
        res["category"] = get_category(ptr->name);
        res["tag"] = get_tag(ptr->name);
        res["type"] = "timer";
        cont["count"] = ptr->count;
        cont["max_time"] = ptr->max_time.count();
        cont["min_time"] = ptr->min_time.count();
        if (ptr->count) {
            cont["avg_time"] = ptr->sum_time.count() / ptr->count;
        } else {
            cont["avg_time"] = 0;
        }
        res["content"] = cont;
        std::stringstream ss;
        ss << res;
        dump(ss.str(), ptr->last_dump_count != ptr->count);
    } else {
        std::stringstream ss;
        ss.setf(std::ios::left, std::ios::adjustfield);
        ss.fill(' ');
        ss << std::setw(calc_dump_width(ptr->name.size())) << ptr->name << ":[count:" << ptr->count << " ,max_time:" << ptr->max_time.count()
           << " ,min_time:" << ptr->min_time.count() << " ,avg_time:";
        if (ptr->count) {
            ss << ptr->sum_time.count() / ptr->count << " (us)]";
        } else {
            ss << "0count ]";
        }
        dump(ss.str(), ptr->last_dump_count != ptr->count);
    }
    ptr->last_dump_count = ptr->count;
}

void timer_handler_t::process_message_event(metrics_variant_ptr & metrics_ptr, event_message const & msg) {
    auto ptr = metrics_ptr.GetRef<metrics_timer_unit_ptr>();
    assert(ptr);
    switch (msg.minor_id) {
    case e_metrics_minor_id::timestart:
        ptr->name_start_timestamp_set.insert(std::make_pair(msg.metrics_name, msg.metrics_value));
        // ptr->start_time_stamp = msg.metrics_value;
        break;
    case e_metrics_minor_id::timeend: {
        time_point start_time{time_point::min()};
        for (auto & p : ptr->name_start_timestamp_set) {
            if (p.first == msg.metrics_name) {
                start_time = p.second;
                ptr->name_start_timestamp_set.erase(p);
                break;
            }
        }
        if (start_time == time_point::min()) {
            // TOP-3301
            // unknown circumstance that metrics_processor got timer_end before timer_start. Or somehow timer_start message lost?.
            // thus cause assert_false(start_time != time_point::min()) in debug or miscalculation metrics in release.
            // temporary discard this metrics. Since less-count is better than mis-count.
            std::stringstream ss{};
            if (msg.appendant_infomation.GetType() == 1) {
                ss << msg.appendant_infomation.GetConstRef<std::string>();
            } else if (msg.appendant_infomation.GetType() == 2) {
                ss << msg.appendant_infomation.GetConstRef<int64_t>();
            }
            xwarn("[metrics] metrics timer end before start. name: %s, info: %s", ptr->name.c_str(), ss.str().c_str());
            break;
        }
        assert(start_time != time_point::min());
        if (msg.appendant_infomation.GetType()) {
            ptr->info = msg.appendant_infomation;
            ptr->timed_out = msg.timed_out;
        }
        auto t = std::chrono::duration_cast<microseconds>(msg.metrics_value.GetConstRef<time_point>() - start_time);
        if (t >= ptr->timed_out) {
            std::stringstream ss;
            ss << "[METRICS_ALERT]TIMED_OUT: " << ptr->name << " ,time:" << t.count() << "(us), appendant_info:";
            if (ptr->info.GetType() == 1) {
                ss << ptr->info.GetRef<std::string>();
            } else if (ptr->info.GetType() == 2) {
                ss << ptr->info.GetRef<int64_t>();
            } else {
                assert(0);
            }
            dump(ss.str(), true);
            //kinfo("%s", ss.str().c_str());

        }
        ptr->count++;
        ptr->sum_time += t;
        ptr->max_time = std::max(ptr->max_time, t);
        ptr->min_time = std::min(ptr->min_time, t);
        break;
    }
    default:
        xwarn(
            "XMETRICS_BUG: name: %s %d - %d, type: %d", msg.metrics_name.c_str(), static_cast<int>(msg.major_id), static_cast<int>(msg.minor_id), msg.appendant_infomation.GetType());
        // assert(false);
        break;
    }
    return;
}

NS_END3