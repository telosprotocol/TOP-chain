// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/udp_transport/udp_transport.h"

#include <stdio.h>
#include <mutex>
#include <iostream>
#include <string>

#include "xbase/xcontext.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#include "xtransport/udp_transport/xudp_socket.h"
#include "xtransport/utils/transport_utils.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/udp_transport/transport_filter.h"

using namespace top;
using namespace top::base;

namespace top {
namespace transport {
static const uint32_t kDumpBandWidthPeriod = 4 * 1000; // 4 seconds

UdpTransport::UdpTransport()
        : io_thread_(NULL),
          udp_socket_(NULL),
          local_ip_(),
          local_port_(0),
          socket_connected_(false),
          udp_handle_(0),
          message_handler_(NULL) {}

UdpTransport::~UdpTransport() {
//    Stop();
}

void UdpTransport::SetOptBuffer() {
    // recv buf
    {
        const int recv_get = base::xsocket_utl::get_recv_buffer(udp_handle_) / 2;
        TOP_INFO("initial recv buf size %d KB", recv_get / 1024);
    }
    const int recv_buf_size = 8 * 1024 * 1024; // 8MB
    base::xsocket_utl::set_recv_buffer(udp_handle_, recv_buf_size);
    const int recv_get = base::xsocket_utl::get_recv_buffer(udp_handle_) / 2;
    if (recv_get < recv_buf_size) {
        TOP_INFO("recv_get(%d) < recv_buf_size(%d)", recv_get / 1024, recv_buf_size / 1024);
    }
    TOP_INFO("new recv buf: %d KB", recv_get / 1024);

    // send buf
    {
        const int send_get = base::xsocket_utl::get_send_buffer(udp_handle_) / 2;
        TOP_INFO("initial send buf size %d KB", send_get / 1024);
    }
    const int send_buf_size = 8 * 1024 * 1024; // 8MB
    base::xsocket_utl::set_send_buffer(udp_handle_, send_buf_size);
    const int send_get = base::xsocket_utl::get_send_buffer(udp_handle_) / 2;
    if (send_get < send_buf_size) {
        TOP_FATAL("send_get(%d) < send_buf_size(%d)", send_get / 1024, send_buf_size / 1024);
    }
    TOP_INFO("new send buf: %d KB", send_get / 1024);
}

int UdpTransport::Start(
        const std::string& local_ip,
        uint16_t local_port,
        MultiThreadHandler* message_handler) {
    TOP_INFO("UdpTransport::Start(%s:%d) ...", local_ip.c_str(), local_port);

    TOP_INFO("starting thread(UdpTransport) ...");
    io_thread_ = top::base::xiothread_t::create_thread(
        top::base::xcontext_t::instance(), 0, -1);
    if (io_thread_ == NULL) {
        TOP_ERROR("create xio thread failed!");
        return kTransportFailed;
    }

    udp_handle_ = base::xsocket_utl::udp_listen(
            "0.0.0.0",
            local_port);
    if (udp_handle_ <= 0) {
        TOP_ERROR("udp listen failed!");
        return kTransportFailed;
    }
#if defined(LINUX) || defined(linux) || defined(__linux) || defined(__linux__)
    SetOptBuffer();
    base::xsocket_utl::set_recv_buffer(udp_handle_, 8 * 1024 * 1024);
#else
    auto send_buf_size = 4 * 1024 * 1024;
    base::xsocket_utl::set_send_buffer(udp_handle_, send_buf_size);
    base::xsocket_utl::set_recv_buffer(udp_handle_, send_buf_size);  // 4 M for mac
#endif


    message_handler_ = message_handler;

    udp_socket_ = new XudpSocket(base::xcontext_t::instance(), io_thread_->get_thread_id(), udp_handle_, message_handler_);

    udp_socket_->StartRead();
    local_ip_ = local_ip;
    local_port_ = udp_socket_->GetLocalPort();
    TOP_INFO("UdpTransport::Start() success[%s:%d]", this->local_ip().c_str(), this->local_port());
    socket_connected_ = true;

    TransportFilter::Instance()->Init();
    return kTransportSuccess;
}

void UdpTransport::Stop() {
    if (udp_socket_) {
        udp_socket_->Stop();
        udp_socket_->Close();
        udp_socket_ = nullptr;
    }
    socket_connected_ = false;
}

int UdpTransport::SendDataWithProp(std::string const & data, const std::string & peer_ip, uint16_t peer_port, UdpPropertyPtr & udp_property, uint16_t priority_flag) {
    if (!socket_connected_) {
        TOP_ERROR("udp socket not alive, SendData failed");
        return kTransportFailed;
    }
    if (udp_socket_ == NULL) {
        TOP_ERROR("udp socket is NULL");
        return kTransportFailed;
    }

    return udp_socket_->SendDataWithProp(data, peer_ip, peer_port, udp_property, priority_flag);
}

int UdpTransport::ReStartServer() {
    std::unique_lock<std::mutex> lock(restart_xbase_udp_server_mutex_);

    if (socket_connected_) {
        TOP_ERROR("udp socket alive, there is no need to restart xbase udp socket");
        return kTransportFailed;
    }
    if (!udp_socket_){
        TOP_ERROR("udp_socket_ not ready yet, ReStartServer forbidden");
        return kTransportFailed;
    }

    // stop first
    udp_socket_->Stop();
    udp_socket_->Close();
    udp_socket_ = nullptr;

    // then restart
    udp_handle_ = base::xsocket_utl::udp_listen(
            "0.0.0.0",
            local_port_);
    if (udp_handle_ <= 0) {
        TOP_ERROR("udp listen failed!");
        return kTransportFailed;
    }

    // SetOptBuffer();

 
        udp_socket_ = new XudpSocket(
            base::xcontext_t::instance(),
            io_thread_->get_thread_id(),
            udp_handle_,
            message_handler_);
        TOP_FATAL("new socket(%p)", udp_socket_);

    udp_socket_->StartRead();
    socket_connected_ = true;
    TOP_INFO("UdpTransport::ReStartServer() success");
    return kTransportSuccess;
}

int UdpTransport::get_socket_status() {
    if (!socket_connected_) {
        TOP_ERROR("UdpTransport::get_udp_socket_status canceled, UdpSocket maybe restarting");
        return kUdpSocketStatusCanceled;
    }
    if (!udp_socket_) {
        TOP_ERROR("udp socket null");
        return kUdpSocketStatusNull;
    }
    if (udp_socket_->GetSocketStatus()) {
        return kUdpSocketStatusConnected;
    } else {
        socket_connected_ = false;
        TOP_INFO("UdpSocket not connected, set socket_connected_ false");
        return kUdpSocketStatusNotConnected;
    }
}

void UdpTransport::register_on_receive_callback(on_receive_callback_t callback) {
    if (udp_socket_) {
        udp_socket_->register_on_receive_callback(callback);
    }
}

void UdpTransport::unregister_on_receive_callback() {
    if (udp_socket_) {
        udp_socket_->unregister_on_receive_callback();
    }
}

int UdpTransport::SendPing(
        const xbyte_buffer_t& data,
        const std::string& peer_ip,
        uint16_t peer_port) {
    if (!socket_connected_) {
        TOP_ERROR("udp socket not alive, SendPing failed");
        return kTransportFailed;
    }
    if (udp_socket_ == NULL) {
        TOP_ERROR("udp socket is NULL");
        return kTransportFailed;
    }
    return udp_socket_->SendPing(data, peer_ip, peer_port);
}

int UdpTransport::SendPing(base::xpacket_t& packet) {
    if (!socket_connected_) {
        TOP_ERROR("udp socket not alive, SendPing failed");
        return kTransportFailed;
    }
    if (udp_socket_ == NULL) {
        TOP_ERROR("udp socket is NULL");
        return kTransportFailed;
    }

    // TODO(smaug(usually not add header here
    //udp_socket_->AddXip2Header(packet);
    return udp_socket_->SendPing(packet);
}
int UdpTransport::RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) {
    if (udp_socket_) {
        return udp_socket_->RegisterOfflineCallback(cb);
    }
    return kTransportSuccess;
}
int UdpTransport::RegisterNodeCallback(std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb) {
    if (udp_socket_) {
        return udp_socket_->RegisterNodeCallback(cb);
    }
    return kTransportSuccess;
}
int UdpTransport::CheckRatelimitMap(const std::string& to_addr)
{
	if (udp_socket_)
		return udp_socket_->CheckRatelimitMap(to_addr);
	return kTransportSuccess;
}
}  // namespace transport
}  // namespace top
