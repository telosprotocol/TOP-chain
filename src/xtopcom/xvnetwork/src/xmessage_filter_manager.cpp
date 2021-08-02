// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xmessage_filter.h"
#include "xmetrics/xmetrics.h"

#include <cinttypes>

NS_BEG2(top, vnetwork)

xtop_message_filter_manager::xtop_message_filter_manager(observer_ptr<xvhost_face_t> const & vhost,
                                                         observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
    : m_vhost{ vhost }, m_election_data_accessor{ election_cache_data_accessor } {}

void xtop_message_filter_manager::start() {
    assert(m_filters.empty());

#define ADD_FILTER(filter_name) m_filters.push_back(top::make_unique<filter_name>(filter_name{ m_vhost, m_election_data_accessor }))
    ADD_FILTER(xtop_message_filter_sender);
    ADD_FILTER(xtop_message_filter_recver);
    ADD_FILTER(xtop_message_filter_message_id);
    ADD_FILTER(xtop_message_filter_recver_is_auditor);
    ADD_FILTER(xtop_message_filter_recver_is_validator);
    ADD_FILTER(xtop_message_filter_recver_is_rec);
    ADD_FILTER(xtop_message_filter_recver_is_zec);
#undef ADD_FILTER

    return;
}

void xtop_message_filter_manager::stop() {
    m_filters.clear();
}

//observer_ptr<xvhost_face_t> xtop_message_filter_manager::get_vhost_ptr() const {
//    return m_vhost_ptr;
//}
//
//observer_ptr<election::cache::xdata_accessor_face_t> xtop_message_filter_manager::get_election_data_accessor_ptr() const {
//    return m_election_data_accessor_ptr;
//}

//void xmessage_filter_manager_t::filt_message(xvnetwork_message_t  & msg) const {
//    assert(m_filter_list.size() > 0);
//
//#if defined DEBUG
//    m_filtered_num = 1;
//#endif
//#ifdef ENABLE_METRICS
//    char msg_info[30] = {0};
//    snprintf(msg_info, 29, "%x|%" PRIx64, msg.message().id(), msg.hash());
//    XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT("vhost_handle_data_filter", msg_info, uint32_t(10000));
//#endif
//    std::error_code ec;
//    for (auto & filter_ptr : m_filter_list) {
//        auto const continue_process = filter_ptr->filter(msg, ec);
//        if (!continue_process) {
//            break;
//        }
//#if defined DEBUG
//        m_filtered_num++;
//#endif
//    }
//}

void xmessage_filter_manager_t::filter_message(xvnetwork_message_t & message, std::error_code & ec) const {
    assert(!m_filters.empty());

    for (auto const & filter : m_filters) {
        if (xfilter_result_t::stop_filtering == filter->filter(message, ec)) {
            break;
        }
    }
}

NS_END2
