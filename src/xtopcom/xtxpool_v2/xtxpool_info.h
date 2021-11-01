// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xtransaction.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xverifier_utl.h"

namespace top {
namespace xtxpool_v2 {

using namespace top::data;

#define table_send_tx_queue_size_max (1024)
#define table_recv_tx_queue_size_max (1024)
#define table_conf_tx_queue_size_max (1024)

#define role_send_tx_queue_size_max_for_each_table (800)
#define role_recv_tx_queue_size_max_for_each_table (800)
#define role_confirm_tx_queue_size_max_for_each_table (800)

class xtx_counter_t {
public:
    void send_tx_inc(int32_t count) {
        xassert(m_send_tx_count + count >= 0);
        m_send_tx_count += count;
    }
    void recv_tx_inc(int32_t count) {
        xassert(m_recv_tx_count + count >= 0);
        m_recv_tx_count += count;
    }
    void conf_tx_inc(int32_t count) {
        xassert(m_conf_tx_count + count >= 0);
        m_conf_tx_count += count;
    }
    void unconfirm_tx_inc(int32_t count) {
        xassert(m_unconfirm_tx_count + count >= 0);
        m_unconfirm_tx_count += count;
    }
    void set_unconfirm_tx_count(uint32_t count) {
        m_unconfirm_tx_count = count;
    }
    int32_t get_send_tx_count() const {
        return m_send_tx_count;
    }
    int32_t get_recv_tx_count() const {
        return m_recv_tx_count;
    }
    int32_t get_conf_tx_count() const {
        return m_conf_tx_count;
    }
    int32_t get_unconfirm_tx_count() const {
        return m_unconfirm_tx_count;
    }

private:
    std::atomic<int32_t> m_send_tx_count{0};
    std::atomic<int32_t> m_recv_tx_count{0};
    std::atomic<int32_t> m_conf_tx_count{0};
    std::atomic<int32_t> m_unconfirm_tx_count{0};
};

class xtxpool_statistic_t {
public:
    void inc_table_num(uint32_t num) {
        m_table_num += num;
    }
    void dec_table_num(uint32_t num) {
        m_table_num -= num;
    }
    void inc_unconfirm_tx_num(int32_t num) {
        m_unconfirm_tx_num += num;
    }
    void inc_unconfirm_tx_cache_num(uint32_t num) {
        m_unconfirm_tx_cache_num += num;
    }
    void dec_unconfirm_tx_cache_num(uint32_t num) {
        m_unconfirm_tx_cache_num -= num;
    }

