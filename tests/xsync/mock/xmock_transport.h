// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include "xvnetwork/xaddress.h"
#include "xbase/xthread.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
#include "xmock_vhost.hpp"

namespace top { namespace mock {

using xmock_transport_target_search_callback = std::function<int32_t(const vnetwork::xvnode_address_t &, std::vector<std::shared_ptr<xmock_vhost_t>> &)>; 

class xmock_transport_item_t {
public:
    std::shared_ptr<xmock_vhost_t> dst_vhost;
    top::vnetwork::xvnode_address_t src;
    top::vnetwork::xmessage_t message;
    int64_t tm;
    uint32_t timeout;
};

class xmock_transport_timer_t;

class xmock_transport_t {
public:

    xmock_transport_t(top::base::xiothread_t* thread, xmock_transport_target_search_callback cb);
    ~xmock_transport_t();

    void start();
    void stop();
    void set_delay(uint32_t delay_ms);
    void set_packet_loss_rate(uint32_t rate);

    int32_t unicast(top::vnetwork::xvnode_address_t const &src, top::vnetwork::xvnode_address_t const & dst, const top::vnetwork::xmessage_t &message);
    int32_t broadcast(top::vnetwork::xvnode_address_t const &src, const top::vnetwork::xmessage_t &message);

    void on_timer();

private:
    bool m_is_start{false};
    top::base::xiothread_t* m_thread{nullptr};
    xmock_transport_target_search_callback m_cb;
    xmock_transport_timer_t *m_timer{nullptr};
    std::mutex m_mutex;
    std::list<xmock_transport_item_t> m_list;
    uint32_t m_delay_ms{0};
    uint32_t m_packet_loss_rate{0};
};


class xmock_transport_timer_t : public top::base::xxtimer_t {
public:
    xmock_transport_timer_t(base::xcontext_t &_context, int32_t timer_thread_id, xmock_transport_t *fwd)
    : base::xxtimer_t(_context, timer_thread_id) {
        m_fwd = fwd;
    }

protected:
    ~xmock_transport_timer_t() override {
    }

protected:
    bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override {
        //printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
        m_fwd->on_timer();
        return true;
    }

    xmock_transport_t *m_fwd;
};

}
}
