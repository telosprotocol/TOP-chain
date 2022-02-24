// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xmbus/xmessage_bus.h"
#include "xvnode/xvnode_sniff_proxy_face.h"

#define INVALID_EVENT_ID (-1)

NS_BEG2(top, vnode)

class xtop_vnode_sniff_proxy final : public xvnode_sniff_proxy_face_t {
public:
    xtop_vnode_sniff_proxy(observer_ptr<mbus::xmessage_bus_face_t> const & bus);

    void start() override;
    void stop() override;
    void reg(common::xnode_address_t const & address, components::sniffing::xsniffer_config_t const & config) override;
    void unreg(common::xnode_address_t const & address) override;
    void sniff(mbus::xevent_ptr_t const & e);

private:
    observer_ptr<mbus::xmessage_bus_face_t> mbus{nullptr};
    int m_store_event_id{INVALID_EVENT_ID};
    int m_timer_event_id{INVALID_EVENT_ID};
    std::map<common::xnode_address_t, components::sniffing::xsniffer_config_t> m_sniff_config;
};

using xvnode_sniff_proxy_t = xtop_vnode_sniff_proxy;

NS_END2
