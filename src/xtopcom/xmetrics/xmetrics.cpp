// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmetrics.h"

NS_BEG2(top, metrics)

void e_metrics::start() {
    if (running()) {
        return;
    }
    XMETRICS_CONFIG_GET("dump_interval", m_dump_interval);
    XMETRICS_CONFIG_GET("queue_procss_behind_sleep_time", m_queue_procss_behind_sleep_time);
    running(true);
    // auto self = shared_from_this();
    // threading::xbackend_thread::spawn([this, self] { run_process(); });
    m_process_thread = std::thread(&e_metrics::run_process, this);
    m_process_thread.detach();
}

void e_metrics::stop() {
    assert(running());
    running(false);
}

void e_metrics::run_process() {
    while (running()) {
        process_message_queue();
        std::this_thread::sleep_for(m_queue_procss_behind_sleep_time);
        update_dump();
    }
}

void e_metrics::process_message_queue() {
    auto message_v = m_message_queue.wait_and_pop_all();
    if (message_v.empty())
        return;
    for (auto & msg_event : message_v) {
        if (msg_event.metrics_name.empty())
            continue;
        metrics::metrics_variant_ptr metrics_ptr;
        auto metrics_real_name = msg_event.metrics_name.substr(0, msg_event.metrics_name.find("&0x", 0));
        if (m_metrics_hub.find(metrics_real_name) == m_metrics_hub.end()) {
            switch (msg_event.major_id) {
            case e_metrics_major_id::count:
                metrics_ptr = m_counter_handler.init_new_metrics(msg_event);
                break;
            case e_metrics_major_id::timer:
                metrics_ptr = m_timer_handler.init_new_metrics(msg_event);
                break;
            case e_metrics_major_id::flow:
                metrics_ptr = m_flow_handler.init_new_metrics(msg_event);
                break;
            default:
                assert(false);
                break;
            }
            m_metrics_hub.insert({metrics_real_name, metrics_ptr});
            assert(m_metrics_hub.count(metrics_real_name));
        } else {
            metrics_ptr = m_metrics_hub[metrics_real_name];
            auto index = static_cast<metrics::e_metrics_major_id>(metrics_ptr.GetType());
            switch (index) {
            case metrics::e_metrics_major_id::count:
                m_counter_handler.process_message_event(metrics_ptr, msg_event);
                break;
            case metrics::e_metrics_major_id::timer:
                m_timer_handler.process_message_event(metrics_ptr, msg_event);
                break;
            case metrics::e_metrics_major_id::flow:
                m_flow_handler.process_message_event(metrics_ptr, msg_event);
                break;
            default:
                break;
            }
        }
    }
}

void e_metrics::update_dump() {
    if (--m_dump_interval)
        return;
    auto const & map_ref = m_metrics_hub;
    for (auto const & pair : map_ref) {
        auto const & metrics_ptr = pair.second;
        auto index = static_cast<metrics::e_metrics_major_id>(pair.second.GetType());
        switch (index) {
        case metrics::e_metrics_major_id::count:
            m_counter_handler.dump_metrics_info(metrics_ptr);
            break;
        case metrics::e_metrics_major_id::timer:
            m_timer_handler.dump_metrics_info(metrics_ptr);
            break;
        case metrics::e_metrics_major_id::flow:
            m_flow_handler.dump_metrics_info(metrics_ptr);
            break;
        default:
            assert(false);
            break;
        }
    }
    XMETRICS_CONFIG_GET("dump_interval", m_dump_interval);
}

/*
 * one funtion for one [major_id-minor_id] message_event
 */

void e_metrics::timer_start(std::string metrics_name, time_point time) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::timer, metrics::e_metrics_minor_id::timestart, metrics_name, time));
}
void e_metrics::timer_end(std::string metrics_name, time_point time, metrics_appendant_info info, microseconds timed_out) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::timer, metrics::e_metrics_minor_id::timeend, metrics_name, time, info, timed_out));
}

void e_metrics::counter_increase(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::increase, metrics_name, value));
}
void e_metrics::counter_decrease(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::decrease, metrics_name, value));
}
void e_metrics::counter_set(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::set, metrics_name, value));
}
void e_metrics::flow_count(std::string metrics_name, int64_t value, time_point timestamp) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::flow, metrics::e_metrics_minor_id::flow_count, metrics_name, value, metrics_appendant_info{timestamp}));
}
NS_END2