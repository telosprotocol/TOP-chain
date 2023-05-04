// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include <sstream>
#include "xvledger/xreceiptid.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, base)

xreceiptid_pair_t::xreceiptid_pair_t() : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_receiptid_pair) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, 1);
}

xreceiptid_pair_t::~xreceiptid_pair_t() {
    statistic_del();
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, -1);
}

xreceiptid_pair_t::xreceiptid_pair_t(uint64_t sendid, uint64_t confirmid, uint64_t recvid, uint64_t send_rsp_id, uint64_t confirm_rsp_id)
    : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_receiptid_pair)  {
    set_sendid_max(sendid);
    set_confirmid_max(confirmid);
    set_recvid_max(recvid);
    set_send_rsp_id_max(send_rsp_id);
    set_confirm_rsp_id_max(confirm_rsp_id);
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xreceiptid_pair_t, 1);
}

int32_t xreceiptid_pair_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_send_id_max);
    stream.write_compact_var(m_unconfirm_num);
    stream.write_compact_var(m_recv_id_max);
    if (m_send_rsp_id_max != 0) {
        stream.write_compact_var(m_send_rsp_id_max);
        stream.write_compact_var(m_unconfirm_rsp_num);
    }
    return (stream.size() - begin_size);
}

int32_t xreceiptid_pair_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_send_id_max);
    stream.read_compact_var(m_unconfirm_num);
    stream.read_compact_var(m_recv_id_max);
    if (stream.size() > 0) {
        stream.read_compact_var(m_send_rsp_id_max);
        stream.read_compact_var(m_unconfirm_rsp_num);
    }
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

// should set send_rsp_id before confirm_rsp_id
void xreceiptid_pair_t::set_send_rsp_id_max(uint64_t value) {
    if (value > m_send_rsp_id_max) {
        m_unconfirm_rsp_num += value - m_send_rsp_id_max;
        m_send_rsp_id_max = value;
    }
}
void xreceiptid_pair_t::set_confirm_rsp_id_max(uint64_t value) {
    xassert(value <= m_send_rsp_id_max);
    uint64_t confirm_rsp_id_max = get_confirm_rsp_id_max();
    if (value > confirm_rsp_id_max) {
        uint64_t new_confirmed_num = value - confirm_rsp_id_max;
        xassert(m_unconfirm_rsp_num >= new_confirmed_num);
        m_unconfirm_rsp_num -= value - confirm_rsp_id_max;
    }
}

std::string xreceiptid_pair_t::dump() const {
    char local_param_buf[64];
    xprintf(local_param_buf,sizeof(local_param_buf),"{sid=%" PRIu64 ",unconfirm=%u,rid=%" PRIu64 ",rspid=%" PRIu64 ",unrspnum=%u}",
        m_send_id_max, m_unconfirm_num, m_recv_id_max, m_send_rsp_id_max, m_unconfirm_rsp_num);
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
        old_pair.set_send_rsp_id_max(pair.get_send_rsp_id_max());
        old_pair.set_confirm_rsp_id_max(pair.get_confirm_rsp_id_max());
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

void xreceiptid_pairs_t::set_send_rsp_id_max(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        iter->second.set_send_rsp_id_max(value);
    } else {
        xreceiptid_pair_t pair;
        pair.set_send_rsp_id_max(value);
        m_all_pairs[sid] = pair;
    }
}
void xreceiptid_pairs_t::set_confirm_rsp_id_max(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_all_pairs.find(sid);
    if (iter != m_all_pairs.end()) {
        iter->second.set_confirm_rsp_id_max(value);
    } else {
        xreceiptid_pair_t pair;
        pair.set_confirm_rsp_id_max(value);
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
        ss << " " << v.second.get_send_rsp_id_max();
        ss << ":" << v.second.get_confirm_rsp_id_max();
        // ss << ":" << v.second.get_recv_rsp_id_max();
        ss << "}";
    }
    return ss.str();
}