    void update_receipt_recv_num(const xcons_transaction_ptr_t & tx, bool is_pulled) {
        if (tx->is_recv_tx()) {
            if (is_pulled) {
                m_pulled_recv_tx_num++;
            } else {
                m_received_recv_tx_num++;
            }
        } else {
            if (is_pulled) {
                m_pulled_confirm_tx_num++;
            } else {
                m_received_confirm_tx_num++;
            }
        }

        uint64_t now = xverifier::xtx_utl::get_gmttime_s();
        uint64_t tx_time = tx->get_receipt_gmtime();
        if (now < tx_time) {
            xtxpool_info("xtxpool_t::update_receipt_recv_num time not fit with other nodes,now:%llu tx_time:%llu,tx:%s", now, tx_time, tx->dump().c_str());
            // m_receipt_recv_num_by_1_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_1_clock, 1);
        } else if (now - tx_time < 10) {
            // m_receipt_recv_num_by_1_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_1_clock, 1);
        } else if (now - tx_time < 20) {
            // m_receipt_recv_num_by_2_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_2_clock, 1);
        } else if (now - tx_time < 30) {
            // m_receipt_recv_num_by_3_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_3_clock, 1);
        } else if (now - tx_time < 40) {
            // m_receipt_recv_num_by_4_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_4_clock, 1);
        } else if (now - tx_time < 50) {
            // m_receipt_recv_num_by_5_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_5_clock, 1);
        } else if (now - tx_time < 60) {
            // m_receipt_recv_num_by_6_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_by_6_clock, 1);
        } else if (now - tx_time < 120) {
            // m_receipt_recv_num_7to12_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_7to12_clock, 1);
        } else if (now - tx_time < 300) {
            // m_receipt_recv_num_13to30_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_13to30_clock, 1);
            xtxpool_info("xtxpool_t::update_receipt_recv_num receipt came too late,delay:%llu,tx:%s", now - tx_time, tx->dump().c_str());
        } else {
            // m_receipt_recv_num_exceed_30_clock++;
            XMETRICS_GAUGE(metrics::txpool_receipt_recv_num_exceed_30_clock, 1);
            xtxpool_warn("xtxpool_t::update_receipt_recv_num receipt came delay exceed 300s,delay:%llu,tx:%s", now - tx_time, tx->dump().c_str());
        }
    }
    void inc_push_tx_send_cur_num(uint32_t num) {
        m_push_tx_send_cur_num += num;
    }
    void dec_push_tx_send_cur_num(uint32_t num) {
        m_push_tx_send_cur_num -= num;
    }
    void inc_push_tx_recv_cur_num(uint32_t num) {
        m_push_tx_recv_cur_num += num;
    }
    void dec_push_tx_recv_cur_num(uint32_t num) {
        m_push_tx_recv_cur_num -= num;
    }
    void inc_push_tx_confirm_cur_num(uint32_t num) {
        m_push_tx_confirm_cur_num += num;
    }
    void dec_push_tx_confirm_cur_num(uint32_t num) {
        m_push_tx_confirm_cur_num -= num;
    }
    void inc_push_tx_send_fail_num(uint32_t num) {
        m_push_tx_send_fail_num += num;
    }
    void inc_push_tx_receipt_fail_num(uint32_t num) {
        m_push_tx_receipt_fail_num += num;
    }
    void inc_receipt_duplicate_num(uint32_t num) {
        m_receipt_duplicate_num += num;
    }
    void inc_receipt_repeat_num(uint32_t num) {
        m_receipt_repeat_num += num;
    }

    void print() const {
        XMETRICS_PACKET_INFO("txpool_state",
                             "table_num",
                             m_table_num.load(),
                             "unconfirm",
                             m_unconfirm_tx_num.load(),
                             "received_recv",
                             m_received_recv_tx_num.load(),
                             "received_confirm",
                             m_received_confirm_tx_num.load(),
                             "pulled_recv",
                             m_pulled_recv_tx_num.load(),
                             "pulled_confirm",
                             m_pulled_confirm_tx_num.load());

        // XMETRICS_PACKET_INFO("txpool_receipt_delay",
        //                      "1clk",
        //                      m_receipt_recv_num_by_1_clock.load(),
        //                      "2clk",
        //                      m_receipt_recv_num_by_2_clock.load(),
        //                      "3clk",
        //                      m_receipt_recv_num_by_3_clock.load(),
        //                      "4clk",
        //                      m_receipt_recv_num_by_4_clock.load(),
        //                      "5clk",
        //                      m_receipt_recv_num_by_5_clock.load(),
        //                      "6clk",
        //                      m_receipt_recv_num_by_6_clock.load(),
        //                      "7to12clk",
        //                      m_receipt_recv_num_7to12_clock.load(),
        //                      "13to30clk",
        //                      m_receipt_recv_num_13to30_clock.load(),
        //                      "ex30clk",
        //                      m_receipt_recv_num_exceed_30_clock.load());

        XMETRICS_PACKET_INFO("txpool_cache",
                             "send_cur",
                             m_push_tx_send_cur_num.load(),
                             "recv_cur",
                             m_push_tx_recv_cur_num.load(),
                             "confirm_cur",
                             m_push_tx_confirm_cur_num.load(),
                             "unconfirm_cur",
                             m_unconfirm_tx_cache_num.load(),
                             "push_send_fail",
                             m_push_tx_send_fail_num.load(),
                             "push_receipt_fail",
                             m_push_tx_receipt_fail_num.load(),
                             "duplicate",
                             m_receipt_duplicate_num.load(),
                             "repeat",
                             m_receipt_repeat_num.load());
    }

