// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include <sstream>
#include "xvledger/xreceiptid.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, base)

xreceiptid_pair_t::xreceiptid_pair_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, 1);
}

xreceiptid_pair_t::~xreceiptid_pair_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, -1);
}

xreceiptid_pair_t::xreceiptid_pair_t(uint64_t sendid, uint64_t confirmid, uint64_t recvid) {
    set_sendid_max(sendid);
    set_confirmid_max(confirmid);
    set_recvid_max(recvid);
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, 1);
}

int32_t xreceiptid_pair_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_send_id_max);
    stream.write_compact_var(m_unconfirm_num);
    stream.write_compact_var(m_recv_id_max);
    return (stream.size() - begin_size);
}

int32_t xreceiptid_pair_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_send_id_max);
    stream.read_compact_var(m_unconfirm_num);
    stream.read_compact_var(m_recv_id_max);
    return (begin_size - stream.size());
}

int32_t xreceiptid_pair_t::serialize_to(std::string & bin_data) const {
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    int32_t result = do_write(_stream);
    if(result > 0)
        bin_data.assign((const char*)_stream.data(),_stream.size());
    xassert(result > 0);
    return result;
}

int32_t xreceiptid_pair_t::serialize_from(const std::string & bin_data) {
    base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
    int32_t result = do_read(_stream);
    xassert(result > 0);
    return result;
}

// should set sendid before confirmid
void xreceiptid_pair_t::set_sendid_max(uint64_t value) {
    if (value > m_send_id_max) {
        m_unconfirm_num += value - m_send_id_max;
        m_send_id_max = value;
    }
}
void xreceiptid_pair_t::set_confirmid_max(uint64_t value) {
    xassert(value <= m_send_id_max);
    uint64_t confirmid_max = get_confirmid_max();
    if (value > confirmid_max) {
        uint64_t new_confirmed_num = value - confirmid_max;
        xassert(m_unconfirm_num >= new_confirmed_num);
        m_unconfirm_num -= value - confirmid_max;
    }
}
void xreceiptid_pair_t::set_recvid_max(uint64_t value) {
    if (value > m_recv_id_max) {
        m_recv_id_max = value;
    }
}

std::string xreceiptid_pair_t::dump() const {
    char local_param_buf[64];
    xprintf(local_param_buf,sizeof(local_param_buf),"{sendid=%" PRIu64 ",unconfirm_num=%u,recvid=%" PRIu64 "}",
        m_send_id_max, m_unconfirm_num, m_recv_id_max);
    return std::string(local_param_buf);
}

xreceiptid_pairs_t::xreceiptid_pairs_t() {

}

xreceiptid_pairs_t::~xreceiptid_pairs_t() {

}

bool xreceiptid_pairs_t::find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) const {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        pair = iter->second;
        return true;
    }
    return false;
}

void xreceiptid_pairs_t::add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        xreceiptid_pair_t & old_pair = iter->second;
        old_pair.set_sendid_max(pair.get_sendid_max());
        old_pair.set_recvid_max(pair.get_recvid_max());
        old_pair.set_confirmid_max(pair.get_confirmid_max());
    } else {
        m_all_pairs[sid] = pair;
    }
}

void xreceiptid_pairs_t::clear_binlog() {
    m_all_pairs.clear();
}

void xreceiptid_pairs_t::set_sendid_max(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        iter->second.set_sendid_max(value);
    } else {
        xreceiptid_pair_t pair;
        pair.set_sendid_max(value);
        m_all_pairs[sid] = pair;
    }
}
void xreceiptid_pairs_t::set_confirmid_max(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        iter->second.set_confirmid_max(value);
    } else {
        xreceiptid_pair_t pair;
        pair.set_confirmid_max(value);
        m_all_pairs[sid] = pair;
    }
}
void xreceiptid_pairs_t::set_recvid_max(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        iter->second.set_recvid_max(value);
    } else {
        xreceiptid_pair_t pair;
        pair.set_recvid_max(value);
        m_all_pairs[sid] = pair;
    }
}

std::string xreceiptid_pairs_t::dump() const {
    std::stringstream ss;
    for (auto & v : m_all_pairs) {
        ss << "{" << v.first;
        ss << " " << v.second.get_sendid_max();
        ss << ":" << v.second.get_confirmid_max();
        ss << ":" << v.second.get_recvid_max();
        ss << "}";
    }
    return ss.str();
}


xreceiptid_state_t::xreceiptid_state_t() {
    m_binlog = std::make_shared<xreceiptid_pairs_t>();
    m_modified_binlog = std::make_shared<xreceiptid_pairs_t>();
}

xreceiptid_state_t::xreceiptid_state_t(xtable_shortid_t tableid, uint64_t height) {
    m_binlog = std::make_shared<xreceiptid_pairs_t>();
    m_modified_binlog = std::make_shared<xreceiptid_pairs_t>();
    set_tableid_and_height(tableid, height);
}

void xreceiptid_state_t::add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair) {
    m_binlog->add_pair(sid, pair);
}

bool xreceiptid_state_t::find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) {
    return m_binlog->find_pair(sid, pair);
}

uint32_t xreceiptid_state_t::get_unconfirm_tx_num() const {
    return m_unconfirm_tx_num;
}

void xreceiptid_state_t::update_unconfirm_tx_num() {
    uint32_t unconfirm_tx_num = 0;
    const auto & receiptid_pairs = m_binlog->get_all_pairs();
    for (auto & iter : receiptid_pairs) {
        unconfirm_tx_num += iter.second.get_unconfirm_num();
    }
    m_unconfirm_tx_num = unconfirm_tx_num;
}

