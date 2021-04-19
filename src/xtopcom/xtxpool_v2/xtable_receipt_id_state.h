// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"

#include <string>

NS_BEG2(top, xtxpool_v2)

class xpeer_receipt_id_state_t {
public:
    xpeer_receipt_id_state_t(const xpeer_receipt_id_state_t & peer_receipt_id_state) {
        m_max_out_id = peer_receipt_id_state.get_max_out_id();
        m_latest_commit_out_id = peer_receipt_id_state.get_latest_commit_out_id();
        m_latest_commit_in_id = peer_receipt_id_state.get_latest_commit_in_id();
    }
    xpeer_receipt_id_state_t(uint64_t max_out_id, uint64_t latest_commit_out_id, uint64_t latest_commit_in_id)
      : m_max_out_id(max_out_id), m_latest_commit_out_id(latest_commit_out_id), m_latest_commit_in_id(latest_commit_in_id) {
    }
    void set_max_out_id(uint64_t max_out_id) {
        xassert(m_max_out_id <= max_out_id);
        xassert(m_latest_commit_out_id <= max_out_id);
        m_max_out_id = max_out_id;
    }
    void set_latest_commit_out_id(uint64_t latest_commit_out_id) {
        xassert(m_max_out_id >= latest_commit_out_id);
        xassert(m_latest_commit_out_id <= latest_commit_out_id);
        m_latest_commit_out_id = latest_commit_out_id;
    }
    void set_latest_commit_in_id(uint64_t latest_commit_in_id) {
        m_latest_commit_in_id = latest_commit_in_id;
    }
    uint64_t get_max_out_id() const {
        return m_max_out_id;
    }
    uint64_t get_latest_commit_out_id() const {
        return m_latest_commit_out_id;
    }
    uint64_t get_latest_commit_in_id() const {
        return m_latest_commit_in_id;
    }
    bool all_confirmed() const {
        return (m_max_out_id == m_latest_commit_out_id);
    }

private:
    uint64_t m_max_out_id{0};
    uint64_t m_latest_commit_out_id{0};
    uint64_t m_latest_commit_in_id{0};
};

class xtable_receipt_id_state_t {
public:
    void set_max_out_id(uint16_t peer_sid, uint64_t max_out_id) {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            it->second->set_max_out_id(max_out_id);
        } else {
            auto peer_state = std::make_shared<xpeer_receipt_id_state_t>(max_out_id, 0, 0);
            m_peer_receipt_id_state_map[peer_sid] = peer_state;
        }
    }
    void set_latest_commit_out_id(uint16_t peer_sid, uint64_t latest_commit_out_id) {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            it->second->set_latest_commit_out_id(latest_commit_out_id);
        } else {
            auto peer_state = std::make_shared<xpeer_receipt_id_state_t>(0, latest_commit_out_id, 0);
            m_peer_receipt_id_state_map[peer_sid] = peer_state;
        }
    }
    void set_latest_commit_in_id(uint16_t peer_sid, uint64_t latest_commit_in_id) {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            it->second->set_latest_commit_in_id(latest_commit_in_id);
        } else {
            auto peer_state = std::make_shared<xpeer_receipt_id_state_t>(0, 0, latest_commit_in_id);
            m_peer_receipt_id_state_map[peer_sid] = peer_state;
        }
    }
    uint64_t get_max_out_id(uint16_t peer_sid) const {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            return it->second->get_max_out_id();
        } else {
            return 0;
        }
    }
    uint64_t get_latest_commit_out_id(uint16_t peer_sid) const {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            return it->second->get_latest_commit_out_id();
        } else {
            return 0;
        }
    }
    uint64_t get_latest_commit_in_id(uint16_t peer_sid) const {
        auto it = m_peer_receipt_id_state_map.find(peer_sid);
        if (it != m_peer_receipt_id_state_map.end()) {
            return it->second->get_latest_commit_in_id();
        } else {
            return 0;
        }
    }

    const std::map<uint16_t, std::shared_ptr<xpeer_receipt_id_state_t>> & get_map() const {
        return m_peer_receipt_id_state_map;
    };

private:
    std::map<uint16_t, std::shared_ptr<xpeer_receipt_id_state_t>> m_peer_receipt_id_state_map;
};

static uint16_t get_table_sid_by_addr(std::string account_addr) {
    auto xid = base::xvaccount_t::get_xid_from_account(account_addr);
    uint16_t zone = get_vledger_zone_index(xid);
    uint16_t subaddr = get_vledger_subaddr(xid);
    uint16_t table_sid = (zone << 12) | subaddr;
    return table_sid;
}

NS_END2
