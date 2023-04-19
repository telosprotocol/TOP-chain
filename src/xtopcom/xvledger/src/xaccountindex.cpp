// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xvledger/xaccountindex.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xbasic_size.hpp"

NS_BEG2(top, base)

xaccount_index_t::xaccount_index_t() : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_account_index) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xaccount_index, 1);
}

xaccount_index_t::~xaccount_index_t() {
    statistic_del();
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xaccount_index, -1);
}

xaccount_index_t::xaccount_index_t(const xaccount_index_t& left) : xstatistic::xstatistic_obj_face_t(left){
    m_version            = left.m_version;
    m_latest_tx_nonce    = left.m_latest_tx_nonce;
    m_latest_unit_height = left.m_latest_unit_height;
    m_latest_unit_viewid = left.m_latest_unit_viewid;
    m_account_flag       = left.m_account_flag;
    m_unit_hash          = left.m_unit_hash;
    m_state_hash         = left.m_state_hash;
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xaccount_index, 1);
}

// old version
xaccount_index_t::xaccount_index_t(uint64_t height,
                                   uint64_t viewid,
                                   uint64_t nonce,
                                   enum_xblock_consensus_type _cs_type,
                                   base::enum_xvblock_class _unitclass,
                                   base::enum_xvblock_type _unittype,
                                   bool has_unconfirm_tx,
                                   bool is_account_destroy) : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_account_index) {
    m_latest_tx_nonce    = nonce;
    m_latest_unit_height = height;
    m_latest_unit_viewid = viewid;
    set_latest_unit_class(_unitclass);
    set_latest_unit_type(_unittype);
    set_latest_unit_consensus_type(_cs_type);
    if (has_unconfirm_tx) {
        set_account_index_flag(enum_xaccount_index_flag_has_unconfirm_tx);
    }
    if (is_account_destroy) {
        set_account_index_flag(enum_xaccount_index_flag_account_destroy);
    }
    if (m_latest_tx_nonce > 0) {
        set_account_index_flag(enum_xaccount_index_flag_carry_nonce);
    }
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xaccount_index, 1);
}

// new version
xaccount_index_t::xaccount_index_t(enum_xaccountindex_version_t version, uint64_t height, std::string const & unithash, std::string const & statehash, uint64_t nonce)
  : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_account_index) {
    assert(version == enum_xaccountindex_version_snapshot_hash || version == enum_xaccountindex_version_state_hash);
    assert(!unithash.empty() && !statehash.empty());
    m_version = (uint8_t)version;
    m_latest_tx_nonce    = nonce;
    m_latest_unit_height = height;
    m_latest_unit_viewid = 0;
    m_unit_hash = unithash;
    m_state_hash = statehash;
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xaccount_index, 1);
}

bool xaccount_index_t::operator == (const xaccount_index_t &other) const {
    if (m_latest_unit_height    == other.m_latest_unit_height
        && m_latest_unit_viewid == other.m_latest_unit_viewid
        && m_account_flag       == other.m_account_flag
        && m_latest_tx_nonce    == other.m_latest_tx_nonce
        && m_unit_hash          == other.m_unit_hash
        && m_state_hash         == other.m_state_hash
        && m_version            == other.m_version) {
        return true;
    }
    return false;
}

int32_t xaccount_index_t::old_do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_latest_unit_height);
    stream.write_compact_var(m_latest_unit_viewid);
    stream.write_compact_var(m_account_flag);
    
    if (check_account_index_flag(enum_xaccount_index_flag_carry_nonce)) {
        stream.write_compact_var(m_latest_tx_nonce);
    }
    if (check_account_index_flag(enum_xaccount_index_flag_carry_unit_hash)) {
        xassert(0);
    }

    return (stream.size() - begin_size);
}

int32_t xaccount_index_t::old_do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_latest_unit_height);
    stream.read_compact_var(m_latest_unit_viewid);
    stream.read_compact_var(m_account_flag);
    
    if (check_account_index_flag(enum_xaccount_index_flag_carry_nonce)) {
        stream.read_compact_var(m_latest_tx_nonce);
    }
    if (check_account_index_flag(enum_xaccount_index_flag_carry_unit_hash)) {
        xassert(0);
    }
    
    return (begin_size - stream.size());
}

int32_t xaccount_index_t::old_serialize_to(std::string & bin_data) const {
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    int32_t result = old_do_write(_stream);
    if(result > 0)
        bin_data.assign((const char*)_stream.data(),_stream.size());
    xassert(result > 0);
    return result;
}

int32_t xaccount_index_t::old_serialize_from(const std::string & bin_data) {
    base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
    int32_t result = old_do_read(_stream);
    xassert(result > 0);
    return result;
}

int32_t xaccount_index_t::serialize_to(std::string & bin_data) const {
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    int32_t result = do_write(_stream);
    if(result > 0)
        bin_data.assign((const char*)_stream.data(),_stream.size());
    xassert(result > 0);
    return result;
}