int32_t xreceiptid_pairs_t::get_object_size_real() const {
    // each node of std::map<xtable_shortid_t, xreceiptid_pair_t> alloc 80B
    int32_t total_size = sizeof(*this) + m_all_pairs.size()*80;
    xdbg("-----cache size----- xreceiptid_pairs_t total_size:%d this:%d,m_all_pairs:%d * 80", total_size, sizeof(*this), m_all_pairs.size());
    return total_size;
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

bool xreceiptid_state_t::find_pair(xtable_shortid_t sid, xreceiptid_pair_t & pair) const {
    return m_binlog->find_pair(sid, pair);
}

uint32_t xreceiptid_state_t::get_unconfirm_tx_num() const {
    return m_unconfirm_tx_num;
}

void xreceiptid_state_t::update_unconfirm_tx_num() {
    uint32_t unconfirm_tx_num = 0;
    uint32_t unconfirm_rsp_tx_num = 0;
    const auto & receiptid_pairs = m_binlog->get_all_pairs();
    for (auto & iter : receiptid_pairs) {
        unconfirm_tx_num += iter.second.get_unconfirm_num();
        unconfirm_rsp_tx_num += iter.second.get_unconfirm_rsp_num();
    }
    m_unconfirm_tx_num = unconfirm_tx_num;
    m_unconfirm_rsp_tx_num = unconfirm_rsp_tx_num;
}

uint32_t xreceiptid_state_t::get_unconfirm_rsp_tx_num() const {
    return m_unconfirm_rsp_tx_num;
}

void xreceiptid_state_t::set_tableid_and_height(xtable_shortid_t tableid, uint64_t height) {
    m_self_tableid = tableid;
    m_height = height;
}

bool xreceiptid_state_t::find_pair_modified(xtable_shortid_t sid, xreceiptid_pair_t & pair) const {
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

int32_t xreceiptid_state_t::get_object_size_real() const {
    int32_t binlog_size = 0;
    int32_t modified_binlog_size = 0;
    if (m_binlog != nullptr) {
        binlog_size = m_binlog->get_object_size_real();
    }
    if (m_modified_binlog != nullptr) {
        modified_binlog_size = m_modified_binlog->get_object_size_real();
    }

    int32_t total_size = sizeof(*this) + binlog_size + modified_binlog_size;
    xdbg("------cache size------ xreceiptid_state_t total_size:%d this:%d,binlog_size:%d,modified_binlog_size:%d", total_size, sizeof(*this), binlog_size, modified_binlog_size);
    return total_size;
}

void xids_check_t::set_id(xtable_shortid_t sid, uint64_t value) {
    auto iter = m_ids.find(sid);
    if (iter != m_ids.end()) {
        std::set<uint64_t> & id_set = iter->second;
        auto ret = id_set.insert(value);
        xassert(ret.second);
    } else {
        std::set<uint64_t> id_set;
        id_set.insert(value);
        m_ids[sid] = id_set;
    }
}

uint32_t xids_check_t::size() const {
    return m_ids.size();
}

bool xids_check_t::check_continuous(const xreceiptid_state_ptr_t & receiptid_state) const {
    for (auto & v : m_ids) {
        xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        xreceiptid_pair_t pair;
        receiptid_state->find_pair(tableid, pair);
        uint64_t begin_id = get_begin_id(pair);
        if (false == check_receiptids_contious(ids, begin_id)) {
            return false;
        }
    }
    return true;
}

void xids_check_t::get_modified_pairs(const base::xreceiptid_state_ptr_t & receiptid_state, xreceiptid_pairs_ptr_t & modified_pairs) const {
    for (auto & v : m_ids) {
        base::xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        if (false == modified_pairs->find_pair(tableid, pair)) {  // find modified pairs firstly
            receiptid_state->find_pair(tableid, pair);
        }
        set_receiptid_max(pair, maxid);
        modified_pairs->add_pair(tableid, pair);  // save to modified pairs
    }
}

bool xids_check_t::check_receiptids_contious(const std::set<uint64_t> & ids, uint64_t begin_id) {
    for (auto & id : ids) {
        if (id != begin_id + 1) {
            return false;
        }
        begin_id++;
    }
    return true;
}

// void xids_check_t::dump(std::stringstream & ss) const {
//     for (auto & v : m_ids) {
//         ss << "{" << v.first;
//         ss << " " << *v.second.begin();
//         ss << ":" << *v.second.rbegin();
//         xassert(*v.second.rbegin() - *v.second.begin() + 1 == v.second.size());
//         ss << "}";
//     }
// }

uint64_t xsendids_check_t::get_begin_id(xreceiptid_pair_t & pair) const {
    return pair.get_sendid_max();
}

void xsendids_check_t::set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const {
    pair.set_sendid_max(receiptid);
}

uint64_t xrecvids_check_t::get_begin_id(xreceiptid_pair_t & pair) const {
    return pair.get_recvid_max();
}

void xrecvids_check_t::set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const {
    pair.set_recvid_max(receiptid);
}

uint64_t xconfirmids_check_t::get_begin_id(xreceiptid_pair_t & pair) const {
    return pair.get_confirmid_max();
}

void xconfirmids_check_t::set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const {
    pair.set_confirmid_max(receiptid);
}

uint64_t xsend_rsp_ids_check_t::get_begin_id(xreceiptid_pair_t & pair) const {
    return pair.get_send_rsp_id_max();
}

void xsend_rsp_ids_check_t::set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const {
    pair.set_send_rsp_id_max(receiptid);
}

uint64_t xconfirm_rsp_ids_check_t::get_begin_id(xreceiptid_pair_t & pair) const {
    return pair.get_confirm_rsp_id_max();
}

void xconfirm_rsp_ids_check_t::set_receiptid_max(xreceiptid_pair_t & pair, uint64_t receiptid) const {
    pair.set_confirm_rsp_id_max(receiptid);
}

void    xreceiptid_check_t::set_sendid(xtable_shortid_t sid, uint64_t value) {
    m_sendids.set_id(sid, value);
}

void    xreceiptid_check_t::set_recvid(xtable_shortid_t sid, uint64_t value) {
    m_recvids.set_id(sid, value);
}

void    xreceiptid_check_t::set_confirmid(xtable_shortid_t sid, uint64_t value) {
    m_confirmids.set_id(sid, value);
}


void    xreceiptid_check_t::set_send_rsp_id(xtable_shortid_t sid, uint64_t value) {
    m_send_rsp_ids.set_id(sid, value);
}

void    xreceiptid_check_t::set_confirm_rsp_id(xtable_shortid_t sid, uint64_t value) {
    m_confirm_rsp_ids.set_id(sid, value);
}

bool    xreceiptid_check_t::check_continuous(const xreceiptid_state_ptr_t & receiptid_state) const {
    bool ret = m_sendids.check_continuous(receiptid_state);
    if (!ret) {
        return false;
    }
    ret = m_recvids.check_continuous(receiptid_state);
    if (!ret) {
        return false;
    }
    ret = m_send_rsp_ids.check_continuous(receiptid_state);
    if (!ret) {
        return false;
    }
    ret = m_confirm_rsp_ids.check_continuous(receiptid_state);
    if (!ret) {
        return false;
    }
    return true;
}

xreceiptid_pairs_ptr_t xreceiptid_check_t::get_modified_pairs(const base::xreceiptid_state_ptr_t & receiptid_state) const {
    xreceiptid_pairs_ptr_t modified_pairs = std::make_shared<base::xreceiptid_pairs_t>();
    m_sendids.get_modified_pairs(receiptid_state, modified_pairs);
    m_recvids.get_modified_pairs(receiptid_state, modified_pairs);
    m_confirmids.get_modified_pairs(receiptid_state, modified_pairs);
    m_send_rsp_ids.get_modified_pairs(receiptid_state, modified_pairs);
    m_confirm_rsp_ids.get_modified_pairs(receiptid_state, modified_pairs);
    return modified_pairs;
}

// std::string xreceiptid_check_t::dump() const {
//     std::stringstream ss;
//     ss << "sendid:";
//     m_sendids.dump(ss);
//     ss << "recvid:";
//     m_recvids.dump(ss);
//     ss << "confirmid:";
//     m_confirmids.dump(ss);
//     ss << "sendrpsid:";
//     m_send_rsp_ids.dump(ss);
//     ss << "confirmrspid:";
//     m_confirm_rsp_ids.dump(ss);
//     return ss.str();
// }


void xreceiptid_all_table_states::add_table_receiptid_state(xtable_shortid_t sid, const base::xreceiptid_state_ptr_t & receiptid_state) {
    m_all_states[sid] = receiptid_state;
}

bool  xreceiptid_all_table_states::get_two_table_pair(xtable_shortid_t table1, xtable_shortid_t table2, xreceiptid_pair_t & pair, uint64_t & height) const {
    auto table1_iter = m_all_states.find(table1);
    if (table1_iter == m_all_states.end()) {
        return false;
    }
    table1_iter->second->find_pair(table2, pair);
    height = table1_iter->second->get_block_height();
    return true;
}

std::vector<xreceiptid_unfinish_info_t> xreceiptid_all_table_states::get_unfinish_info() const {
    std::vector<xreceiptid_unfinish_info_t> infos;
    for (auto & table1 : m_all_states) {
        for (auto & table2 : m_all_states) {
            xreceiptid_pair_t table1_table2_pair;
            uint64_t table1_height = 0;
            get_two_table_pair(table1.first, table2.first, table1_table2_pair, table1_height);

            xreceiptid_pair_t table2_table1_pair;
            uint64_t table2_height = 0;
            get_two_table_pair(table2.first, table1.first, table2_table1_pair, table2_height);
            
            xreceiptid_unfinish_info_t info;
            if (table1_table2_pair.get_sendid_max() > table2_table1_pair.get_recvid_max() || table1_table2_pair.get_unconfirm_rsp_num() > 0) {
                info.source_height = table1_height;
                info.target_height = table2_height;
                info.source_id = table1.first;
                info.target_id = table2.first;
                info.unrecv_num = table1_table2_pair.get_sendid_max() - table2_table1_pair.get_recvid_max();
                info.unconfirm_num = table1_table2_pair.get_unconfirm_rsp_num();
                infos.push_back(info);
            }
        }
    }
    return infos;
}

NS_END2
