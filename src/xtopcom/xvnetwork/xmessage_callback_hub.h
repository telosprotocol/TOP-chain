// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <list>

#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, vnetwork)

class xtop_message_callback_hub final : public std::enable_shared_from_this<xtop_message_callback_hub>
                                      , public xtrival_runnable_t<xtop_message_callback_hub> {
private:
    observer_ptr<xvhost_face_t> m_vhost{};
    std::list<xmessage_ready_callback_t> m_callbacks;

public:
    xtop_message_callback_hub(xtop_message_callback_hub const &)             = delete;
    xtop_message_callback_hub & operator=(xtop_message_callback_hub const &) = delete;
    xtop_message_callback_hub(xtop_message_callback_hub &&)                  = default;
    xtop_message_callback_hub & operator=(xtop_message_callback_hub &&)      = default;
    ~xtop_message_callback_hub()                                             = default;

    explicit
    xtop_message_callback_hub(observer_ptr<xvhost_face_t> const & vhost);

    void
    register_message_ready_notify(xmessage_ready_callback_t const & cb);

    void
    start() override;

    void
    stop() override;

    common::xnetwork_id_t const &
    network_id() const noexcept;

private:
    void
    on_message(common::xnode_address_t const & sender, xmessage_t const & message, std::uint64_t const hight);
};
using xmessage_callback_hub_t = xtop_message_callback_hub;

NS_END2