void xreceiptid_state_t::set_tableid_and_height(xtable_shortid_t tableid, uint64_t height) {
    m_self_tableid = tableid;
    m_height = height;
}

bool xreceiptid_state_t::find_pair_modified(xtable_shortid_t sid, xreceiptid_pair_t & pair) {
    // firstly find in binlog, secondly find in last full
    bool ret = m_modified_binlog->find_pair(sid, pair);
    if (ret) {
        return ret;
    }
    return find_pair(sid, pair);
}
void xreceiptid_state_t::add_pair_modified(xtable_shortid_t sid, const xreceiptid_pair_t & pair) {
    m_modified_binlog->add_pair(sid, pair);
}

void xreceiptid_state_t::clear_pair_modified() {
    m_modified_binlog->clear_binlog();
}

void    xreceiptid_check_t::set_sendid(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_sendids.find(sid);
    if (iter != m_sendids.end()) {
        std::set<uint64_t> & receiptid_set = iter->second;
        auto ret = receiptid_set.insert(value);
        xassert(ret.second);
    } else {
        std::set<uint64_t> receiptid_set;
        receiptid_set.insert(value);
        m_sendids[sid] = receiptid_set;
    }
}

void    xreceiptid_check_t::set_recvid(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_recvids.find(sid);
    if (iter != m_recvids.end()) {
        std::set<uint64_t> & receiptid_set = iter->second;
        auto ret = receiptid_set.insert(value);
        xassert(ret.second);
    } else {
        std::set<uint64_t> receiptid_set;
        receiptid_set.insert(value);
        m_recvids[sid] = receiptid_set;
    }
}

void    xreceiptid_check_t::set_confirmid(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_confirmids.find(sid);
    if (iter != m_confirmids.end()) {
        std::set<uint64_t> & receiptid_set = iter->second;
        auto ret = receiptid_set.insert(value);
        xassert(ret.second);
    } else {
        std::set<uint64_t> receiptid_set;
        receiptid_set.insert(value);
        m_confirmids[sid] = receiptid_set;
    }
}

uint64_t xreceiptid_check_t::get_sendid_max(xtable_shortid_t sid) {
    auto iter = m_sendids.find(sid);
    if (iter != m_sendids.end()) {
        std::set<uint64_t> & ids = iter->second;
        uint64_t maxid = *ids.rbegin();
        return maxid;
    }
    return 0;
}
uint64_t xreceiptid_check_t::get_recvid_max(xtable_shortid_t sid) {
    auto iter = m_recvids.find(sid);
    if (iter != m_recvids.end()) {
        std::set<uint64_t> & ids = iter->second;
        uint64_t maxid = *ids.rbegin();
        return maxid;
    }
    return 0;
}
uint64_t xreceiptid_check_t::get_confirmid_max(xtable_shortid_t sid) {
    auto iter = m_confirmids.find(sid);
    if (iter != m_confirmids.end()) {
        std::set<uint64_t> & ids = iter->second;
        uint64_t maxid = *ids.rbegin();
        return maxid;
    }
    return 0;
}

bool    xreceiptid_check_t::check_receiptids_contious(const std::set<uint64_t> & ids, uint64_t begin_id) const {
    for (auto & id : ids) {
        if (id != begin_id + 1) {
            return false;
        }
        begin_id++;
    }
    return true;
}

bool    xreceiptid_check_t::check_contious(const xreceiptid_state_ptr_t & receiptid_state) const {
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_imp_check_contious");
    for (auto & v : m_sendids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        uint64_t begin_id = pair.get_sendid_max();
        if (false == check_receiptids_contious(ids, begin_id)) {
            return false;
        }
    }

    for (auto & v : m_recvids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        uint64_t begin_id = pair.get_recvid_max();
        if (false == check_receiptids_contious(ids, begin_id)) {
            return false;
        }
    }

    for (auto & v : m_confirmids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        uint64_t begin_id = pair.get_confirmid_max();
        if (false == check_receiptids_contious(ids, begin_id)) {
            return false;
        }
    }

    return true;
}

void xreceiptid_check_t::update_state(const xreceiptid_state_ptr_t & receiptid_state) const {
    for (auto & v : m_sendids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        pair.set_sendid_max(maxid);
        receiptid_state->add_pair(tableid, pair);
    }

    for (auto & v : m_recvids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        pair.set_recvid_max(maxid);
        receiptid_state->add_pair(tableid, pair);
    }

    for (auto & v : m_confirmids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        pair.set_confirmid_max(maxid);
        receiptid_state->add_pair(tableid, pair);
    }
}

std::string xreceiptid_check_t::dump() const {
    std::stringstream ss;
    ss << "sendid:";
    for (auto & v : m_sendids) {
        ss << "{" << v.first;
        ss << " " << *v.second.begin();
        ss << ":" << *v.second.rbegin();
        xassert(*v.second.rbegin() - *v.second.begin() + 1 == v.second.size());
        ss << "}";
    }
    ss << "recvid:";
    for (auto & v : m_recvids) {
        ss << "{" << v.first;
        ss << " " << *v.second.begin();
        ss << ":" << *v.second.rbegin();
        xassert(*v.second.rbegin() - *v.second.begin() + 1 == v.second.size());
        ss << "}";
    }
    ss << "confirmid:";
    for (auto & v : m_confirmids) {
        ss << "{" << v.first;
        ss << " " << *v.second.begin();
        ss << ":" << *v.second.rbegin();
        xassert(*v.second.rbegin() - *v.second.begin() + 1 == v.second.size());
        ss << "}";
    }
    return ss.str();
}


NS_END2