    // std::string state_dump() const {
    //     char local_param_buf[256];
    //     xprintf(local_param_buf,
    //             sizeof(local_param_buf),
    //             "table num:%u unconfirm num:%d received recv:%u conf:%u pulled recv:%u conf:%u",
    //             m_table_num.load(),
    //             m_unconfirm_tx_num.load(),
    //             m_received_recv_tx_num.load(),
    //             m_received_confirm_tx_num.load(),
    //             m_pulled_recv_tx_num.load(),
    //             m_pulled_confirm_tx_num.load());
    //     return std::string(local_param_buf);
    // }

    // std::string receipt_delay_dump() const {
    //     char local_param_buf[256];
    //     xprintf(local_param_buf,
    //             sizeof(local_param_buf),
    //             "receipt recv num by clock:%u:%u:%u:%u:%u:%u:%u:%u:%u",
    //             m_receipt_recv_num_by_1_clock.load(),
    //             m_receipt_recv_num_by_2_clock.load(),
    //             m_receipt_recv_num_by_3_clock.load(),
    //             m_receipt_recv_num_by_4_clock.load(),
    //             m_receipt_recv_num_by_5_clock.load(),
    //             m_receipt_recv_num_by_6_clock.load(),
    //             m_receipt_recv_num_7to12_clock.load(),
    //             m_receipt_recv_num_13to30_clock.load(),
    //             m_receipt_recv_num_exceed_30_clock.load());
    //     return std::string(local_param_buf);
    // }

    // std::string cache_dump() const {
    //     char local_param_buf[256];
    //     xprintf(local_param_buf,
    //             sizeof(local_param_buf),
    //             "tx cur cache:%u:%u:%u unconfirm cache:%u push fail:%u:%u dupulicate:%u repeat:%u",
    //             m_push_tx_send_cur_num.load(),
    //             m_push_tx_recv_cur_num.load(),
    //             m_push_tx_confirm_cur_num.load(),
    //             m_unconfirm_tx_cache_num.load(),
    //             m_push_tx_send_fail_num.load(),
    //             m_push_tx_receipt_fail_num.load(),
    //             m_receipt_duplicate_num.load(),
    //             m_receipt_repeat_num.load());
    //     return std::string(local_param_buf);
    // }

private:
    std::atomic<uint32_t> m_table_num{0};
    std::atomic<int32_t> m_unconfirm_tx_num{0};
    std::atomic<uint32_t> m_unconfirm_tx_cache_num{0};
    std::atomic<uint32_t> m_received_recv_tx_num{0};
    std::atomic<uint32_t> m_received_confirm_tx_num{0};
    std::atomic<uint32_t> m_pulled_recv_tx_num{0};
    std::atomic<uint32_t> m_pulled_confirm_tx_num{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_1_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_2_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_3_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_4_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_5_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_by_6_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_7to12_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_13to30_clock{0};
    // std::atomic<uint32_t> m_receipt_recv_num_exceed_30_clock{0};
    std::atomic<uint32_t> m_push_tx_send_cur_num{0};
    std::atomic<uint32_t> m_push_tx_recv_cur_num{0};
    std::atomic<uint32_t> m_push_tx_confirm_cur_num{0};
    std::atomic<uint32_t> m_push_tx_send_fail_num{0};
    std::atomic<uint32_t> m_push_tx_receipt_fail_num{0};
    std::atomic<uint32_t> m_receipt_duplicate_num{0};
    std::atomic<uint32_t> m_receipt_repeat_num{0};
};

class xtxpool_role_info_t : public xtx_counter_t {
public:
    xtxpool_role_info_t(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type)
      : m_zone(zone), m_front_table_id(front_table_id), m_back_table_id(back_table_id), m_node_type(node_type) {
        // m_max_send_tx_num = role_send_tx_queue_size_max_for_each_table * (back_table_id + 1 - front_table_id);
        // m_max_recv_tx_num = role_recv_tx_queue_size_max_for_each_table * (back_table_id + 1 - front_table_id);
        // m_max_confirm_tx_num = role_confirm_tx_queue_size_max_for_each_table * (back_table_id + 1 - front_table_id);
    }
    bool is_ids_match(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) const {
        return (m_zone == zone && m_front_table_id == front_table_id && m_back_table_id == back_table_id && m_node_type == node_type);
    }
    bool is_id_contained(uint8_t zone, uint16_t table_id) const {
        return (zone == m_zone && table_id >= m_front_table_id && table_id <= m_back_table_id);
    }
    uint8_t get_zone() const {
        return m_zone;
    }
    uint16_t get_front_table_id() const {
        return m_front_table_id;
    }
    uint16_t get_back_table_id() const {
        return m_back_table_id;
    }
    common::xnode_type_t get_node_type() const {
        return m_node_type;
    }
    void add_sub_count() {
        m_sub_count++;
    }
    void del_sub_count() {
        m_sub_count--;
    }
    uint8_t get_sub_count() const {
        return m_sub_count.load();
    }

private:
    uint8_t m_zone;
    uint16_t m_front_table_id;
    uint16_t m_back_table_id;
    common::xnode_type_t m_node_type;
    std::atomic<uint8_t> m_sub_count{0};
};

class xtxpool_table_info_t : public base::xvaccount_t {
public:
    xtxpool_table_info_t() = delete;
    xtxpool_table_info_t(const xtxpool_table_info_t &) = delete;
    xtxpool_table_info_t(const std::string & address,
                         xtxpool_role_info_t * role,
                         xtxpool_statistic_t * statistic,
                         xtable_state_cache_t * table_state_cache,
                         std::set<base::xtable_shortid_t> * all_table_sids = nullptr)
      : base::xvaccount_t(address), m_statistic(statistic), m_table_state_cache(table_state_cache), m_all_table_sids(all_table_sids) {
        XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtxpool_table_info_t, 1);
        m_roles.push_back(role);
    }
    ~xtxpool_table_info_t() {
        XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtxpool_table_info_t, -1);
        m_statistic->dec_push_tx_send_cur_num(m_counter.get_send_tx_count());
        m_statistic->dec_push_tx_recv_cur_num(m_counter.get_recv_tx_count());
        m_statistic->dec_push_tx_confirm_cur_num(m_counter.get_conf_tx_count());
        set_unconfirm_tx_count(0);

