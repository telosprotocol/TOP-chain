// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/udp_transport/xudp_socket.h"

#include <stdio.h>
#include <mutex>
#include <iostream>
#include <string>

#include "xbase/xcontext.h"
#include "xbase/xatom.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#include "xtransport/utils/transport_utils.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xmetrics/xmetrics.h"
#include "xtransport/udp_transport/transport_filter.h"
#include "json/json.h"
#include "xcrypto/xckey.h"
#include "xutility/xhash.h"

using namespace top;
using namespace top::base;

namespace top {
namespace transport {
const static uint16_t XUDP_RATELIMIT_INTERVAL = 5;  //  5 minutes
const static uint16_t XUDP_RATELIMIT_CONNECTION_TIMES = 10;  //  not more than 5 connection times
const static uint16_t XUDP_RATELIMIT_CONNECTION_TIMES_WITHIN_MINUTE = 3;  //  not more than 3 connection times within one minute
const static std::string XUDP_VERSION = "{\"msg_encode_type\": 1}";

const static std::string SUB_NETWORK_MASK = "255.255.255.0";
const static uint32_t IP_SEG_MAX_ADDR = 5;  // same ip_seg max addr number

/*
add_ref,release_ref: use xp2pudp_t, including UdpProperty and xudp_client_map
add_linkrefcount, release_linkrefcount: relationship with routing_table, including UdpProperty
*/
UdpProperty::~UdpProperty()
{
    if(m_xudp_ != nullptr)
    {
        TOP_DBG_INFO("~UdpProperty:%p, xudp:%p, get_refcount:%d.call release_ref", this, m_xudp_, m_xudp_->get_refcount());
        m_xudp_->release_linkrefcount();
        m_xudp_->release_ref();
    }
}

void UdpProperty::SetXudp(xp2pudp_t* xudp_in)
{
    xp2pudp_t * old = base::xatomic_t::xexchange(m_xudp_,xudp_in);
    if (xudp_in != nullptr) {
        TOP_DBG_INFO("set xudp:%p, get_refcount:%d.this:%p. call add_ref", xudp_in, xudp_in->get_refcount(), this);
        xudp_in->add_ref();
        xudp_in->add_linkrefcount();
    }

    if(old != nullptr)
    {
        TOP_DBG_INFO("set xudp:%p, get_refcount:%d.this:%p. call release_ref.", old, old->get_refcount(), this);
        old->release_linkrefcount();
        xcontext_t::instance().delay_release_object(old);
//        old->release_ref();
    }
}

int xp2pudp_t::add_linkrefcount()
{
    TOP_DBG_INFO("add_linkrefcount:xudp:%p, m_link_refcount:%d, get_refcount:%d", this, m_link_refcount.load(), get_refcount());
    return ++m_link_refcount;
}
int xp2pudp_t::release_linkrefcount()
{
    TOP_DBG_INFO("release_linkrefcount,xudp:%p, m_link_refcount:%d, get_refcount:%d", this, m_link_refcount.load(), get_refcount());
    return --m_link_refcount;
    // const int new_count = --m_link_refcount;
    // if(new_count <= 0)
    // {
    //     close(true); //truely close it
    //     if(listen_server_ != nullptr)
    //     {
    //         listen_server_->CloseXudp(this);
    //     } else {
    //         TOP_INFO("listen_server_ is null,xudp:%p", this);
    //     }
    // }
    // TOP_DBG_INFO("release_linkrefcount,xudp:%p, m_link_refcount:%d, get_refcount:%d", this, m_link_refcount.load(), get_refcount());
    // if (m_link_refcount < 0)
    //     m_link_refcount = 0;
    // return new_count;
}

//notify the child endpont is ready to use when receive on_endpoint_open of error_code = enum_xcode_successful
bool xp2pudp_t::on_endpoint_open(
    const int32_t error_code,
    const int32_t cur_thread_id,
    const uint64_t timenow_ms,
    xendpoint_t* from_child)
{
    const int result = xudp_t::on_endpoint_open(error_code,cur_thread_id,timenow_ms,from_child);
    TOP_DBG_INFO("xp2pudp_t connected successful,handle(%d),local address[%s : %d]-[%d : %d] vs\
        peer address[%s : %d]-[%d : %d]; cur_thread_id=%d,object_id(%ld) for this(%p),get_refcount:%d,status:%d",
        get_handle(), get_local_ip_address().c_str(),get_local_real_port(),get_local_logic_port(),get_local_logic_port_token(),
        get_peer_ip_address().c_str(),get_peer_real_port(),get_peer_logic_port(),get_peer_logic_port_token(),
        get_thread_id(),get_obj_id(),this, get_refcount(), m_status);
    if (m_status != top::transport::enum_xudp_status::enum_xudp_closed && m_status != top::transport::enum_xudp_status::enum_xudp_closed_released)
        m_status = top::transport::enum_xudp_status::enum_xudp_connected;
    else
        return result;

    if (listen_server_ != nullptr) {
        listen_server_->AddXudp(xslsocket_t::get_peer_ip_address() +":" + std::to_string(xslsocket_t::get_peer_real_port()), this);
    } else {
        TOP_INFO("listen_server_ is null");
    }
    return result;
}
//when associated io-object close happen,post the event to receiver
//error_code is 0 when it is closed by caller/upper layer
bool xp2pudp_t::on_endpoint_close(
    const int32_t error_code,
    const int32_t cur_thread_id,
    const uint64_t timenow_ms,
    xendpoint_t* from_child)
{
    TOP_DBG_INFO("xp2pudp_t close successful,cur_thread_id=%d,object_id(%d) for this(%p),\
        local address[%s : %d]-[%d : %d] vs  peer address[%s : %d]-[%d : %d],get_refcount:%d",
        cur_thread_id,get_obj_id(),this,
        get_local_ip_address().c_str(),get_local_real_port(),get_local_logic_port(),get_local_logic_port_token(),
        get_peer_ip_address().c_str(),get_peer_real_port(),get_peer_logic_port(),get_peer_logic_port_token(),get_refcount());
    m_status = top::transport::enum_xudp_status::enum_xudp_closed;
    
    if(listen_server_ != nullptr)
    {
        listen_server_->CloseXudp(this);
    }


    return xudp_t::on_endpoint_close(error_code,cur_thread_id,timenow_ms,from_child);
}

//notify upper layer,child endpoint update keeplive status
bool xp2pudp_t::on_endpoint_keepalive(
    const std::string & _payload,
    const int32_t cur_thread_id,
    const uint64_t timenow_ms,
    xendpoint_t* from_child)
{
    //note: _payload is from send_keepalive_packet();
//        printf(" xp2pudp_t<--keepalive,handle(%d),local[%s : %d]-[%d : %d] vs peer[%s : %d]-[%d : %d];
// thread_id=%d,object_id(%lld)\n",get_handle(), get_local_ip_address().c_str(),get_local_real_port(),get_local_logic_port(),get_local_logic_port_token(),
// get_peer_ip_address().c_str(),get_peer_real_port(),get_peer_logic_port(),get_peer_logic_port_token(),get_thread_id(),get_obj_id());
    return xudp_t::on_endpoint_keepalive(_payload,cur_thread_id,timenow_ms,from_child);
}
int32_t xp2pudp_t::connect_xudp(const std::string& target_ip,const uint16_t target_port, XudpSocket* udp_server){
    listen_server_ = udp_server;
    if (m_status != top::transport::enum_xudp_status::enum_xudp_closed && m_status != top::transport::enum_xudp_status::enum_xudp_closed_released)
        m_status = top::transport::enum_xudp_status::enum_xudp_connecting;
    else 
        return 0;
    TOP_DBG_INFO("connect,this:%p,ip:%s,port:%d,listen_ptr:%p, get_refcount:%d", this, target_ip.c_str(), target_port, udp_server, get_refcount());
    return xudp_t::connect(target_ip, target_port, const_default_connection_timeout , const_default_connection_timeout * 3, 5000);//send keepalive every 5 seconds,and socket is timeout if 45 seconds not receive any packet
}
int xp2pudp_t::send(xpacket_t& packet)
{
    XMETRICS_GAUGE(metrics::message_transport_send, 1);
    if (packet.get_size() > 512) {
        packet.set_process_flag(enum_xpacket_process_flag_compress); //ask compres
    }
    int ret_status = xslsocket_t::send(0, 0, 0, 0, packet, 0, 0, NULL);

#ifdef XENABLE_P2P_BENDWIDTH
    transport::protobuf::RoutingMessage message;
    if (!message.ParseFromArray((const char*)packet.get_body().data() + enum_xbase_header_len,packet.get_body().size() - enum_xbase_header_len))
    {
        TOP_ERROR("Message ParseFromString from string failed!");
        return enum_xcode_successful;
    }

    if (message.has_broadcast() && message.broadcast()) {
        XMETRICS_FLOW_COUNT("p2p_transport_broadcast_packet_send", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_broadcast_bandwidth_send", packet.get_size());
    }

    // total
    XMETRICS_FLOW_COUNT("p2p_transport_packet_send", 1);
    XMETRICS_FLOW_COUNT("p2p_transport_bandwidth_send", packet.get_size());
    TransportFilter::Instance()->AddTrafficData(true, message.type(), packet.get_size());
#endif

    return ret_status;

}

int32_t xp2pudp_t::recv(
    uint64_t from_xip_addr_low,
    uint64_t from_xip_addr_high,
    uint64_t to_xip_addr_low,
    uint64_t to_xip_addr_high,
    xpacket_t & packet,
    int32_t cur_thread_id,
    uint64_t timenow_ms,
    xendpoint_t* from_child_end) {
#ifdef ENABLE_XSECURITY
    if (TransportFilter::Instance()->BlackIpCheck(packet.get_from_ip_addr())) {
        TOP_WARN("on_recv deny ddos black_list ip:%s", packet.get_from_ip_addr().c_str());
        return enum_xerror_code_fail;
    }
    TransportFilter::Instance()->AddTrafficData(packet.get_size(), packet.get_from_ip_addr());
#endif

    if (packet.get_size() <= (int)enum_xbase_header_len) {
        TOP_ERROR("packet.size(%d) invalid", (int)packet.get_size());
        return 0;
    }

    /*
    TOP_DEBUG("Recv size: %d, local address[%s : %d]-[%d : %d],peer address[%s : %d]-[%d : %d]", packet.get_body().size(),
             get_local_ip_address().c_str(),get_local_real_port(),get_local_logic_port(),get_local_logic_port_token(),
             get_peer_ip_address().c_str(),get_peer_real_port(),get_peer_logic_port(),get_peer_logic_port_token());
             */
    
    if (listen_server_ == nullptr) {
        TOP_INFO("listen_server_ is null");
        return enum_xcode_successful;
    }

#ifdef XENABLE_P2P_BENDWIDTH
    transport::protobuf::RoutingMessage message;
    if (!message.ParseFromArray((const char*)packet.get_body().data() + enum_xbase_header_len,packet.get_body().size() - enum_xbase_header_len))
    {
        TOP_ERROR("Message ParseFromString from string failed!");
        return enum_xcode_successful;
    }

    if (message.has_broadcast() && message.broadcast()) {
        XMETRICS_FLOW_COUNT("p2p_transport_broadcast_packet_recv", 1);
        XMETRICS_FLOW_COUNT("p2p_transport_broadcast_bandwidth_recv", packet.get_size());
    }

    // total
    XMETRICS_FLOW_COUNT("p2p_transport_packet_recv", 1);
    XMETRICS_FLOW_COUNT("p2p_transport_bandwidth_recv", packet.get_size());
    TransportFilter::Instance()->AddTrafficData(false, message.type(), packet.get_size());
#endif

    listen_server_->multi_thread_message_handler_->HandleMessage(packet);
    //go default handle,where may deliver packet to parent object
    return xsocket_t::recv(
        from_xip_addr_low,
        from_xip_addr_high,
        to_xip_addr_low,
        to_xip_addr_high,
        packet,
        cur_thread_id,
        timenow_ms,
        from_child_end);
}

XudpSocket::XudpSocket(
        base::xcontext_t & _context,
        int32_t target_thread_id,
        xfd_handle_t native_handle,
        MultiThreadHandler* message_handler)
        : xudplisten_t(_context, NULL, target_thread_id, native_handle),
          multi_thread_message_handler_(message_handler)
    {
        //note: target_thread_id is used for io only(read and write packet from/to system),but m_xudpsocket_mgr_thread_id used to manage xudp_t objects(include keepalive/timer,lifemanage)
        m_xudpsocket_mgr_thread_id = 0;
        xiothread_t * dedicated_thread_ptr = _context.find_thread(base::xiothread_t::enum_xthread_type_manage, true);//try to shared thread
        if(NULL == dedicated_thread_ptr)//on-demand create
            dedicated_thread_ptr = base::xiothread_t::create_thread(_context, base::xiothread_t::enum_xthread_type_manage, -1);
        m_xudpsocket_mgr_thread_id = dedicated_thread_ptr->get_thread_id();
        TOP_INFO("using mgr thread(%d) to manage all xudp_t objects",m_xudpsocket_mgr_thread_id);
    }

XudpSocket::~XudpSocket()
{
    Stop();
    //dont stop/close xudpsocket_mgr_thread that could be shared by other XudpSocket objects
}
xslsocket_t* XudpSocket::create_xslsocket(
    xendpoint_t * parent,
    xfd_handle_t handle,
    xsocket_property & property,
    int32_t cur_thread_id,
    uint64_t timenow_ms) {
//    property._peer_account_id = top::global_node_id;
    xp2pudp_t* udp_ptr = new xp2pudp_t(*get_context(),parent,m_xudpsocket_mgr_thread_id,handle,property, this);
    TOP_DBG_INFO("create_xslsocket:%p,get_refcount:%d at thread(%d)", udp_ptr, udp_ptr->get_refcount(),m_xudpsocket_mgr_thread_id);
    return udp_ptr;
}
xslsocket_t* XudpSocket::on_xslsocket_accept(xfd_handle_t handle,xsocket_property & property, int32_t cur_thread_id,uint64_t timenow_ms)
{
    // xudp connection version && protocol
    if (property._peer_payload != XUDP_VERSION) {
        TOP_INFO("on_xslsocket_accept,account:%s,sign_size:%s,payload:%s.authentication fail.",
                 property._peer_account_id.c_str(),
                 HexEncode(property._peer_signature).c_str(),
                 property._peer_payload.c_str());
        return nullptr;
    }
    if (m_register_node_callback != nullptr) {
#ifndef XENABLE_MOCK_ZEC_STAKE
        if (m_register_node_callback(property._peer_account_id, property._peer_signature) != 0) {
            // register_callback failed.
            TOP_INFO("on_xslsocket_accept,account:%s,sign_size:%s,payload:%s.authentication fail.",
                     property._peer_account_id.c_str(),
                     HexEncode(property._peer_signature).c_str(),
                     property._peer_payload.c_str());
            return nullptr;
        }
#endif
    }
    // else{}
    // here ` if m_register_node_callback == nullptr ` means the genesis time. thus no else.

    TOP_INFO("on_xslsocket_accept,account:%s,sign_size:%s,payload:%s.authentication succ.",
             property._peer_account_id.c_str(),
             HexEncode(property._peer_signature).c_str(),
             property._peer_payload.c_str());

#ifdef ENABLE_XSECURITY
    if (!AddToConlimitMap(property._peer_ip_addr)) {
        TOP_WARN("on_xslsocket_accept deny connection for same ip_seg ip:%s", property._peer_ip_addr.c_str());
        return nullptr;
    }

    if (TransportFilter::Instance()->BlackIpCheck(property._peer_ip_addr)) {
        TOP_WARN("on_xslsocket_accept deny ddos black_list ip:%s", property._peer_ip_addr.c_str());
        return nullptr;
    }
#endif
    return xudplisten_t::on_xslsocket_accept(handle, property, cur_thread_id, timenow_ms);
}

void XudpSocket::register_on_receive_callback(on_receive_callback_t callback) {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    assert(callback_ == nullptr);
    TOP_DBG_INFO("register callback for XudpSocket");
    callback_ = callback;
}

void XudpSocket::unregister_on_receive_callback() {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    callback_ = nullptr;
    TOP_DBG_INFO("unregister callback for XudpSocket");
}

void XudpSocket::Stop() {
    std::unique_lock<std::recursive_mutex> lock(xudp_mutex_);
    TOP_DBG_INFO("xudp Stop");
    std::map<std::string, xp2pudp_t*>::iterator it;
    for (it= xudp_client_map.begin(); it != xudp_client_map.end(); ++it) {
        xp2pudp_t* peer_xudp_socket = it->second;
        if(peer_xudp_socket != nullptr)
        {
            peer_xudp_socket->close();
            peer_xudp_socket->release_ref();
        }
    }
    xudp_client_map.clear();
    XMETRICS_COUNTER_SET("xtransport_xudp_num", 0);
}

int XudpSocket::SendToLocal(base::xpacket_t& packet) {
    if (packet.get_size() <= (int)enum_xbase_header_len) {
        TOP_ERROR("<blueshi> packet.size(%d) invalid", (int)packet.get_size());
        return kTransportFailed;
    }
    TOP_DBG_INFO("SendToLocal:%d", packet.get_size());
    multi_thread_message_handler_->HandleMessage(packet);
    return kTransportSuccess;
}

int XudpSocket::SendDataWithProp(std::string const & data,const std::string & peer_ip,uint16_t peer_port,UdpPropertyPtr& udp_property,uint16_t priority_flag){
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0, 0, false);
    packet.get_body().push_back((uint8_t *)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(peer_ip);
    packet.set_to_ip_port(peer_port);
    AddXip2Header(packet,priority_flag);
    return SendDataWithProp(packet, udp_property);
}

int XudpSocket::SendData(base::xpacket_t& packet) {
    UdpPropertyPtr udp_property;
    return SendDataWithProp(packet, udp_property);
}

int XudpSocket::SendDataWithProp(
        base::xpacket_t& packet,
        UdpPropertyPtr& udp_property) {
    // TOP_FATAL("SendData(from(%s:%d), to(%s:%d), size=%d)", packet.get_from_ip_addr().c_str(), packet.get_from_ip_port(),
    //     packet.get_to_ip_addr().c_str(), packet.get_to_ip_port(), (int)packet.get_size());

//  printf("ip:%s,%s,%s,port:%d,%d,%d\n", packet.get_to_ip_addr().c_str(),packet.get_from_ip_addr().c_str(), xsocket_t::get_local_ip_address().c_str(), packet.get_to_ip_port(),packet.get_from_ip_port(), xsocket_t::get_local_real_port());
    if (GetLocalIp() == packet.get_to_ip_addr() && packet.get_to_ip_port()== GetLocalPort()) {
//      printf("send to local addr.\n");
        packet.set_from_ip_addr(packet.get_to_ip_addr());
        packet.set_from_ip_port(packet.get_to_ip_port());
        SendToLocal(packet);
        TOP_DEBUG("sendto local");
        return kTransportFailed;
    }

    if (udp_property != nullptr) {
        xp2pudp_t* prop_xudp_ptr = udp_property->GetXudp();
        if(prop_xudp_ptr != nullptr) {
            if (prop_xudp_ptr->GetStatus() != top::transport::enum_xudp_status::enum_xudp_closed &&
                prop_xudp_ptr->GetStatus() != top::transport::enum_xudp_status::enum_xudp_closed_released &&
                (false == prop_xudp_ptr->is_close())) {
                if (prop_xudp_ptr->send(packet) != enum_xcode_successful) {
                    TOP_WARN("send xpacket failed!packet size is :%d\n",packet.get_size());
                    return kTransportFailed;
                }
                
                TOP_DEBUG("send packet using node_ptr,to:%s:%d, size:%d, xudp:%p, prop:%p",
                    packet.get_to_ip_addr().c_str(), (int)packet.get_to_ip_port(),
                    packet.get_size(), prop_xudp_ptr, udp_property.get());
                    
                return kTransportSuccess;
            } else {
                udp_property->SetXudp(nullptr);
                // need not delete session map here, udp_property's xudp may not equal to map data.
            }
        }
    }

    if (packet.get_to_ip_port() == 0) {
        TOP_WARN("port is zero.");
        transport::protobuf::RoutingMessage pro_message;
        if (!pro_message.ParseFromArray((const char*)packet.get_body().data() + enum_xbase_header_len, packet.get_body().size() - enum_xbase_header_len))
        {
            TOP_ERROR("Message ParseFromString from string failed!");
            return kTransportFailed;
        }
        TOP_INFO("SendData error:%d,%s,%d", pro_message.type(), pro_message.des_node_id().c_str(), pro_message.id());
        return kTransportFailed;
    }

// add connect for xudp
    std::map<std::string, xp2pudp_t*>::iterator iter;
    int ret;
    std::unique_lock<std::recursive_mutex> autolock(xudp_mutex_);
    std::string to_addr(packet.get_to_ip_addr() +":" + std::to_string(packet.get_to_ip_port()));
    iter = xudp_client_map.find(to_addr);
    xp2pudp_t * peer_xudp_socket = nullptr;
    if (iter == xudp_client_map.end()) {
        if ((ret = CheckRatelimitMap(to_addr)) != enum_xcode_successful) {
            TOP_ERROR("reach xudp connection rate limit, drop this packet:%s,ret:%d", to_addr.c_str(), ret);
            return kTransportSuccess;
        }
        TOP_DEBUG("not find:%s, size:%d", to_addr.c_str(), packet.get_body().size());
//        peer_xudp_socket = (xp2pudp_t*)xudplisten_t::create_xslsocket(enum_socket_type_xudp);
        std::string node_sign;
        if (GetSign(node_sign) != enum_xcode_successful) {
            TOP_ERROR("get sign failed.");
            return kTransportSuccess;
        }
        TOP_DEBUG("xudp first connect %s:%u", packet.get_to_ip_addr().c_str(), packet.get_to_ip_port());
        peer_xudp_socket = (xp2pudp_t*)xudplisten_t::create_xslsocket(global_node_id, node_sign, XUDP_VERSION, enum_socket_type_xudp);
        peer_xudp_socket->connect_xudp(packet.get_to_ip_addr(), packet.get_to_ip_port(), this);

        xudp_client_map[to_addr] = peer_xudp_socket;
        XMETRICS_COUNTER_INCREMENT("xtransport_xudp_num", 1);
        TOP_DEBUG("conn %s:%p", to_addr.c_str(), peer_xudp_socket);
        AddToRatelimitMap(to_addr);
    } else {
        peer_xudp_socket = xudp_client_map[to_addr];
        TOP_DEBUG("find:%s, size:%d", to_addr.c_str(), packet.get_body().size());

        if ( peer_xudp_socket->is_close() || (peer_xudp_socket->GetStatus() == top::transport::enum_xudp_status::enum_xudp_closed) ||
            (peer_xudp_socket->GetStatus() == top::transport::enum_xudp_status::enum_xudp_closed_released)) {
            TOP_INFO("wait reconnect... release_ref:%p, get_refcount:%d", peer_xudp_socket, peer_xudp_socket->get_refcount());

            if (m_offline_cb != nullptr) {
                m_offline_cb(peer_xudp_socket->get_peer_ip_address(), peer_xudp_socket->get_peer_real_port());
            }
            peer_xudp_socket->SetStatus(top::transport::enum_xudp_status::enum_xudp_closed_released);
            peer_xudp_socket->close(true);
            peer_xudp_socket->release_ref();
            xudp_client_map.erase(iter);
            XMETRICS_COUNTER_INCREMENT("xtransport_xudp_num", -1);

            if ((ret = CheckRatelimitMap(to_addr)) != enum_xcode_successful) {
                TOP_ERROR("reach xudp connection rate limit2, drop this packet:%s, ret:%d", to_addr.c_str(), ret);
                return kTransportSuccess;
            }
            std::string node_sign;
            if (GetSign(node_sign) != enum_xcode_successful) {
                TOP_ERROR("get sign failed.");
                return kTransportSuccess;
            }
            TOP_DEBUG("xudp reconnect %s:%u", packet.get_to_ip_addr().c_str(), packet.get_to_ip_port());
            peer_xudp_socket = (xp2pudp_t*)xudplisten_t::create_xslsocket(global_node_id, node_sign, XUDP_VERSION, enum_socket_type_xudp);
            peer_xudp_socket->connect_xudp(packet.get_to_ip_addr(), packet.get_to_ip_port(), this);

            xudp_client_map[to_addr] = peer_xudp_socket;
            XMETRICS_COUNTER_INCREMENT("xtransport_xudp_num", 1);
            AddToRatelimitMap(to_addr);
            TOP_INFO("reconn %s:%p", to_addr.c_str(), peer_xudp_socket);
        }
    }

    if (udp_property != nullptr) {
        udp_property->SetXudp(peer_xudp_socket);
        TOP_DEBUG("setxudp,udp_property:%p,xudp:%p", udp_property.get(), peer_xudp_socket);
    } else {
        TOP_DEBUG("udp_property null");
    }

    if (peer_xudp_socket->send(packet) != enum_xcode_successful) {
        TOP_ERROR("send xpacket failed!packet size is :%d\n",packet.get_size());
        return kTransportFailed;
    }
    TOP_DEBUG("send xpacket packet size:%d src %s:%d dest %s:%d\n",
              packet.get_size(),
              packet.get_from_ip_addr().c_str(),
              packet.get_from_ip_port(),
              packet.get_to_ip_addr().c_str(),
              packet.get_to_ip_port());

    return kTransportSuccess;
}
int XudpSocket::GetSign(std::string& node_sign) {
    if (global_node_id.empty() || global_node_signkey.empty())
        return enum_xerror_code_fail;

    uint256_t   hash_value;
    top::utl::xsha2_256_t hasher;
    hasher.update(global_node_id.c_str(), global_node_id.size());
    hasher.get_hash(hash_value);
    XMETRICS_GAUGE(metrics::cpu_hash_256_XudpSocket_calc, 1);
//    node_sign.assign(hash_value.data(), hash_value.size());

    top::utl::xecprikey_t pri_key_obj((uint8_t*)global_node_signkey.data());
//  top::uint256_t digest(hash);
    top::utl::xecdsasig_t signature_obj = pri_key_obj.sign(hash_value);
    node_sign.assign((char*)signature_obj.get_compact_signature(), signature_obj.get_compact_signature_size());

    TOP_INFO("node_id:%s,signkey:%s,node_sign:%s", global_node_id.c_str(), HexEncode(global_node_signkey).c_str(), HexEncode(node_sign).c_str());
    return enum_xcode_successful;
}
bool XudpSocket::CloseXudp(xp2pudp_t* xudpobj_ptr)
{
    if (xudpobj_ptr == nullptr) {
        TOP_WARN("close xudp fail:null");
        return false;
    }
#ifdef ENABLE_XSECURITY
        RemoveConlimitMap(xudpobj_ptr->get_peer_ip_address());
#endif

    //TODO find related item of xudp_client_map and clean it
    const std::string to_addr = xudpobj_ptr->get_peer_ip_address() + ":" + std::to_string(xudpobj_ptr->get_peer_real_port());
    
    {
        std::unique_lock<std::recursive_mutex> autolock(xudp_mutex_);
        std::map<std::string, xp2pudp_t*>::iterator it = xudp_client_map.find(to_addr);
        if (it == xudp_client_map.end() || it->second != xudpobj_ptr) {
            /*
            for (it= xudp_client_map.begin(); it != xudp_client_map.end(); ++it) {
                TOP_INFO("print xudp map:%s,%p",it->first.c_str(), it->second);
            }
            */
            TOP_WARN("close xudp: not find in map:%p.to_addr:%s", xudpobj_ptr, to_addr.c_str());
            return false;
        }
        xudp_client_map.erase(it);  // wait for UdpProperty destruction (remove from routing_table)
        XMETRICS_COUNTER_INCREMENT("xtransport_xudp_num", -1);
    }

    TOP_DBG_INFO("close xudp: find in map:%p, get_refcount:%d.call release_ref. close ip:%s:%d",
        xudpobj_ptr, xudpobj_ptr->get_refcount(), xudpobj_ptr->get_peer_ip_address().c_str(), xudpobj_ptr->get_peer_real_port());

    if (m_offline_cb != nullptr) {
        m_offline_cb(xudpobj_ptr->get_peer_ip_address(), xudpobj_ptr->get_peer_real_port());
    }
    xudpobj_ptr->SetStatus( top::transport::enum_xudp_status::enum_xudp_closed_released);
    xudpobj_ptr->close(true);
    xcontext_t::instance().delay_release_object(xudpobj_ptr);
    return true;
}

void XudpSocket::AddXip2Header(base::xpacket_t & packet, uint16_t priority_flag) {
    _xbase_header header;
    memset(&header, 0, sizeof(header));  // TODO(blueshi): init header

    header.flags |= priority_flag;
    header.ver_protocol = kVersionV1ProtocolProtobuf;
    packet.get_body().push_front((uint8_t *)&header, enum_xbase_header_len);
}

bool XudpSocket::GetSocketStatus() {
    if (this->get_socket_status() != top::base::enum_socket_status_connected) {
        return false;
    }
    return true;
}
    
int XudpSocket::AddXudp(const std::string& ip_port, xp2pudp_t* new_xudp) {
    if (new_xudp == nullptr)
        return 1;
    
    if (new_xudp->GetStatus() == top::transport::enum_xudp_status::enum_xudp_closed) {
        TOP_INFO("new_xudp is closed");
        return 1;
    } else if (new_xudp->GetStatus() == top::transport::enum_xudp_status::enum_xudp_closed_released) {
        TOP_INFO("new_xudp is closed and released");
        return 1;
    }

    xp2pudp_t* existing_ptr = nullptr;
    {
        std::unique_lock<std::recursive_mutex> autolock(xudp_mutex_);
        std::map<std::string, xp2pudp_t*>::iterator iter = xudp_client_map.find(ip_port);
        if (iter != xudp_client_map.end()) {
            existing_ptr = iter->second;
        } else {
            new_xudp->add_ref();
            xudp_client_map[ip_port] = new_xudp;
            XMETRICS_COUNTER_INCREMENT("xtransport_xudp_num", 1);
            TOP_DBG_INFO("conn ok %s:%p.call add_ref:%d", ip_port.c_str(), new_xudp, new_xudp->get_refcount());
            return enum_xcode_successful;
        }
        
        if (existing_ptr == new_xudp) {
            TOP_INFO("need not add this xudp:%p", existing_ptr);
            return 0;
        }
        
        new_xudp->add_ref();
        xudp_client_map[ip_port] = new_xudp;
        TOP_DBG_INFO("add to map:to_addr:%s,xudp:%p.call add_ref:%d", ip_port.c_str(), new_xudp, new_xudp->get_refcount());
    }
    
    if (existing_ptr != NULL) {
        if (existing_ptr->GetStatus() == top::transport::enum_xudp_status::enum_xudp_closed_released) {
            TOP_DBG_INFO("this xudp already released,addr:%s,old xudp:%p,new xudp:%p, get_refcount:%d", ip_port.c_str(), existing_ptr, new_xudp, existing_ptr->get_refcount());
            return 0;
        }
        TOP_DBG_INFO("replace:%s, replaced:%p, new:%p, get_refcount:%d. call release_ref", ip_port.c_str(), existing_ptr, new_xudp, existing_ptr->get_refcount());
        
//      if (m_offline_cb != nullptr) {
//          m_offline_cb(existing_ptr->get_peer_ip_address(), existing_ptr->get_peer_real_port());
//      }
        existing_ptr->SetStatus(top::transport::enum_xudp_status::enum_xudp_closed_released);
        existing_ptr->close(true);
        xcontext_t::instance().delay_release_object(existing_ptr); 
    }
    return enum_xcode_successful;
}
int XudpSocket::CheckRatelimitMap(const std::string& to_addr) {
    std::unique_lock<std::recursive_mutex> lock(xudp_ratelimit_mutex_);
    std::map<std::string, std::list<int64_t>>::iterator rate_it = xudp_ratelimit_map.find(to_addr);
    if (rate_it == xudp_ratelimit_map.end()) {
        return enum_xcode_successful;
    }
    std::list<int64_t>& conns = rate_it->second;
    if (conns.empty())
        return enum_xcode_successful;
    std::list<int64_t>::iterator conn_it;
    while(1) {
        conn_it = conns.begin();
        if (top::base::xtime_utl::gettimeofday() - *conn_it < XUDP_RATELIMIT_INTERVAL * 60) {  // recent connections
            break;
        }
        conns.pop_front();
        TOP_DBG_INFO("delete old connection:%s, remains:%d", to_addr.c_str(), conns.size());
        if (conns.empty())
            break;
    }
    if (conns.size() >= XUDP_RATELIMIT_CONNECTION_TIMES)  // reach connection rate limit
        return enum_xerror_code_bad_size;

    uint16_t count_one_minute = 0;
    for (conn_it = conns.begin(); conn_it != conns.end(); ++conn_it) {
        if (top::base::xtime_utl::gettimeofday() - *conn_it < 60)  // count connections within one minute
            count_one_minute++;
    }
    if (count_one_minute >= XUDP_RATELIMIT_CONNECTION_TIMES_WITHIN_MINUTE)  // reach connection rate limit
        return enum_xerror_code_over_limit;
    
    return enum_xcode_successful;
}

#ifdef ENABLE_XSECURITY
uint32_t XudpSocket::IpToUInt(const std::string& ip) {
    //SUB_NETWORK_MASK
    if (ip.empty()) {
        TOP_WARN("iptouint failed, ip empty");
        return 0;
    }
    uint32_t a, b, c, d;
    uint32_t addr = 0;
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        TOP_WARN("iptouint failed, ip:%s invalid", ip.c_str());
        return 0;
    }

    addr =  a << 24;
    addr |= b << 16;
    addr |= c << 8;
    addr |= d;
    return addr;
}

// when connect
bool XudpSocket::AddToConlimitMap(const std::string& ip) {
    return true;
    uint32_t ip_addr = IpToUInt(ip);
    uint32_t mask_addr = IpToUInt(SUB_NETWORK_MASK);
    if (ip_addr == 0 || mask_addr == 0) {
        TOP_WARN("connect limit map ip:%s invalid", ip.c_str());
        return false;
    }

    uint32_t ip_segment = (ip_addr & mask_addr);

    {
        std::unique_lock<std::mutex> lock(xudp_conlimit_map_mutex_);
        if (xudp_conlimit_map_[ip_segment] >= IP_SEG_MAX_ADDR) {
            TOP_WARN("ip:%s ip_seg:%u reach max_num:%u, deny connect", ip.c_str(), ip_segment, IP_SEG_MAX_ADDR);
            return false;
        }

        xudp_conlimit_map_[ip_segment] += 1;
    }
    return true;
}

// when close
void XudpSocket::RemoveConlimitMap(const std::string& ip) {
    uint32_t ip_addr = IpToUInt(ip);
    uint32_t mask_addr = IpToUInt(SUB_NETWORK_MASK);
    if (ip_addr == 0 || mask_addr == 0) {
        TOP_WARN("connect limit map ip:%s invalid", ip.c_str());
        return ;
    }

    uint32_t ip_segment = (ip_addr & mask_addr);

    {
        std::unique_lock<std::mutex> lock(xudp_conlimit_map_mutex_);
        if (xudp_conlimit_map_[ip_segment] > 0) {
            xudp_conlimit_map_[ip_segment] -= 1;
        }
        if (xudp_conlimit_map_[ip_segment] <= 0) {
            // avoid mem growing
            xudp_conlimit_map_.erase(ip_segment);
        }
    }
    TOP_DEBUG("remove connection limitmap for ip:%s", ip.c_str());
    return;
}
#endif

int XudpSocket::AddToRatelimitMap(const std::string& to_addr) {
    std::unique_lock<std::recursive_mutex> lock(xudp_ratelimit_mutex_);
    std::map<std::string, std::list<int64_t>>::iterator rate_it = xudp_ratelimit_map.find(to_addr);
    if (rate_it != xudp_ratelimit_map.end()) {
        rate_it->second.push_back(top::base::xtime_utl::gettimeofday());
        TOP_DBG_INFO("add ratelimit:%s", to_addr.c_str());
        return enum_xcode_successful;
    }
    std::list<int64_t> ratelimit_list;
    ratelimit_list.push_back(top::base::xtime_utl::gettimeofday());
    xudp_ratelimit_map[to_addr] = ratelimit_list;
    TOP_DBG_INFO("new ratelimit:%s", to_addr.c_str());  
    return enum_xcode_successful;
}
int XudpSocket::RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) {
    TOP_DBG_INFO("RegisterOfflineCallback");
    m_offline_cb = cb;
    return enum_xcode_successful;
}
int XudpSocket::RegisterNodeCallback(std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb) {
    TOP_DBG_INFO("RegisterNodeCallback");
    m_register_node_callback = cb;
    return enum_xcode_successful;
}

int XudpSocket::send_ping_packet(
        std::string target_ip_addr,
        uint16_t target_ip_port,
        uint16_t target_logic_port,
        uint16_t target_logic_port_token,
        const std::string & _payload,
        uint16_t TTL) {
    if(target_ip_addr.empty() || (0 == target_ip_port))
    {
        target_ip_addr = get_peer_ip_address();
        target_ip_port = get_peer_real_port();
    }

    if( (0 == target_logic_port) || (0 == target_logic_port_token) )
    {
        target_logic_port = get_peer_logic_port();
        target_logic_port_token = get_peer_logic_port_token();
    }

    base::xlink_ping_pdu _pdu(*get_context(),0);
    _pdu._pdu_payload  = _payload;
    _pdu._pdu_fire_TTL = TTL;
    _pdu._from_ip_address = get_local_ip_address();
    _pdu._from_ip_port = get_local_real_port();
    _pdu._to_ip_address = target_ip_addr;
    _pdu._to_ip_port = target_ip_port;

    base::xlinkhead_t& _pdu_header = _pdu.get_header();
    _pdu_header.set_from_logic_port(get_local_logic_port());
    _pdu_header.set_from_logic_port_token(get_local_logic_port_token());
    _pdu_header.set_to_logic_port(target_logic_port);
    _pdu_header.set_to_logic_port_token(target_logic_port_token);
    
    base::xpacket_t _packet;
    _packet.set_from_ip_addr(get_local_ip_address());
    _packet.set_from_ip_port(get_local_real_port());
    _packet.set_to_ip_addr(target_ip_addr);
    _packet.set_to_ip_port(target_ip_port);
    _packet.set_process_flag(enum_xpacket_process_flag_compress); //ask compress

    _pdu.serialize_to(_packet);

    return write_packet(_packet, 0, 0);
}

int32_t XudpSocket::on_ping_packet_recv(
        base::xpacket_t& packet_raw,
        int32_t cur_thread_id,
        uint64_t timenow_ms,
        xendpoint_t* from_child_end) {
    TOP_DEBUG("on_ping_packet_recv");
    base::xpacket_t packet;
    base::xlink_ping_pdu _pdu(*get_context(),0);//version 0
    if(_pdu.serialize_from(packet_raw) <= 0) {
        TOP_WARN("no pdu in packet");
        return enum_xcode_successful;
    }

    // base::xlinkhead_t& _header = _pdu.get_header();
    packet.set_from_ip_addr(packet_raw.get_from_ip_addr());
    packet.set_from_ip_port(packet_raw.get_from_ip_port());
    packet.set_to_ip_addr(packet_raw.get_to_ip_addr());
    packet.set_to_ip_port(packet_raw.get_to_ip_port());
    packet.get_body().push_back((uint8_t*)_pdu._pdu_payload.data(), _pdu._pdu_payload.size());

#ifdef ENABLE_XSECURITY
    if (TransportFilter::Instance()->BlackIpCheck(packet.get_from_ip_addr())) {
        TOP_WARN("on_recv deny ddos black_list ip:%s", packet.get_from_ip_addr().c_str());
        return enum_xerror_code_fail;
    }
    TransportFilter::Instance()->AddTrafficData(packet.get_size(), packet.get_from_ip_addr());
#endif



    multi_thread_message_handler_->HandleMessage(packet);
    return enum_xcode_successful;
}

int XudpSocket::SendPing(
        const xbyte_buffer_t& data,
        const std::string& peer_ip,
        uint16_t peer_port) {
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0,0, false);
    packet.get_body().push_back((uint8_t*)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(peer_ip);
    packet.set_to_ip_port(peer_port);
    AddXip2Header(packet);
    return SendPing(packet);
}

int XudpSocket::SendPing(base::xpacket_t& packet) {
    const std::string payload((char*)packet.get_body().data(), (size_t)packet.get_body().size());
    auto ret = send_ping_packet(packet.get_to_ip_addr(), packet.get_to_ip_port(), 0, 0, payload, 1);
    TOP_DEBUG("send_ping_packet, ret = %d", ret);
    return ret;
}

}  // namespace transport
}  // namespace top
