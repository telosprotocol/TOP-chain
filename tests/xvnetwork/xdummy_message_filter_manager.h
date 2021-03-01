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

NS_BEG3(top, tests, vnetwork)

class xtop_dummy_message_filter_manager : public top::vnetwork::xmessage_filter_manager_face_t {
private:
    observer_ptr<top::vnetwork::xvhost_face_t> m_vhost_ptr;

    observer_ptr<top::election::cache::xdata_accessor_face_t> m_election_data_accessor_ptr;

public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_message_filter_manager);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_message_filter_manager);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_message_filter_manager);

    void start() override {}

    void stop() override {}

    observer_ptr<top::vnetwork::xvhost_face_t> get_vhost_ptr() const override { return m_vhost_ptr; }

    observer_ptr<top::election::cache::xdata_accessor_face_t> get_election_data_accessor_ptr() const override { return m_election_data_accessor_ptr; }

    void filt_message(top::vnetwork::xvnetwork_message_t & msg) const override {}
};
using xdummy_message_filter_manager_t = xtop_dummy_message_filter_manager;

NS_END3
