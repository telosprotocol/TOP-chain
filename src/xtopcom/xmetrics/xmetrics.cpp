// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmetrics.h"

NS_BEG2(top, metrics)

// simple metrics counters
e_metrics::simple_counter e_metrics::s_counters[e_simple_total];

#ifndef DECL_METRICS
#define DECL_METRICS(tag)  std::make_shared<metrics_counter_unit>(#tag, 0)
#endif

// simple metrics description array
metrics_variant_ptr e_metrics::s_metrics[e_simple_total] = {
    DECL_METRICS(blockstore_cache_block_total),  //blockstore_cache_block_total
    DECL_METRICS(dataobject_cur_xbase_type_cons_transaction),  //txpool_cons_transaction
    DECL_METRICS(vhost_recv_msg), // vhost_recv_msg,
    DECL_METRICS(vhost_recv_callback), // vhost_recv_callback,
    DECL_METRICS(dataobject_tx_receipt_t), // tx_receipt
    DECL_METRICS(dataobject_unit_state), // unit_state
    DECL_METRICS(dataobject_xvtxindex), // xvtxindex
    DECL_METRICS(dataobject_xvbstate), // xvbstate
    DECL_METRICS(dataobject_xvproperty), // xvproperty
    DECL_METRICS(dataobject_account), // account
    DECL_METRICS(dataobject_exeunit), // exeunit
    DECL_METRICS(dataobject_exegroup), // exegroup
    DECL_METRICS(dataobject_xvexecontxt), // xvexecontxt
    DECL_METRICS(dataobject_xaccount_index), // xvexecontxt
    DECL_METRICS(dataobject_xreceiptid_pair_t), // xreceiptid_pair_t
    DECL_METRICS(dataobject_xvbindex_t), // dataobject_xvbindex_t
    DECL_METRICS(dataobject_xtransaction_t), // dataobject_xtransaction_t
    DECL_METRICS(dataobject_provcert),
    DECL_METRICS(dataobject_xvaccount),
    DECL_METRICS(dataobject_xvaction),
    DECL_METRICS(dataobject_xvheader),
    DECL_METRICS(dataobject_xvqcert),
    DECL_METRICS(dataobject_xvblock),
    DECL_METRICS(dataobject_xvinput),
    DECL_METRICS(dataobject_xvoutput),
    DECL_METRICS(dataobject_xventity),

    //db key value metrics (see declare)
    DECL_METRICS(db_key_tx),
    DECL_METRICS(db_key_block_index),
    DECL_METRICS(db_key_block),
    DECL_METRICS(db_key_block_object),
    DECL_METRICS(db_key_block_input),
    DECL_METRICS(db_key_block_input_resource),
    DECL_METRICS(db_key_block_output),
    DECL_METRICS(db_key_block_output_resource),
    DECL_METRICS(db_key_block_state),
    DECL_METRICS(db_key_block_offdata),

    // consensus
    DECL_METRICS(cons_drand_leader_finish_succ),
    DECL_METRICS(cons_drand_backup_finish_succ),
    DECL_METRICS(cons_drand_leader_finish_fail),
    DECL_METRICS(cons_drand_backup_finish_fail),
    DECL_METRICS(cons_tableblock_leader_finish_succ),
    DECL_METRICS(cons_tableblock_backup_finish_succ),
    DECL_METRICS(cons_tableblock_leader_finish_fail),
    DECL_METRICS(cons_tableblock_backup_finish_fail),

    // store
    DECL_METRICS(store_db_read),
    DECL_METRICS(store_db_write),
    DECL_METRICS(store_db_delete),
    DECL_METRICS(store_state_read),
    DECL_METRICS(store_state_write),
    DECL_METRICS(store_state_delete),
    DECL_METRICS(store_block_read),
    DECL_METRICS(store_block_index_read),
    DECL_METRICS(store_block_input_read),
    DECL_METRICS(store_block_output_read),
    DECL_METRICS(store_block_write_call),
    DECL_METRICS(store_block_write),
    DECL_METRICS(store_block_delete),
};

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
    if (message_v.size() > 50000) {
        xkinfo("[xmetrics]alarm metrics_queue_size %zu", message_v.size());
    }
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

    // simpe metrics dump
    gauge_dump();
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
void e_metrics::gauge(E_SIMPLE_METRICS_TAG tag, int64_t value) {
    if (tag >= e_simple_total || tag < e_simple_begin ) {
        return;
    }
    s_counters[tag].value += value;
    s_counters[tag].call_count++;
}

void e_metrics::gauge_dump() {
    for(auto index = (int32_t)e_simple_begin; index < (int32_t)e_simple_total; index++) {
        auto metrics_ptr = s_metrics[index];
        auto ptr = metrics_ptr.GetRef<metrics_counter_unit_ptr>();
        ptr->inner_val = s_counters[index].value;
        ptr->count = s_counters[index].call_count;
        m_counter_handler.dump_metrics_info(ptr);
    }
}
NS_END2
