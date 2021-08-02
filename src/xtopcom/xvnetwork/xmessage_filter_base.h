// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xvnetwork_message.h"

NS_BEG2(top, vnetwork)

enum class xenum_filter_result : uint8_t {
    continue_filtering,
    stop_filtering,
};
using xfilter_result_t = xenum_filter_result;

class xtop_message_filter_base {
public:
    xtop_message_filter_base(xtop_message_filter_base const &) = delete;
    xtop_message_filter_base(xtop_message_filter_base &&) = default;
    xtop_message_filter_base & operator=(xtop_message_filter_base const &) = delete;
    xtop_message_filter_base & operator=(xtop_message_filter_base &&) = delete;
    virtual ~xtop_message_filter_base() = default;

    virtual xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const = 0;

protected:
    xtop_message_filter_base() = default;

    void normalize_message_recver(xvnetwork_message_t & vnetwork_message,
                                  observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                  observer_ptr<election::cache::xdata_accessor_face_t> const & data_accessor,
                                  std::error_code & ec) const;

    void normalize_message_recver(xvnetwork_message_t & vnetwork_message,
                                  observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                  std::shared_ptr<election::cache::xgroup_element_t> const & recver_group,
                                  std::error_code & ec) const;

    void normalize_message_recver_by_message_sender(xvnetwork_message_t & vnetwork_message,
                                                    observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                    observer_ptr<election::cache::xdata_accessor_face_t> const & data_accessor,
                                                    std::error_code & ec) const;
};
using xmessage_filter_base_t = xtop_message_filter_base;

NS_END2
