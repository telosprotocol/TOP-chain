// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xmessage_filter.h"
#include "xvnetwork/xvhost_face_fwd.h"
#include "xmetrics/xmetrics.h"
#include <cinttypes>

NS_BEG2(top, vnetwork)

xtop_message_filter_manager::xtop_message_filter_manager(observer_ptr<xvhost_face_t> const &                          vhost_shared_ptr,
                                                         observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor_ptr)
  : m_vhost_ptr{vhost_shared_ptr}, m_election_data_accessor_ptr{election_cache_data_accessor_ptr} {}

void xtop_message_filter_manager::start() {
    m_filter_list.clear();
#define FILTER_LIST_PUSH(filter_name) m_filter_list.push_back(top::make_unique<filter_name>(filter_name{this}))
    FILTER_LIST_PUSH(xmsg_filter_message_empty);
    FILTER_LIST_PUSH(xmsg_filter_wrong_dst);
    FILTER_LIST_PUSH(xmsg_filter_local_time);
    FILTER_LIST_PUSH(xmsg_filter_validator_neighbors_version_mismatch);
    FILTER_LIST_PUSH(xmsg_filter_validator_from_auditor);
    FILTER_LIST_PUSH(xmsg_filter_validator_from_archive);
    FILTER_LIST_PUSH(xmsg_filter_auditor_from_validator);
    FILTER_LIST_PUSH(xmsg_filter_version_still_empty);
#undef FILTER_LIST_PUSH
    return;
}

void xtop_message_filter_manager::stop() {}

observer_ptr<xvhost_face_t> xtop_message_filter_manager::get_vhost_ptr() const {
    return m_vhost_ptr;
}

observer_ptr<election::cache::xdata_accessor_face_t> xtop_message_filter_manager::get_election_data_accessor_ptr() const {
    return m_election_data_accessor_ptr;
}

void xmessage_filter_manager_t::filt_message(xvnetwork_message_t  & msg) const {
    assert(m_filter_list.size() > 0);

#if defined DEBUG
    m_filtered_num = 1;
#endif
#ifdef ENABLE_METRICS
    char msg_info[30] = {0};
    snprintf(msg_info, 29, "%x|%" PRIx64, msg.message().id(), msg.hash());
    XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT("vhost_handle_data_filter", msg_info, uint32_t(10000));
#endif
    for (auto & filter_ptr : m_filter_list) {
        filter_ptr->filt(msg);
        if (msg.empty())
            break;
#if defined DEBUG
        m_filtered_num++;
#endif
    }
}

NS_END2
