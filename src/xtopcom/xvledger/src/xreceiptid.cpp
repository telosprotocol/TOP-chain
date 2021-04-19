// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xvledger/xreceiptid.h"

NS_BEG2(top, base)

REG_CLS(xreceiptid_pairs_t);

xreceiptid_pair_t::xreceiptid_pair_t(uint64_t sendid_max, uint32_t unconfirm_num, uint64_t recvid_max)
: m_send_id_max(sendid_max), m_unconfirm_num(unconfirm_num), m_recv_id_max(recvid_max) {
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
    m_all_pairs[sid] = pair;
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

xreceiptid_state_t::xreceiptid_state_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_last_full = make_object_ptr<xreceiptid_pairs_t>();
    m_binlog = make_object_ptr<xreceiptid_pairs_t>();
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

NS_END2
