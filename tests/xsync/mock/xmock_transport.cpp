// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmock_transport.h"

namespace top { namespace mock {

class xmock_transport_thread_event_para_t : public top::base::xobject_t {
public:
    xmock_transport_thread_event_para_t(std::shared_ptr<xmock_vhost_t> &dst_vhost, 
                top::vnetwork::xvnode_address_t const &src, const top::vnetwork::xmessage_t &message)
    : m_dst_vhost(dst_vhost)
    , m_src(src)
    , m_message(message) {
    }
private:
    ~xmock_transport_thread_event_para_t() override {}

public:
    std::shared_ptr<xmock_vhost_t> m_dst_vhost;
    top::vnetwork::xvnode_address_t m_src;
    top::vnetwork::xmessage_t m_message;
};

static bool xmock_transport_thread_event(top::base::xcall_t& call, const int32_t thread_id, const uint64_t timenow_ms) {
    xmock_transport_thread_event_para_t* para = dynamic_cast<xmock_transport_thread_event_para_t*>(call.get_param1().get_object());

    std::shared_ptr<xmock_vhost_t> &dst_vhost = para->m_dst_vhost;
    top::vnetwork::xvnode_address_t &src = para->m_src;
    top::vnetwork::xmessage_t &message = para->m_message;

    dst_vhost->on_message(src, message);
    return true;
}

xmock_transport_t::xmock_transport_t(top::base::xiothread_t* thread, xmock_transport_target_search_callback cb)
: m_thread(thread)
, m_cb(cb)
, m_timer(nullptr)
, m_delay_ms(0)
, m_packet_loss_rate(0) {
}

xmock_transport_t::~xmock_transport_t() {
}

void xmock_transport_t::start() {
    m_timer = new xmock_transport_timer_t(top::base::xcontext_t::instance(), m_thread->get_thread_id(), this);
    m_timer->start(0, 10);
    m_is_start = true;
}

void xmock_transport_t::stop() {
    m_is_start = false;

    while (1) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_list.size() == 0)
                break;
        }

        top::base::xtime_utl::sleep_ms(10);
    }

    m_timer->close();
    while (m_timer->is_active())
        top::base::xtime_utl::sleep_ms(10);

    top::base::xtime_utl::sleep_ms(100);
}

void xmock_transport_t::set_delay(uint32_t delay_ms) {
    m_delay_ms = delay_ms;
}

void xmock_transport_t::set_packet_loss_rate(uint32_t rate) {
    m_packet_loss_rate = rate;
}

int32_t xmock_transport_t::unicast(top::vnetwork::xvnode_address_t const &src, top::vnetwork::xvnode_address_t const & dst, const vnetwork::xmessage_t &message) {
    if (!m_is_start) {
        return -1;
    }

    //printf("####  %s   >>>>>>   %s\n", src.to_string().c_str(), dst.to_string().c_str());

    assert(src != dst);

    std::vector<std::shared_ptr<xmock_vhost_t>> vector_src_vnet;
    std::vector<std::shared_ptr<xmock_vhost_t>> vector_dst_vhost;

    int rc = 0;
    rc = m_cb(src, vector_src_vnet);
    assert(rc == 0);
    assert(vector_src_vnet.size() == 1);
    rc = m_cb(dst, vector_dst_vhost);
    assert(rc == 0);

    xmock_send_filter_callback send_filter = nullptr;
    vector_src_vnet[0]->get_send_filter(send_filter);

    vnetwork::xmessage_t message_1;
    if (send_filter != nullptr) {
        int32_t ret = send_filter(message, message_1);
        if (ret == 0) {
            //
        } else if (ret > 0) {
            message_1 = message;
        } else if (ret < 0) {
            printf("%s >>>> %s discard\n", src.to_string().c_str(), dst.to_string().c_str());
            return 0;
        }
    } else {
        message_1 = message;
    }

    for (auto it: vector_dst_vhost) {
        std::shared_ptr<xmock_vhost_t> dst_vhost = it;
        xmock_recv_filter_callback recv_filter = nullptr;
        vnetwork::xmessage_t message_2;

        if (src == dst_vhost->address())
            continue;

        dst_vhost->get_recv_filter(recv_filter);
        if (recv_filter != nullptr) {
            int32_t ret = recv_filter(message_1, message_2);
            if (ret == 0) {
                //
            } else if (ret > 0) {
                message_2 = message_1;
            } else if (ret < 0) {
                printf("%s >>>> %s filter discard\n", src.to_string().c_str(), dst_vhost->address().to_string().c_str());
                continue;
            }
        } else {
            message_2 = message_1;
        }

        if (m_packet_loss_rate != 0) {
            uint32_t v = base::xtime_utl::get_fast_randomu() % 100;
            if (v < m_packet_loss_rate) {
                printf("%s >>>> %s random discard\n", src.to_string().c_str(), dst_vhost->address().to_string().c_str());
                continue;
            }
        }

        if (m_delay_ms != 0) {
            printf("%s >>>> %s delay:%ums\n", src.to_string().c_str(), dst_vhost->address().to_string().c_str(), m_delay_ms);
            xmock_transport_item_t item;
            item.dst_vhost = dst_vhost;
            item.src = src;
            item.message = message_2;
            item.tm = base::xtime_utl::time_now_ms();
            item.timeout = m_delay_ms;

            std::mutex m_mutex;
            m_list.push_back(item);
            continue;
        }

        //printf("%s >>>> %s\n", src.to_string().c_str(), dst_vhost->address().to_string().c_str());

        base::xauto_ptr<xmock_transport_thread_event_para_t> para = new xmock_transport_thread_event_para_t(dst_vhost, src, message);
        top::base::xparam_t param(para.get());
        top::base::xcall_t tmp_func((top::base::xcallback_ptr)xmock_transport_thread_event, param);
        auto ret = m_thread->send_call(tmp_func);

        if (ret != 0) {
            printf("send call failed %d\n", ret);
            assert(0);
        }
    }

    return 0;
}

int32_t xmock_transport_t::broadcast(top::vnetwork::xvnode_address_t const &src, const top::vnetwork::xmessage_t &message) {
    if (!m_is_start) {
        return -1;
    }
#if 0
    if (!m_is_start) {
        assert(0);
        int *p = 0x0;
        *p = 1;
    }

    for (uint32_t i=0; i<m_nodes->size(); i++) {
        const top::vnetwork::xvnode_address_t &addr = (*m_nodes)[i]->get_vnet()->address();
        if (addr == src)
            continue;
        (*m_nodes)[i]->on_message(src, message);
    }
#endif
    return 0;
}

void xmock_transport_t::on_timer() {
    int64_t now = base::xtime_utl::time_now_ms();
    while (1) {
        std::mutex m_mutex;

        if (m_list.empty())
            break;

        xmock_transport_item_t &item = m_list.front();
        if (now >= (item.tm + item.timeout)) {
            const top::vnetwork::xvnode_address_t &src = item.src;
            const top::vnetwork::xmessage_t &message = item.message;
            std::shared_ptr<xmock_vhost_t> &dst_vhost = item.dst_vhost;

            dst_vhost->on_message(src, message);
            m_list.pop_front();
        }
        else
            break;
    }
}


}
}
