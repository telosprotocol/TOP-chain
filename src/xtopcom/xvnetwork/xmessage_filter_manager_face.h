// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file xmessage_filter_manager_face.h
 * @brief interface definitions for xmessage_filter_manager class
 */

#pragma once

#include "xbasic/xrunnable.h"
#include "xvnetwork/xvnetwork_message.h"

NS_BEG2(top, vnetwork)

class xtop_message_filter_manager_face : public xbasic_runnable_t<xtop_message_filter_manager_face> {
public:
    xtop_message_filter_manager_face(xtop_message_filter_manager_face const &) = delete;
    xtop_message_filter_manager_face(xtop_message_filter_manager_face &&) = default;
    xtop_message_filter_manager_face & operator=(xtop_message_filter_manager_face const &) = delete;
    xtop_message_filter_manager_face & operator=(xtop_message_filter_manager_face &&) = delete;
    ~xtop_message_filter_manager_face() override = default;

    virtual void filter_message(xvnetwork_message_t & message, std::error_code & ec) const = 0;

protected:
    xtop_message_filter_manager_face() = default;
};
using xmessage_filter_manager_face_t = xtop_message_filter_manager_face;

NS_END2
