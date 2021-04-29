// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include <sstream>
#include "xvledger/xreceiptid.h"

NS_BEG2(top, base)

REG_CLS(xreceiptid_pairs_t);

xreceiptid_pair_t::xreceiptid_pair_t(uint64_t sendid, uint64_t confirmid, uint64_t recvid) {
    set_sendid_max(sendid);
    set_confirmid_max(confirmid);
    set_recvid_max(recvid);
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

bool xreceiptid_pairs_t::find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) {
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
void xreceiptid_pairs_t::add_pairs(const std::map<xtable_shortid_t, xreceiptid_pair_t> & pairs) {
    for (auto & v : pairs) {
        add_pair(v.first, v.second);
    }
}
void xreceiptid_pairs_t::add_binlog(const xobject_ptr_t<xreceiptid_pairs_t> & binlog) {
    const std::map<xtable_shortid_t, xreceiptid_pair_t> & all_pairs = binlog->get_all_pairs();
    for (auto & v : all_pairs) {
        add_pair(v.first, v.second);
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

int32_t xreceiptid_pairs_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    xassert(m_all_pairs.size() < 65535);
    uint16_t count = (uint16_t)m_all_pairs.size();
    stream.write_compact_var(count);
    for (auto & v : m_all_pairs) {
        stream.write_compact_var(v.first);
        v.second.do_write(stream);
    }
    return (stream.size() - begin_size);
}

int32_t xreceiptid_pairs_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    uint16_t count = 0;
    stream.read_compact_var(count);
    for (uint16_t i = 0; i < count; i++) {
        xtable_shortid_t sid;
        stream.read_compact_var(sid);
        xreceiptid_pair_t pair;
        pair.do_read(stream);
        m_all_pairs[sid] = pair;
    }
    return (begin_size - stream.size());
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


xreceiptid_state_t::xreceiptid_state_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_last_full = make_object_ptr<xreceiptid_pairs_t>();
    m_binlog = make_object_ptr<xreceiptid_pairs_t>();
    m_modified_binlog = make_object_ptr<xreceiptid_pairs_t>();
}

xreceiptid_state_t::xreceiptid_state_t(const xreceiptid_pairs_ptr_t & last_full, const xreceiptid_pairs_ptr_t & binlog)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    // last full or binlog maybe null
    m_last_full = last_full;
    if (m_last_full == nullptr) {
        m_last_full = make_object_ptr<xreceiptid_pairs_t>();
    }
    m_binlog = binlog;
    if (m_binlog == nullptr) {
        m_binlog = make_object_ptr<xreceiptid_pairs_t>();
    }
    m_modified_binlog = make_object_ptr<xreceiptid_pairs_t>();
}

int32_t xreceiptid_state_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    xassert(m_last_full != nullptr);
    m_last_full->serialize_to(stream);
    xassert(m_binlog != nullptr);
    m_binlog->serialize_to(stream);
    return (stream.size() - begin_size);
}

int32_t xreceiptid_state_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    m_last_full = make_object_ptr<xreceiptid_pairs_t>();
    m_last_full->serialize_from(stream);
    m_binlog = make_object_ptr<xreceiptid_pairs_t>();
    m_binlog->serialize_from(stream);
    return (begin_size - stream.size());
}

void xreceiptid_state_t::merge_new_full() {
    if (m_binlog->get_size() != 0) {
        m_last_full->add_binlog(m_binlog);
        m_binlog->clear_binlog();
    }
}

std::string xreceiptid_state_t::build_root_hash(enum_xhash_type hashtype) {
    xassert(m_binlog->get_all_pairs().empty());  // must do merge first
    std::string bin_str;
    serialize_to_string(bin_str);
    return xcontext_t::instance().hash(bin_str, hashtype);
}

void xreceiptid_state_t::add_pair(xtable_shortid_t sid, const xreceiptid_pair_t & pair) {
    m_binlog->add_pair(sid, pair);
}

void xreceiptid_state_t::add_pairs(const std::map<xtable_shortid_t, xreceiptid_pair_t> & pairs) {
    m_binlog->add_pairs(pairs);
}

bool xreceiptid_state_t::find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) {
    // firstly find in binlog, secondly find in last full
    bool ret = m_binlog->find_pair(sid, pair);
    if (ret) {
        return ret;
    }
    return m_last_full->find_pair(sid, pair);
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
