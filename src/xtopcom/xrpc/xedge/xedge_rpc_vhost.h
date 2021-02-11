// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_msg_define.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xvnetwork/xaddress.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xrouter/xrouter_face.h"
#include "xrouter/xrouter.h"
#include "xcommon/xnode_type.h"

NS_BEG2(top, xrpc)
using vnetwork::xvhost_face_t;
using vnetwork::xvnetwork_driver_face_t;
using vnetwork::xmessage_t;
using vnetwork::xvnode_address_t;
using vnetwork::address_cast;
using common::xnode_type_t;
using router::xrouter_face_t;

using xedge_message_callback_t = std::function<void(const xvnode_address_t&, const xrpc_msg_response_t&)>;

class xrpc_edge_vhost
{
public:
    xrpc_edge_vhost(std::shared_ptr<xvnetwork_driver_face_t> edge_host, observer_ptr<xrouter_face_t> router_ptr,
                                observer_ptr<top::base::xiothread_t> thread);
    void on_message(const xvnode_address_t& sender, const xmessage_t& message);
    void register_message_handler(enum_xrpc_type msg_type, xedge_message_callback_t cb);
    std::shared_ptr<xvnetwork_driver_face_t> get_vnetwork_driver() const noexcept ;
    observer_ptr<xrouter_face_t> get_router() const noexcept { return m_router_ptr; }
    void reset_edge_vhost(shared_ptr<xrpc_edge_vhost> edge_vhost);
private:
    std::shared_ptr<xvnetwork_driver_face_t>                m_vnetwork_driver;
    observer_ptr<xrouter_face_t>                            m_router_ptr;
    unordered_map<enum_xrpc_type, xedge_message_callback_t> m_edge_handler_map;
    mutable std::mutex                                      m_ventwork_driver_mutex;
    observer_ptr<top::base::xiothread_t>                    m_thread;
};

NS_END2
