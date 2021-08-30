// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xvledger/xaccountindex.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"

#ifndef __DISABLE_NONCE_OF_INDEX__
    #include "xdata/xunit_bstate.h"
#endif

NS_BEG2(top, base)

xaccount_index_t::xaccount_index_t() {
    XMETRICS_GAUGE(metrics::dataobject_xaccount_index, 1);
}

xaccount_index_t::~xaccount_index_t() {
    XMETRICS_GAUGE(metrics::dataobject_xaccount_index, -1);
}

xaccount_index_t::xaccount_index_t(const xaccount_index_t& left) {
    m_latest_unit_height = left.m_latest_unit_height;
    m_latest_unit_viewid = left.m_latest_unit_viewid;
    m_account_flag = left.m_account_flag;
    m_latest_nonce = left.m_latest_nonce;
    XMETRICS_GAUGE(metrics::dataobject_xaccount_index, 1);
}

xaccount_index_t::xaccount_index_t(base::xvblock_t* unit,
                                    bool has_unconfirm_tx,
                                    enum_xblock_consensus_type _cs_type,
                                    bool is_account_destroy,
                                    uint64_t latest_send_nonce) {
    m_latest_unit_height = unit->get_height();
    m_latest_unit_viewid = unit->get_viewid();
    set_latest_unit_class(unit->get_block_class());
    set_latest_unit_type(unit->get_block_type());
    set_latest_unit_consensus_type(_cs_type);
    if (has_unconfirm_tx) {
        set_account_index_flag(enum_xaccount_index_flag_has_unconfirm_tx);
    }
    if (is_account_destroy) {
        set_account_index_flag(enum_xaccount_index_flag_account_destroy);
    }
    
    m_latest_nonce = latest_send_nonce;
    // //XTODO, set m_latest_nonce
    // {
    //     #ifndef __DISABLE_NONCE_OF_INDEX__
    //     base::xauto_ptr<base::xvbstate_t> block_bstate =
    //     base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(unit, metrics::statestore_access_from_txpool_get_accountstate);
    //     if (block_bstate == nullptr)
    //     {
    //         xwarn("xaccount_index_t::xaccount_index_t fail-get unitstate. block=%s", unit->dump().c_str());
    //     }
    //     else
    //     {
    //         data::xaccount_ptr_t account_state = std::make_shared<data::xunit_bstate_t>(block_bstate.get());
    //         const uint64_t new_nonce = account_state->get_latest_send_trans_number();
    //         if(new_nonce > m_latest_nonce)
    //         {
    //             xinfo("xaccount_index_t::xaccount_index_t,unitstate. block=%s and m_latest_nonce=%llu,new_nonce=%llu", unit->dump().c_str(),m_latest_nonce,new_nonce);
    //             m_latest_nonce = new_nonce;
    //         }
    //         else
    //         {
    //             xwarn("xaccount_index_t::xaccount_index_t,unitstate. block=%s and m_latest_nonce=%llu,new_nonce=%llu", unit->dump().c_str(),m_latest_nonce,new_nonce);
    //         }
    //     }
    //     #endif
    // }
    
    XMETRICS_GAUGE(metrics::dataobject_xaccount_index, 1);
}

int32_t xaccount_index_t::do_write(base::xstream_t & stream) const {
    const int32_t begin_size = stream.size();
    stream.write_compact_var(m_latest_unit_height);
    stream.write_compact_var(m_latest_unit_viewid);
    stream.write_compact_var(m_account_flag);
    stream.write_compact_var(m_latest_nonce);
    return (stream.size() - begin_size);
}

int32_t xaccount_index_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    stream.read_compact_var(m_latest_unit_height);
    stream.read_compact_var(m_latest_unit_viewid);
    stream.read_compact_var(m_account_flag);
    stream.read_compact_var(m_latest_nonce);
    return (begin_size - stream.size());
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
    xprintf(local_param_buf,sizeof(local_param_buf),"{height=%" PRIu64 ",viewid=%" PRIu64 ",flag=0x%x}",
        m_latest_unit_height, m_latest_unit_viewid, m_account_flag);
    return std::string(local_param_buf);
}

NS_END2
