// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"

namespace top {
namespace xtxpool_v2 {

#define table_send_tx_queue_size_max (256)
#define table_recv_tx_queue_size_max (1024)
#define table_conf_tx_queue_size_max (256)

#define shard_send_tx_queue_size_max (4096)
#define shard_recv_tx_queue_size_max (16384)
#define shard_conf_tx_queue_size_max (4096)

class xtx_counter_t {
public:
    void send_tx_inc(int32_t count) {
        xassert(m_send_tx_count >= 0);
        m_send_tx_count += count;
    }
    void recv_tx_inc(int32_t count) {
        xassert(m_recv_tx_count >= 0);
        m_recv_tx_count += count;
    }
    void conf_tx_inc(int32_t count) {
        xassert(m_conf_tx_count >= 0);
        m_conf_tx_count += count;
    }
    uint32_t get_send_tx_count() const {
        return m_send_tx_count;
    }
    uint32_t get_recv_tx_count() const {
        return m_recv_tx_count;
    }
    uint32_t get_conf_tx_count() const {
        return m_conf_tx_count;
    }

private:
    std::atomic<uint32_t> m_send_tx_count{0};
    std::atomic<uint32_t> m_recv_tx_count{0};
    std::atomic<uint32_t> m_conf_tx_count{0};
};

class xtxpool_shard_info_t : public xtx_counter_t {
public:
    xtxpool_shard_info_t(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) : m_zone(zone), m_front_table_id(front_table_id), m_back_table_id(back_table_id) {
    }
    bool is_ids_match(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) const {
        return (m_zone == zone && m_front_table_id == front_table_id && m_back_table_id == back_table_id);
    }
    void get_shard_ids(uint8_t & zone, uint16_t & front_table_id, uint16_t & back_table_id) {
        m_zone = zone;
        m_front_table_id = front_table_id;
        m_back_table_id = back_table_id;
    }
    void subscribe() {
        m_sub_count++;
    }
    void unsubscribe() {
        xassert(m_sub_count > 0);
        m_sub_count--;
    }
    uint8_t get_sub_count() const {
        return m_sub_count;
    }

private:
    uint8_t m_sub_count{0};
    uint8_t m_zone;
    uint16_t m_front_table_id;
    uint16_t m_back_table_id;
};

class xtxpool_table_info_t {
public:
    xtxpool_table_info_t(const std::string & address, xtxpool_shard_info_t * shard) : m_address(address), m_shard(shard) {
    }
    const std::string & get_table_addr() {
        return m_address;
    }
    void send_tx_inc(int32_t count) {
        m_counter.send_tx_inc(count);
        m_shard->send_tx_inc(count);
    }
    void recv_tx_inc(int32_t count) {
        m_counter.recv_tx_inc(count);
        m_shard->recv_tx_inc(count);
    }
    void conf_tx_inc(int32_t count) {
        m_counter.conf_tx_inc(count);
        m_shard->recv_tx_inc(count);
    }

    bool is_send_tx_reached_upper_limit() {
        if (m_counter.get_send_tx_count() >= table_send_tx_queue_size_max || m_counter.get_conf_tx_count() >= table_conf_tx_queue_size_max) {
            return true;
        }
        if (m_shard->get_send_tx_count() >= shard_send_tx_queue_size_max) {
            return true;
        }
        return false;
    }
    bool is_recv_tx_reached_upper_limit() {
        if (m_counter.get_recv_tx_count() >= table_recv_tx_queue_size_max) {
            return true;
        }
        if (m_shard->get_recv_tx_count() >= shard_recv_tx_queue_size_max) {
            return true;
        }
        return false;
    }

private:
    std::string m_address;
    xtxpool_shard_info_t * m_shard;
    xtx_counter_t m_counter;
};

}  // namespace xtxpool_v2
}  // namespace top
