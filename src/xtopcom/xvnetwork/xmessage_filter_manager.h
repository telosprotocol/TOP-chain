// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xmessage_filter_manager_face.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <list>

NS_BEG2(top, vnetwork)

class xtop_message_filter_manager final : public xmessage_filter_manager_face_t {
private:
    observer_ptr<xvhost_face_t> m_vhost;
    observer_ptr<election::cache::xdata_accessor_face_t> m_election_data_accessor;

    std::list<std::unique_ptr<xmessage_filter_base_t>> m_filters;

public:
    xtop_message_filter_manager(xtop_message_filter_manager const &) = delete;
    xtop_message_filter_manager(xtop_message_filter_manager &&) = default;
    xtop_message_filter_manager & operator=(xtop_message_filter_manager const &) = delete;
    xtop_message_filter_manager & operator=(xtop_message_filter_manager &&) = delete;
    ~xtop_message_filter_manager() = default;

    xtop_message_filter_manager(observer_ptr<xvhost_face_t> const & vhost,
                                observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor);

    void start() override;
    void stop() override;

    void filter_message(xvnetwork_message_t & message, std::error_code & ec) const override;
};
using xmessage_filter_manager_t = xtop_message_filter_manager;

NS_END2
