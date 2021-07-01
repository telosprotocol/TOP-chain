// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "metrics_handler/counter_handler.h"
#include "metrics_handler/flow_handler.h"
#include "metrics_handler/timer_handler.h"
#include "metrics_handler/xmetrics_packet_info.h"
#include "xbasic/xrunnable.h"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#include "xmetrics_event.h"
#include "xmetrics_unit.h"

#include <chrono>
#include <map>
#include <string>
#include <thread>
NS_BEG2(top, metrics)

enum E_SIMPLE_METRICS_TAG {
    e_simple_begin = 0,
    blockstore_cache_block_total = e_simple_begin,
    dataobject_cur_xbase_type_cons_transaction,
    vhost_recv_msg,
    vhost_recv_callback,
    dataobject_tx_receipt_t,
    dataobject_unit_state,
    dataobject_xvtxindex,
    dataobject_xvbstate,
    dataobject_xvproperty,
    dataobject_xvaccountobj,
    dataobject_exeunit,
    dataobject_exegroup,
    dataobject_xvexecontxt,
    dataobject_xaccount_index,
    dataobject_xreceiptid_pair_t,
    dataobject_xvbindex_t,
    dataobject_xtransaction_t,
    dataobject_provcert,
    dataobject_xvaccount,
    dataobject_xvaction,
    dataobject_xvheader,
    dataobject_xvqcert,
    dataobject_xvblock,
    dataobject_xvinput,
    dataobject_xvoutput,
    dataobject_xventity,
    // db bock key, see xvdbkey for specific info
    // 't/', 'i/', 'b/'
    // 'b/.../h', 'b/.../i', 'b/.../ir', 'b/.../o', 'b/.../or', 'b/.../s', 'b/.../d'
    db_key_tx,
    db_key_block_index,
    db_key_block,
    db_key_block_object,
    db_key_block_input,
    db_key_block_input_resource,
    db_key_block_output,
    db_key_block_output_resource,
    db_key_block_state,
    db_key_block_offdata,

    // consensus
    cons_drand_leader_finish_succ,
    cons_drand_backup_finish_succ,
    cons_drand_leader_finish_fail,
    cons_drand_backup_finish_fail,
    cons_tableblock_leader_finish_succ,
    cons_tableblock_backup_finish_succ,
    cons_tableblock_leader_finish_fail,
    cons_tableblock_backup_finish_fail,

    // store
    store_db_read,
    store_db_write,
    store_db_delete,
    store_state_read,
    store_state_write,
    store_state_delete,
    store_block_read,
    store_block_index_read,
    store_block_input_read,
    store_block_output_read,
    store_block_call,
    store_block_write,
    store_block_input_write,
    store_block_output_write,
    store_block_delete,
    store_tx_index_self,
    store_tx_index_send,
    store_tx_index_recv,
    store_tx_index_confirm,
    store_tx_origin,

    // vledger dataobject
    dataobject_xvnode_t,
    dataobject_xvexestate_t,
    dataobject_xvnodegroup,
    dataobject_xcscoreobj_t,
    dataobject_xblock_maker_t,
    dataobject_xblockacct_t,
    dataobject_xtxpool_table_info_t,
    dataobject_xacctmeta_t,

    // message category
    message_category_begin,
    message_category_consensus = message_category_begin,
    message_category_timer,
    message_category_txpool,
    message_category_rpc,
    message_category_sync,
    message_block_broadcast,
    message_category_end,
    message_category_unknown = message_category_end,

    // sync 
    xsync_recv_new_block,
    xsync_recv_new_hash,
    xsync_recv_invalid_block,
    xsync_recv_duplicate_block,
    xsync_recv_block_size,
    e_simple_total,
};
using xmetircs_tag_t = E_SIMPLE_METRICS_TAG;

class e_metrics : public top::xbasic_runnable_t<e_metrics> {
public:
    XDECLARE_DELETED_COPY_AND_MOVE_SEMANTICS(e_metrics);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(e_metrics);

    static e_metrics & get_instance() {
        static e_metrics instance;
        return instance;
    }

