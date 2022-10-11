// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xlog.h"
#include "xbase/xobject.h"
#include "xbase/xpacket.h"
#include "xbase/xsocket.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/transport.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

namespace top {

namespace kadmlia {
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
};  // namespace kadmlia

namespace transport {

class SocketIntf {
public:
    virtual ~SocketIntf() {
    }
    virtual void Stop() = 0;
    virtual int SendData(base::xpacket_t & packet) = 0;
    virtual int SendDataWithProp(std::string const & data, const std::string & peer_ip, uint16_t peer_port, UdpPropertyPtr & udp_property, uint16_t priority_flag = 0) = 0;

    virtual int SendDataWithProp(base::xpacket_t & packet, UdpPropertyPtr & udp_property) = 0;
    virtual int SendToLocal(base::xpacket_t & packet) = 0;
    virtual void AddXip2Header(base::xpacket_t & packet, uint16_t priority_flag = 0) = 0;
    virtual bool GetSocketStatus() = 0;

    virtual void StartRead() = 0;
    virtual void Close() = 0;
    virtual uint16_t GetLocalPort() = 0;
    virtual const std::string & GetLocalIp() = 0;

    virtual int SendPing(const xbyte_buffer_t & data, const std::string & peer_ip, uint16_t peer_port) = 0;
    virtual int SendPing(base::xpacket_t & packet) = 0;
    virtual int RegisterOfflineCallback(std::function<void(const std::string & ip, const uint16_t port)> cb) = 0;
    virtual int RegisterNodeCallback(std::function<int32_t(std::string const & node_addr, std::string const & node_sign)> cb) = 0;
    virtual int CheckRatelimitMap(const std::string & to_addr) = 0;
};

}  // namespace transport
}  // namespace top