        XMETRICS_GAUGE(metrics::txpool_send_tx_cur, -m_counter.get_send_tx_count());
        XMETRICS_GAUGE(metrics::txpool_recv_tx_cur, -m_counter.get_recv_tx_count());
        XMETRICS_GAUGE(metrics::txpool_confirm_tx_cur, -m_counter.get_conf_tx_count());
        // XMETRICS_COUNTER_SET("table_send_tx_cur" + get_address(), 0);
        // XMETRICS_COUNTER_SET("table_recv_tx_cur" + get_address(), 0);
        // XMETRICS_COUNTER_SET("table_confirm_tx_cur" + get_address(), 0);
    }
    const std::string & get_table_addr() const {
        return get_address();
    }
    void send_tx_inc(int32_t count) {
        m_counter.send_tx_inc(count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("send_tx_inc role(%p) table:%s old send num:%d inc num:%d", role, get_address().c_str(), role->get_send_tx_count(), count);
        //     role->send_tx_inc(count);
        // }

        m_statistic->inc_push_tx_send_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_send_tx_cur, count);
        // XMETRICS_COUNTER_INCREMENT("table_send_tx_cur" + get_address(), count);
        xdbg("send_tx_inc table %s send queue size:%u", get_address().c_str(), m_counter.get_send_tx_count());
    }
    void send_tx_dec(int32_t count) {
        m_counter.send_tx_inc(-count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("send_tx_dec role(%p) table:%s old send num:%d dec num:%d", role, get_address().c_str(), role->get_send_tx_count(), count);
        //     role->send_tx_inc(-count);
        // }
        m_statistic->dec_push_tx_send_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_send_tx_cur, -count);
        // XMETRICS_COUNTER_DECREMENT("table_send_tx_cur" + get_address(), count);
        xdbg("send_tx_dec table %s send queue size:%u", get_address().c_str(), m_counter.get_send_tx_count());
    }
    void recv_tx_inc(int32_t count) {
        m_counter.recv_tx_inc(count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("recv_tx_inc role(%p) table:%s old recv num:%d inc num:%d", role, get_address().c_str(), role->get_recv_tx_count(), count);
        //     role->recv_tx_inc(count);
        // }
        m_statistic->inc_push_tx_recv_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_recv_tx_cur, count);
        // XMETRICS_COUNTER_INCREMENT("table_recv_tx_cur" + get_address(), count);
        xdbg("recv_tx_inc table %s recv queue size:%u", get_address().c_str(), m_counter.get_recv_tx_count());
    }
    void recv_tx_dec(int32_t count) {
        m_counter.recv_tx_inc(-count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("recv_tx_dec role(%p) table:%s old recv num:%d dec num:%d", role, get_address().c_str(), role->get_recv_tx_count(), count);
        //     role->recv_tx_inc(-count);
        // }
        m_statistic->dec_push_tx_recv_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_recv_tx_cur, -count);
        // XMETRICS_COUNTER_DECREMENT("table_recv_tx_cur" + get_address(), count);
        xdbg("recv_tx_dec table %s recv queue size:%u", get_address().c_str(), m_counter.get_recv_tx_count());
    }
    void conf_tx_inc(int32_t count) {
        m_counter.conf_tx_inc(count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("conf_tx_inc role(%p) table:%s old confirm num:%d inc num:%d", role, get_address().c_str(), role->get_conf_tx_count(), count);
        //     role->conf_tx_inc(count);
        // }
        m_statistic->inc_push_tx_confirm_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_confirm_tx_cur, count);
        // XMETRICS_COUNTER_INCREMENT("table_confirm_tx_cur" + get_address(), count);
        xdbg("conf_tx_inc table %s confirm queue size:%u", get_address().c_str(), m_counter.get_conf_tx_count());
    }
    void conf_tx_dec(int32_t count) {
        m_counter.conf_tx_inc(-count);
        // for (auto & role : m_roles) {
        //     xtxpool_dbg("conf_tx_dec role(%p) table:%s old confirm num:%d dec num:%d", role, get_address().c_str(), role->get_conf_tx_count(), count);
        //     role->conf_tx_inc(-count);
        // }
        m_statistic->dec_push_tx_confirm_cur_num(count);
        XMETRICS_GAUGE(metrics::txpool_confirm_tx_cur, -count);
        // XMETRICS_COUNTER_DECREMENT("table_confirm_tx_cur" + get_address(), count);
        xdbg("conf_tx_dec table %s confirm queue size:%u", get_address().c_str(), m_counter.get_conf_tx_count());
    }

