// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xvnetwork_driver_face.h"

#include <mutex>
#include <unordered_map>

NS_BEG2(top, vnetwork)

class xtop_vnetwork_driver_base : public xvnetwork_driver_face_t
{
protected:
    mutable std::mutex m_callbacks_mutex{};
    std::unordered_map<common::xmessage_category_t, xvnetwork_message_ready_callback_t> m_callbacks{};

public:
    xtop_vnetwork_driver_base()                                              = default;
    xtop_vnetwork_driver_base(xtop_vnetwork_driver_base const &)             = delete;
    xtop_vnetwork_driver_base & operator=(xtop_vnetwork_driver_base const &) = delete;
    xtop_vnetwork_driver_base(xtop_vnetwork_driver_base &&)                  = default;
    xtop_vnetwork_driver_base & operator=(xtop_vnetwork_driver_base &&)      = default;
    ~xtop_vnetwork_driver_base() override                                    = default;

    void
    register_message_ready_notify(common::xmessage_category_t message_category,
                                  xvnetwork_message_ready_callback_t cb) final;

    void
    unregister_message_ready_notify(common::xmessage_category_t message_category) final;
};
using xvnetwork_driver_base_t = xtop_vnetwork_driver_base;

NS_END2
