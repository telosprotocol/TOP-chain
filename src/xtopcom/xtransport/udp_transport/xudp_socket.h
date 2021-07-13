// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <atomic>

#include "xtransport/udp_transport/socket_intf.h"
#include "xbase/xlog.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xbase/xdata.h"
#include "xbase/xpacket.h"
#include "xbase/xsocket.h"
#include "xbase/xutl.h"

#include "xtransport/transport.h"
#include "xbasic/xbyte_buffer.h"
#include "xpbase/base/top_utils.h"
#include "xkad/routing_table/node_info.h"

using namespace top;
using namespace base;

namespace top {
namespace kadmlia {
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
};

namespace transport {

class MultiThreadHandler;
class UdpTransport;
class SocketIntf;

enum class enum_xudp_status {
    enum_xudp_init,
    enum_xudp_connecting,
    enum_xudp_connected,
    enum_xudp_closed,  // closed, not released
    enum_xudp_closed_released,  // closed, released
};

class xp2pudp_t;
class XudpSocket;

class UdpProperty {
public:
    ~UdpProperty();
public:
    xp2pudp_t* GetXudp() {
        return m_xudp_;
    }
    void SetXudp(xp2pudp_t* xudp_in);

private:
    xp2pudp_t* m_xudp_{nullptr};
};


class xp2pudp_t : public base::xudp_t
{
public:
    xp2pudp_t(xcontext_t & _context,xendpoint_t * parent,const int32_t target_thread_id,int64_t virtual_handle,xsocket_property & property, XudpSocket* listen_server)
     : xudp_t(_context,parent,target_thread_id,virtual_handle,property)
    {
        m_link_refcount = 0;
        m_status = top::transport::enum_xudp_status::enum_xudp_init;
        listen_server_ = listen_server;
    }
protected:
    virtual ~xp2pudp_t()
    {
        xassert(m_link_refcount == 0);
    };
private:
    xp2pudp_t();
    xp2pudp_t(const xp2pudp_t &);
    xp2pudp_t & operator = (const xp2pudp_t &);
protected:
    //notify the child endpont is ready to use when receive on_endpoint_open of error_code = enum_xcode_successful
    virtual bool             on_endpoint_open(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
    //when associated io-object close happen,post the event to receiver
    //error_code is 0 when it is closed by caller/upper layer
    virtual bool             on_endpoint_close(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
    //notify upper layer,child endpoint update keeplive status
    virtual bool             on_endpoint_keepalive(const std::string & _payload,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
protected:

    //receive data packet that already trim header of link layer
    virtual int32_t recv(uint64_t from_xip_addr_low,uint64_t from_xip_addr_high,uint64_t to_xip_addr_low,uint64_t to_xip_addr_high,xpacket_t & packet,int32_t cur_thread_id,uint64_t timenow_ms,xendpoint_t* from_child_end) override;
public:
    int send(xpacket_t& packet);
    int32_t connect_xudp(const std::string& target_ip,const uint16_t target_port, XudpSocket* udp_server);
    enum_xudp_status GetStatus() {return m_status;}
    void SetStatus(enum_xudp_status s) { m_status = s;}

    int    add_linkrefcount();
    int    release_linkrefcount();
protected:
    std::atomic<int>  m_link_refcount;  // indicate how many routing table linked this socket
private:
    enum_xudp_status m_status;  // 0 init, 1 connecting, 2 connected, 3 closed
    XudpSocket* listen_server_;
};

class XudpSocket : protected base::xudplisten_t, public SocketIntf {
public:
    XudpSocket(
            base::xcontext_t& context,
            int32_t target_thread_id,
            xfd_handle_t native_handle,
            MultiThreadHandler* message_handler);
    virtual ~XudpSocket() override;
    void Stop() override;
    int SendData(base::xpacket_t& packet) override;
    int SendDataWithProp(std::string const & data,const std::string & peer_ip,uint16_t peer_port,UdpPropertyPtr& udp_property,uint16_t priority_flag = 0);
    int SendDataWithProp(base::xpacket_t & packet, UdpPropertyPtr & udp_property);
    int SendToLocal(base::xpacket_t& packet) override;
    void AddXip2Header(base::xpacket_t& packet,uint16_t priority_flag = 0) override;
    bool GetSocketStatus() override;

    void register_on_receive_callback(on_receive_callback_t callback) override;
    void unregister_on_receive_callback() override;

    int AddXudp(const std::string& ip_port, xp2pudp_t* xudp);
    bool CloseXudp(xp2pudp_t* xudp);

    virtual void StartRead() override {
        base::xudplisten_t::start_read(0);
    }

    virtual void Close() override {
        // TODO(blueshi): how to close once?
        // auto ref1 = base::xudplisten_t::get_refcount();
        // if (ref1 > 1) {
            base::xudplisten_t::close();
        // }

        auto future = future_;
        future.get();  // block wait until closed

        // TODO(blueshi): how to release?
        // while (base::xudplisten_t::get_refcount() > 0) {
        //     base::xudplisten_t::release_ref();
        // }
    }

    virtual uint16_t GetLocalPort() override {
        if (local_port_ == 0) {
            local_port_ = base::xudplisten_t::get_local_real_port();
        }
        return local_port_;
    }

    virtual const std::string& GetLocalIp() override {
        if (local_ip_.empty()) {
            local_ip_  = base::xudplisten_t::get_local_ip_address();
        }
        return local_ip_;
    }

    virtual int SendPing(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    virtual int SendPing(base::xpacket_t& packet) override;
    virtual int RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) override;
    virtual int RegisterNodeCallback(std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb) override;
    virtual int CheckRatelimitMap(const std::string& to_addr) override;

protected:
    virtual xslsocket_t* create_xslsocket(xendpoint_t * parent,xfd_handle_t handle,xsocket_property & property, int32_t cur_thread_id,uint64_t timenow_ms) override;

    int send_ping_packet(
            std::string target_ip_addr,
            uint16_t target_ip_port,
            uint16_t target_logic_port,
            uint16_t target_logic_port_token,
            const std::string & _payload,
            uint16_t TTL);
    virtual int32_t on_ping_packet_recv(
            base::xpacket_t& packet_raw,
            int32_t cur_thread_id,
            uint64_t timenow_ms,
            xendpoint_t* from_child_end) override;
    virtual bool on_object_close() override {
        auto ret = base::xudplisten_t::on_object_close();
        promise_.set_value();  // wake block
        return ret;
    }
    virtual xslsocket_t*	 on_xslsocket_accept(xfd_handle_t handle,xsocket_property & property, int32_t cur_thread_id,uint64_t timenow_ms);

private:
    XudpSocket();
    XudpSocket(const XudpSocket &);
    XudpSocket & operator = (const XudpSocket &);
    int AddToRatelimitMap(const std::string& to_addr);
    int GetSign(std::string& node_sign);

#ifdef ENABLE_XSECURITY
    uint32_t IpToUInt(const std::string& ip);
    // return true if add success (ip_seg not larger than max), else false
    bool AddToConlimitMap(const std::string& ip);
    void RemoveConlimitMap(const std::string& ip);
#endif

private:
    int32_t     m_xudpsocket_mgr_thread_id;    //dedicated thread to manage xudpt socket (include keepalive,lifemanagent)
    on_receive_callback_t callback_;
    std::mutex callback_mutex_;
    std::promise<void> promise_;
    std::shared_future<void> future_{promise_.get_future()};
    std::string local_ip_;
    uint16_t local_port_ {0};

    std::map<std::string, xp2pudp_t*> xudp_client_map;  // ip+port, xp2pudp_t*
    std::recursive_mutex xudp_mutex_;
    std::map<std::string, std::list<int64_t>> xudp_ratelimit_map;  // ip+port, xudp connection time
    std::recursive_mutex xudp_ratelimit_mutex_;

#ifdef ENABLE_XSECURITY
    std::mutex xudp_conlimit_map_mutex_;
    // key is uint32_t(ip/24), value is count
    std::map<uint32_t, std::atomic<uint32_t>> xudp_conlimit_map_;
#endif

public:
    MultiThreadHandler* multi_thread_message_handler_;
    std::function<void(const std::string& ip, const uint16_t port)> m_offline_cb;
    std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> m_register_node_callback;
};

}  // namespace transport
}  // namespace top