    void tx_inc(enum_transaction_subtype subtype, int32_t count) {
        if (subtype == enum_transaction_subtype_confirm) {
            conf_tx_inc(count);
        } else if (subtype == enum_transaction_subtype_recv) {
            recv_tx_inc(count);
        } else {
            send_tx_inc(count);
        }
    }

    void tx_dec(enum_transaction_subtype subtype, int32_t count) {
        if (subtype == enum_transaction_subtype_confirm) {
            conf_tx_dec(count);
        } else if (subtype == enum_transaction_subtype_recv) {
            recv_tx_dec(count);
        } else {
            send_tx_dec(count);
        }
    }

    int32_t check_send_tx_reached_upper_limit() {
        if (m_counter.get_send_tx_count() >= table_send_tx_queue_size_max) {
            XMETRICS_GAUGE(metrics::txpool_alarm_send_tx_reached_upper_limit, 1);
            return xtxpool_error_table_reached_upper_limit;
        }/* else if (any_role_send_tx_reached_upper_limit()) {
            return xtxpool_error_role_reached_upper_limit;
        }*/
        return xsuccess;
    }

    bool is_recv_tx_reached_upper_limit() {
        if (m_counter.get_recv_tx_count() >= table_recv_tx_queue_size_max/* || any_role_recv_tx_reached_upper_limit()*/) {
            XMETRICS_GAUGE(metrics::txpool_alarm_recv_tx_reached_upper_limit, 1);
            return true;
        }
        return false;
    }

    bool is_confirm_tx_reached_upper_limit() {
        if (m_counter.get_conf_tx_count() >= table_conf_tx_queue_size_max/* || any_role_confirm_tx_reached_upper_limit()*/) {
            XMETRICS_GAUGE(metrics::txpool_alarm_confirm_tx_reached_upper_limit, 1);
            return true;
        }
        return false;
    }

