// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"
#include "xsync/xsync_netmsg_dispatcher.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xdatautil.h"
#include <inttypes.h>

NS_BEG2(top, sync)

class xsync_netmsg_dispatcher_thread_event_para_t : public top::base::xobject_t {
public:
    xsync_netmsg_dispatcher_thread_event_para_t(xsync_handler_t * sync_handler,
                                                const vnetwork::xvnode_address_t & from_address,
                                                const vnetwork::xvnode_address_t & network_self,
                                                const xbyte_buffer_t & msg,
                                                vnetwork::xmessage_t::message_type msg_type,
                                                vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
                                                int64_t recv_time)
      : m_sync_handler(sync_handler)
      , m_from_address(from_address)
      , m_network_self(network_self)
      , m_msg(std::move(msg))
      , m_msg_type(msg_type)
      , m_msg_hash(msg_hash)
      , m_recv_time(recv_time) {
    }

private:
    ~xsync_netmsg_dispatcher_thread_event_para_t() override {}

public:
    xsync_handler_t *m_sync_handler;
    vnetwork::xvnode_address_t m_from_address;
    vnetwork::xvnode_address_t m_network_self;
    xbyte_buffer_t m_msg;
    vnetwork::xmessage_t::message_type m_msg_type;
    vnetwork::xvnetwork_message_t::hash_result_type m_msg_hash;
    int64_t m_recv_time;
};

static bool xsync_netmsg_dispatcher_thread_event(top::base::xcall_t& call, const int32_t thread_id, const uint64_t timenow_ms) {
    xsync_netmsg_dispatcher_thread_event_para_t* para = dynamic_cast<xsync_netmsg_dispatcher_thread_event_para_t*>(call.get_param1().get_object());
    para->m_sync_handler->on_message(para->m_from_address, para->m_network_self, para->m_msg, para->m_msg_type, para->m_msg_hash, para->m_recv_time);
    return true;
}

xsync_netmsg_dispatcher_t::xsync_netmsg_dispatcher_t(std::string vnode_id, const std::vector<observer_ptr<base::xiothread_t>> &thread_pool,
            const observer_ptr<mbus::xmessage_bus_face_t> &mbus, const observer_ptr<vnetwork::xvhost_face_t> &vhost,
            xsync_handler_t *sync_handler):
m_vnode_id(vnode_id),
m_thread_pool(thread_pool),
m_bus(mbus),
m_vhost(vhost),
m_sync_handler(sync_handler){
    m_thread_count = thread_pool.size();
}

void xsync_netmsg_dispatcher_t::watch(vnetwork::xvnetwork_driver_face_t* driver) {
    xsync_info("xsync_netmsg_dispatcher_t watch %s", driver->address().to_string().c_str());
    driver->register_message_ready_notify(xmessage_category_sync,
            std::bind(&xsync_netmsg_dispatcher_t::on_receive,
            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            driver->address()));
}

void xsync_netmsg_dispatcher_t::unwatch(vnetwork::xvnetwork_driver_face_t* driver) {
    xsync_info("xsync_netmsg_dispatcher_t unwatch %s", driver->address().to_string().c_str());
    driver->unregister_message_ready_notify(xmessage_category_sync);
}

void xsync_netmsg_dispatcher_t::on_receive(vnetwork::xvnode_address_t const & addr, vnetwork::xmessage_t const & msg,
                                                    std::uint64_t const, vnetwork::xvnode_address_t const & vnetwork_self) {

    vnetwork::xmessage_t::message_type msg_type = msg.id();
    // uint32_t msg_size = msg.payload().size();
    xsync_dbg("xsync_netmsg_dispatcher_t on_receive_msg received %x %" PRIx64 " ", msg_type, msg.hash());
    xbyte_buffer_t message;
    xmessage_pack_t::unpack_message(msg.payload(), msg_type, message);
    dispatch(addr, vnetwork_self, message, msg_type, msg.hash());
}

void xsync_netmsg_dispatcher_t::dispatch(
    const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xbyte_buffer_t &msg,
    vnetwork::xmessage_t::message_type msg_type,
    vnetwork::xvnetwork_message_t::hash_result_type msg_hash) {

    if (msg.size() == 0) {
        xsync_warn("xsync_netmsg_dispatcher_t wrong message from remote %s",
                from_address.to_string().c_str());
        //notify_deceit_node(from_address);
        return;
    }

    int64_t recv_time = get_time();
    base::xauto_ptr<xsync_netmsg_dispatcher_thread_event_para_t> para = new xsync_netmsg_dispatcher_thread_event_para_t(m_sync_handler, from_address, network_self, msg, msg_type, msg_hash, recv_time);
    top::base::xparam_t param(para.get());
    top::base::xcall_t tmp_func((top::base::xcallback_ptr)xsync_netmsg_dispatcher_thread_event, param);

    static uint32_t thread_index = 0;
    uint32_t idx = (thread_index++)%m_thread_count;

    // TODO use semaphore & task queue
    auto ret = m_thread_pool[idx]->send_call(tmp_func);

    if (ret != 0) {
        xsync_warn("send call failed %d", ret);
    }
}

int64_t xsync_netmsg_dispatcher_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

NS_END2
