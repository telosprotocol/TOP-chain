// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent_common.h"
#include "xmbus/xmessage_bus.h"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xmetrics/xmetrics.h"
#include "xsync/xmessage_pack.h"
#include "xvnetwork/xvhost_face.h"
#include "xsync/xsync_handler.h"

NS_BEG2(top, sync)

class xsync_netmsg_dispatcher_t {
public:
    virtual ~xsync_netmsg_dispatcher_t() {}

    xsync_netmsg_dispatcher_t(std::string vnode_id, const std::vector<observer_ptr<base::xiothread_t>> &thread_pool, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<vnetwork::xvhost_face_t> &vhost,
        xsync_handler_t *sync_handler);
    void watch(vnetwork::xvnetwork_driver_face_t* driver);
    void unwatch(vnetwork::xvnetwork_driver_face_t* driver);
    void on_receive(vnetwork::xvnode_address_t const & addr, vnetwork::xmessage_t const & msg, std::uint64_t const, vnetwork::xvnode_address_t const & vnetwork_self);

private:
    void dispatch(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xbyte_buffer_t &msg,
        vnetwork::xmessage_t::message_type msg_type,
        vnetwork::xtop_vnetwork_message::hash_result_type msg_hash);
    int64_t get_time();

protected:
    std::string m_vnode_id;
    std::vector<observer_ptr<base::xiothread_t>> m_thread_pool;
    uint32_t m_thread_count{0};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    observer_ptr<vnetwork::xvhost_face_t> m_vhost{};
    xsync_handler_t *m_sync_handler{};
};

NS_END2
