// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <inttypes.h>
#include "xedge_rpc_vhost.h"
#include "xmetrics/xmetrics.h"
#include "xrpc/xrpc_init.h"

NS_BEG2(top, xrpc)


xrpc_edge_vhost::xrpc_edge_vhost(std::shared_ptr<xvnetwork_driver_face_t> edge_host,
                                 observer_ptr<xrouter_face_t> router_ptr,
                                 observer_ptr<top::base::xiothread_t> thread)
    : m_vnetwork_driver(edge_host)
    , m_router_ptr(router_ptr)
    , m_thread(thread)
{
    m_vnetwork_driver->register_message_ready_notify(xmessage_category_rpc, std::bind(&xrpc_edge_vhost::on_message, this, _1, _2));
    xinfo("edge register rpc");
}

void xrpc_edge_vhost::on_message(const xvnode_address_t& sender, const xmessage_t& message)
{
    auto msgid = message.id();
    xinfo_rpc("xedge_rpc_handler on_message,id(%x), %" PRIx64, msgid, message.hash());    //TODO address to_string

    auto process_request = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        rpc_message_para_t * para = dynamic_cast<rpc_message_para_t *>(call.get_param1().get_object());
        auto message = para->m_message;
        auto sender = para->m_sender;

        xrpc_msg_response_t msg = codec::xmsgpack_codec_t<xrpc_msg_response_t>::decode(message.payload());
        auto iter = this->m_edge_handler_map.find(msg.m_type);
        // assert(iter != this->m_edge_handler_map.end());
        if (iter != this->m_edge_handler_map.end()) {
            iter->second(sender, msg);
        }
        return true;
    };

    base::xauto_ptr<rpc_message_para_t> para = new rpc_message_para_t(sender, message);
    base::xcall_t asyn_call(process_request, para.get());
    m_thread->send_call(asyn_call);
}

void xrpc_edge_vhost::register_message_handler(enum_xrpc_type msg_type, xedge_message_callback_t cb)
{
    m_edge_handler_map.emplace(msg_type, cb);
}

std::shared_ptr<xvnetwork_driver_face_t> xrpc_edge_vhost::get_vnetwork_driver() const noexcept
{
    std::lock_guard<std::mutex> lock(m_ventwork_driver_mutex);
    return m_vnetwork_driver;
}

void xrpc_edge_vhost::reset_edge_vhost(shared_ptr<xrpc_edge_vhost> edge_vhost)
{
    xdbg("rpc reset edge_vhost");
    edge_vhost->get_vnetwork_driver()->unregister_message_ready_notify(xmessage_category_rpc);
    {
        std::lock_guard<std::mutex> lock(m_ventwork_driver_mutex);
        m_vnetwork_driver = edge_vhost->get_vnetwork_driver();
    }
    m_vnetwork_driver->register_message_ready_notify(xmessage_category_rpc, std::bind(&xrpc_edge_vhost::on_message, this, _1, _2));
}

NS_END2
