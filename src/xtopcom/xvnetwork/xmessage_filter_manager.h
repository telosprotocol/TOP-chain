// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xmemory.hpp"
#include "xbasic/xns_macro.h"
#include "xbasic/xrunnable.h"
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xmessage_filter_manager_face.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <list>
#include <memory>

NS_BEG2(top, vnetwork)

class xtop_message_filter_manager final : public xmessage_filter_manager_face_t {
private:
    observer_ptr<xvhost_face_t> m_vhost_ptr;

    observer_ptr<election::cache::xdata_accessor_face_t> m_election_data_accessor_ptr;

    std::list<std::unique_ptr<xmessage_filter_base_t>> m_filter_list;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_message_filter_manager);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_message_filter_manager);

    xtop_message_filter_manager(observer_ptr<xvhost_face_t> const &                          vhost_shared_ptr,
                                observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor_ptr);

    void start() override;

    void stop() override;

    observer_ptr<xvhost_face_t> get_vhost_ptr() const override final;

    observer_ptr<election::cache::xdata_accessor_face_t> get_election_data_accessor_ptr() const override final;

    void filt_message(xvnetwork_message_t & msg) const override final;

#if defined DEBUG
    mutable u_int16_t m_filtered_num;
#endif
};
using xmessage_filter_manager_t = xtop_message_filter_manager;

NS_END2