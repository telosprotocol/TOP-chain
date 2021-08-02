// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xmessage_filter_manager_face.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <list>
#include <memory>

NS_BEG3(top, tests, vnetwork)

class xtop_dummy_message_filter_manager : public top::vnetwork::xmessage_filter_manager_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_message_filter_manager);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_message_filter_manager);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_message_filter_manager);

    void start() override {}

    void stop() override {}

    void filter_message(top::vnetwork::xvnetwork_message_t &, std::error_code &) const override {}
};
using xdummy_message_filter_manager_t = xtop_dummy_message_filter_manager;

NS_END3