    void set_unconfirm_tx_count(int32_t count) {
        int32_t old_count = m_counter.get_unconfirm_tx_count();
        if (count == old_count) {
            return;
        }
        // for (auto & role : m_roles) {
        //     xtxpool_dbg(
        //         "set_unconfirm_tx_count role(%p) table:%s old unconfirm count:%d inc count:%d", role, get_address().c_str(), role->get_unconfirm_tx_count(), count - old_count);
        //     role->unconfirm_tx_inc(count - old_count);
        // }
        m_statistic->inc_unconfirm_tx_num(count - old_count);
        XMETRICS_GAUGE(metrics::txpool_unconfirm_tx_cur, count - old_count);
        m_counter.set_unconfirm_tx_count(count);
    }

    xtxpool_statistic_t * get_statistic() {
        return m_statistic;
    }

    // bool any_role_send_tx_reached_upper_limit() {
    //     for (auto & role : m_roles) {
    //         if (role->send_tx_full()) {
    //             xwarn("any_role_send_tx_reached_upper_limit table %s role send queue size:%u", get_address().c_str(), role->get_send_tx_count());
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    // bool any_role_recv_tx_reached_upper_limit() {
    //     for (auto & role : m_roles) {
    //         if (role->recv_tx_full()) {
    //             xwarn("any_role_recv_tx_reached_upper_limit table %s role recv queue size:%u", get_address().c_str(), role->get_recv_tx_count());
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    // bool any_role_confirm_tx_reached_upper_limit() {
    //     for (auto & role : m_roles) {
    //         if (role->confirm_tx_full()) {
    //             xwarn("any_role_confirm_tx_reached_upper_limit table %s role confirm queue size:%u", get_address().c_str(), role->get_conf_tx_count());
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    void add_role(xtxpool_role_info_t * role) {
        // xtxpool_dbg("add_role role(%p) table:%s old count:%d:%d:%d:%d inc count:%d:%d:%d:%d",
        //             role,
        //             get_address().c_str(),
        //             role->get_send_tx_count(),
        //             role->get_recv_tx_count(),
        //             role->get_conf_tx_count(),
        //             role->get_unconfirm_tx_count(),
        //             m_counter.get_send_tx_count(),
        //             m_counter.get_recv_tx_count(),
        //             m_counter.get_conf_tx_count(),
        //             m_counter.get_unconfirm_tx_count());
        // role->send_tx_inc(m_counter.get_send_tx_count());
        // role->recv_tx_inc(m_counter.get_recv_tx_count());
        // role->conf_tx_inc(m_counter.get_conf_tx_count());
        // role->unconfirm_tx_inc(m_counter.get_unconfirm_tx_count());
        m_roles.push_back(role);
    }

    void remove_role(xtxpool_role_info_t * role) {
        xtxpool_dbg("remove_role role(%p) table:%s", role, get_address().c_str());
        for (auto it = m_roles.begin(); it != m_roles.end(); it++) {
            if ((*it) == role) {
                xtxpool_dbg("remove_role find and remove role(%p) table:%s", role, get_address().c_str());
                m_roles.erase(it);
                return;
            }
        }
    }

    bool no_role() const {
        return m_roles.empty();
    }

    int32_t get_send_tx_count() const {
        return m_counter.get_send_tx_count();
    }

    int32_t get_recv_tx_count() const {
        return m_counter.get_recv_tx_count();
    }

    int32_t get_conf_tx_count() const {
        return m_counter.get_conf_tx_count();
    }

    const std::set<base::xtable_shortid_t> get_all_table_sids() const {
        if (m_all_table_sids == nullptr) {
            return m_empty;
        }
        return *m_all_table_sids;
    }

    xtable_state_cache_t * get_table_state_cache() {
        return m_table_state_cache;
    }

private:
    std::vector<xtxpool_role_info_t *> m_roles;
    xtx_counter_t m_counter{};
    xtxpool_statistic_t * m_statistic{nullptr};
    xtable_state_cache_t * m_table_state_cache{nullptr};
    std::set<base::xtable_shortid_t> * m_all_table_sids{nullptr};
    std::set<base::xtable_shortid_t> m_empty{};
};

}  // namespace xtxpool_v2
}  // namespace top
