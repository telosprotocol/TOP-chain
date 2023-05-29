// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xmessage_category.h"
#include "xvnetwork/xvhost_face.h"

#include <mutex>
#include <unordered_map>

NS_BEG2(top, vnetwork)


/**
 * @brief xtop_vhost_base implements non-business functionality.
 */
class xtop_vhost_base : public xvhost_face_t
{
protected:
    mutable std::mutex m_callbacks_mutex{};
    std::unordered_map<xvnode_address_t, xmessage_ready_callback_t> m_callbacks{};

public:
    xtop_vhost_base()                                    = default;
    xtop_vhost_base(xtop_vhost_base const &)             = delete;
    xtop_vhost_base & operator=(xtop_vhost_base const &) = delete;
    xtop_vhost_base(xtop_vhost_base &&)                  = delete;
    xtop_vhost_base & operator=(xtop_vhost_base &&)      = delete;
    ~xtop_vhost_base() override                          = default;

    void
    register_message_ready_notify(xvnode_address_t const & vadd,
                                  xmessage_ready_callback_t cb) override final;

    void
    unregister_message_ready_notify(xvnode_address_t const & vaddr) override final;
};
using xvhost_base_t = xtop_vhost_base;

NS_END2
