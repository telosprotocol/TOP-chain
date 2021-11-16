// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xcons_unorder_cache.h"
#include "xBFT/xconspdu.h"
#include "xmetrics/xmetrics.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xunit_log.h"
NS_BEG2(top, xunit_service)

xcons_unorder_cache::~xcons_unorder_cache() {
    for (auto & v : m_unorder_events) {
        v.second->release_ref();
    }
    m_unorder_events.clear();
}

bool xcons_unorder_cache::filter_event(uint64_t account_viewid, const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet) {
    const uint64_t viewid = packet.get_block_viewid();
    // only process proposal msg now
    if (viewid == account_viewid || packet.get_msg_type() != xconsensus::enum_consensus_msg_type_proposal) {
        return true;
    }

    if (viewid < account_viewid || viewid > account_viewid + unorder_pdu_max_view_num) {
        xunit_warn("xcons_unorder_cache::filter_event drop invalid viewid event.account_viewid=%ld,packet=%s,node_xip=%s.",
                 account_viewid, packet.dump().c_str(), xcons_utl::xip_to_hex(to_addr).c_str());
        return false;
    }

    set_unorder_event(from_addr, to_addr, packet);
    return false;
}

void xcons_unorder_cache::on_view_fire(uint64_t account_viewid) {
    clear_old_unorder_event(account_viewid);
}

void xcons_unorder_cache::set_unorder_event(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet) {
    const uint64_t viewid = packet.get_block_viewid();
    auto iter = m_unorder_events.find(viewid);
    if (iter != m_unorder_events.end()) {
        xunit_warn("xcons_unorder_cache::set_unorder_event erase.old packet=%s,new packet=%s", iter->second->_packet.dump().c_str(), packet.dump().c_str());
        iter->second->release_ref();
        m_unorder_events.erase(iter);
    }

    xconsensus::xcspdu_fire* _event_obj = new xconsensus::xcspdu_fire(packet);
    _event_obj->set_from_xip(from_addr);
    _event_obj->set_to_xip(to_addr);
    m_unorder_events[viewid] = _event_obj;
    xunit_dbg("xcons_unorder_cache::set_unorder_event save.packet=%s", packet.dump().c_str());
}

void xcons_unorder_cache::clear_old_unorder_event(uint64_t account_viewid) {
    for (auto iter = m_unorder_events.begin(); iter != m_unorder_events.end();) {
        if (iter->first < account_viewid) {
            xunit_warn("cons_unorder_cache::clear_old_unorder_event erase old event.account_viewid=%ld,old packet=%s",
                       account_viewid, iter->second->_packet.dump().c_str());
            iter->second->release_ref();
            iter = m_unorder_events.erase(iter);
            continue;
        }
        iter++;
    }
}

xconsensus::xcspdu_fire* xcons_unorder_cache::get_proposal_event(uint64_t account_viewid) {
    for (auto iter = m_unorder_events.begin(); iter != m_unorder_events.end();) {
        if (iter->first == account_viewid && iter->second->_packet.get_msg_type() == xconsensus::enum_consensus_msg_type_proposal) {
            xconsensus::xcspdu_fire* event = iter->second;
            xunit_dbg("xcons_unorder_cache::get_proposal_event find event.account_viewid=%ld", account_viewid);
            iter = m_unorder_events.erase(iter);
            return event;
        }
    }
    return nullptr;
}

NS_END2