    void start() override;
    void stop() override;

private:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(e_metrics);
    void run_process();
    void process_message_queue();
    void update_dump();
    void gauge_dump();

public:
    void timer_start(std::string metrics_name, time_point value);
    void timer_end(std::string metrics_name, time_point value, metrics_appendant_info info = 0, microseconds timed_out = DEFAULT_TIMED_OUT);
    void counter_increase(std::string metrics_name, int64_t value);
    void counter_decrease(std::string metrics_name, int64_t value);
    void counter_set(std::string metrics_name, int64_t value);
    void flow_count(std::string metrics_name, int64_t value, time_point timestamp);
    void gauge(E_SIMPLE_METRICS_TAG tag, int64_t value);

private:
    std::thread m_process_thread;
    std::size_t m_dump_interval;
    std::chrono::microseconds m_queue_procss_behind_sleep_time;
    handler::counter_handler_t m_counter_handler;
    handler::timer_handler_t m_timer_handler;
    handler::flow_handler_t m_flow_handler;
    constexpr static std::size_t message_queue_size{500000};
    top::threading::xthreadsafe_queue<event_message, std::vector<event_message>> m_message_queue{message_queue_size};
    std::map<std::string, metrics_variant_ptr> m_metrics_hub;  // {metrics_name, metrics_vaiant_ptr}
protected:
    struct simple_counter{
        std::atomic_long value;
        std::atomic_long call_count;
    };
    static simple_counter s_counters[e_simple_total]; // simple counter counter
    static metrics_variant_ptr s_metrics[e_simple_total]; // simple metrics dump info
};

class metrics_time_auto {
public:
    metrics_time_auto(std::string metrics_name, metrics_appendant_info key = std::string{"NOT SET appendant_info"}, microseconds timed_out = DEFAULT_TIMED_OUT)
      : m_metrics_name{metrics_name}, m_key{key}, m_timed_out{timed_out} {
        char c[15] = {0};
        snprintf(c, 14, "%p", (void*)(this));
        m_metrics_name = m_metrics_name + "&" + c;
        e_metrics::get_instance().timer_start(m_metrics_name, std::chrono::system_clock::now());
    }
    ~metrics_time_auto() { e_metrics::get_instance().timer_end(m_metrics_name, std::chrono::system_clock::now(), m_key, m_timed_out); }

    std::string m_metrics_name;
    metrics_appendant_info m_key;
    microseconds m_timed_out;
};

#ifdef ENABLE_METRICS
#define XMETRICS_INIT()                                                                                                                                                            \
    {                                                                                                                                                                              \
        auto & ins = top::metrics::e_metrics::get_instance();                                                                                                                      \
        ins.start();                                                                                                                                                               \
    }

#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream()  << x)).str()
#define ADD_THREAD_HASH(metrics_name) SSTR(metrics_name) + "&0x" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()))
#define STR_CONCAT_IMPL(a, b) a##b
#define STR_CONCAT(str_a, str_b) STR_CONCAT_IMPL(str_a, str_b)

#define MICROSECOND(timeout)                                                                                                                                                       \
    std::chrono::microseconds { timeout }

#define XMETRICS_TIME_RECORD(metrics_name)                                                                                                                                         \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name }
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)                                                                                                                                \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key }
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                             \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key, MICROSECOND(timeout) }

#define XMETRICS_TIMER_START(metrics_name) top::metrics::e_metrics::get_instance().timer_start(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP(metrics_name) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP_KEY(metrics_name, key) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key);

#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                              \
    top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key, MICROSECOND(timeout));

#define XMETRICS_COUNTER_INCREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_increase(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_decrease(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value) top::metrics::e_metrics::get_instance().counter_set(metrics_name, value)

#define XMETRICS_FLOW_COUNT(metrics_name, value) top::metrics::e_metrics::get_instance().flow_count(metrics_name, value, std::chrono::system_clock::now());

#define XMETRICS_PACKET_INFO(metrics_name, ...)                                                                                                                                    \
    top::metrics::handler::metrics_pack_unit STR_CONCAT(packet_info_auto_, __LINE__){metrics_name};                                                                                                       \
    top::metrics::handler::metrics_packet_impl(STR_CONCAT(packet_info_auto_, __LINE__), __VA_ARGS__);

#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), 1);
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), -1);
#define XMETRICS_GAUGE(TAG, value) top::metrics::e_metrics::get_instance().gauge(TAG, value)
#else
#define XMETRICS_INIT()
#define XMETRICS_TIME_RECORD(metrics_name)
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_TIMER_START(metrics_name)
#define XMETRICS_TIMER_STOP(metrics_name)
#define XMETRICS_TIMER_STOP_KEY(metrics_name, key)
#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_COUNTER_INCREMENT(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value)
#define XMETRICS_FLOW_COUNT(metrics_name, value)
#define XMETRICS_PACKET_INFO(metrics_name, ...)
#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key)
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key)
#define XMETRICS_GAUGE(TAG, value)
#endif

NS_END2