int32_t xaccount_index_t::serialize_from(const std::string & bin_data) {
    base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
    int32_t result = do_read(_stream);
    xassert(result > 0);
    return result;
}

int32_t xaccount_index_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_version);
    stream.write_compact_var(m_latest_unit_height);
    stream.write_compact_var(m_latest_tx_nonce);
    stream.write_compact_var(m_unit_hash);
    stream.write_compact_var(m_state_hash);

    return (stream.size() - begin_size);
}

int32_t xaccount_index_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_version);
    // xassert(m_version == 0);
    // if (m_version != 0){
    //     return 0;
    // }
    stream.read_compact_var(m_latest_unit_height);
    stream.read_compact_var(m_latest_tx_nonce);
    stream.read_compact_var(m_unit_hash);
    stream.read_compact_var(m_state_hash);
    
    return (begin_size - stream.size());
}

void xaccount_index_t::set_tx_nonce(uint64_t txnonce) {
    m_latest_tx_nonce = txnonce;
}

void xaccount_index_t::reset_unit_hash(std::string const& unithash) {
    m_unit_hash = unithash;
}

// [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
void xaccount_index_t::set_latest_unit_class(base::enum_xvblock_class _class) {
    m_account_flag = (m_account_flag & 0x3FFF) | (_class << 13);
}
void xaccount_index_t::set_latest_unit_type(base::enum_xvblock_type _type) {
    m_account_flag = (m_account_flag & 0xE03F) | (_type << 6);
}
void xaccount_index_t::set_account_index_flag(enum_xaccount_index_flag _flag) {
    enum_xaccount_index_flag old_flag = get_account_index_flag();
    enum_xaccount_index_flag new_flag = (enum_xaccount_index_flag)(old_flag | _flag);

    m_account_flag = (m_account_flag & 0xFFC3) | (new_flag << 2);
}
void xaccount_index_t::set_latest_unit_consensus_type(enum_xblock_consensus_type _type) {
    m_account_flag = (m_account_flag & 0xFFFC) | (_type);
}

bool xaccount_index_t::check_account_index_flag(enum_xaccount_index_flag _flag) const {
    enum_xaccount_index_flag copy_flags = get_account_index_flag();
    return ((copy_flags & _flag) != 0);
}

bool xaccount_index_t::is_match_unit(base::xvblock_t* unit) const {
    if ( (m_latest_unit_height == unit->get_height())
        && (m_latest_unit_viewid == unit->get_viewid()) ) {
        return true;
    }
    return false;
}

std::string xaccount_index_t::dump() const {
    char local_param_buf[128];
    xprintf(local_param_buf,sizeof(local_param_buf),"{ver=%d,height=%" PRIu64 ",nonce=%" PRIu64 ",unit=%s,state=%s}",
        m_version, m_latest_unit_height, m_latest_tx_nonce, base::xstring_utl::to_hex(m_unit_hash).c_str(), base::xstring_utl::to_hex(m_state_hash).c_str());
    return std::string(local_param_buf);
}

size_t xaccount_index_t::get_object_size_real() const {
    size_t total_size = sizeof(*this) + get_size(m_unit_hash) + get_size(m_state_hash);
    xdbg("-----cache size----- xaccount_index_t total_size:%zu this:%d,%d:%d", total_size, sizeof(*this), get_size(m_unit_hash), get_size(m_state_hash));
    return total_size;
}

int32_t xaccount_indexs_t::serialize_to_string(std::string & _str) const {
    base::xstream_t _raw_stream(base::xcontext_t::instance());
    int32_t ret = do_write(_raw_stream);
    _str.assign((const char*)_raw_stream.data(),_raw_stream.size());
    return ret;
}

int32_t xaccount_indexs_t::serialize_from_string(const std::string & _str) {
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)_str.data(), (int32_t)_str.size());
    int32_t ret = do_read(_stream);
    return ret;
}

void xaccount_indexs_t::add_account_index(const std::string & addr, const xaccount_index_t & account_index) {
    m_account_indexs.push_back(std::make_pair(addr, account_index));
}

int32_t xaccount_indexs_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    const uint32_t count = (uint32_t)m_account_indexs.size();
    stream.write_compact_var(count);
    for (auto & account_index_pair : m_account_indexs) {
        stream.write_compact_var(account_index_pair.first);
        account_index_pair.second.do_write(stream);
    }
    return (stream.size() - begin_size);
}

int32_t xaccount_indexs_t::do_read(base::xstream_t & stream) {
    m_account_indexs.clear();
    const int32_t begin_size = stream.size();
    uint32_t count = 0;
    stream.read_compact_var(count);
    for (uint32_t i = 0; i < count; i++) {
        std::string addr;
        xaccount_index_t account_index;
        stream.read_compact_var(addr);
        account_index.do_read(stream);
        m_account_indexs.push_back(std::make_pair(addr, account_index));
    }
    return (begin_size - stream.size());
}


NS_END2
