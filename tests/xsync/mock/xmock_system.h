// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xaddress.h"
#include "xbase/xthread.h"
#include "xstore/xstore_face.h"
#include "xmock_transport.h"
#include "xmock_vnet.hpp"
#include "xmock_node.h"
#include "../../mock/xmock_network.hpp"
#include "../../mock/xmock_nodesrv.hpp"
#include "../../mock/xmock_auth.hpp"

namespace top { namespace mock {

class simple_timer;

class xmock_system_t {
public:
    xmock_system_t(xmock_network_t &network);
    ~xmock_system_t();

    void start();
    void stop();
    void on_timer();

    std::shared_ptr<xmock_node_t> get_node(const std::string &node_name);
    std::vector<std::shared_ptr<xmock_node_t>> get_group_node(const std::string &group_name);

    void set_delay(uint32_t delay_ms);
    void set_packet_loss_rate(uint32_t rate);

private:
    void create_mock_node(std::vector<std::shared_ptr<xmock_node_info_t>> &all_nodes);
    int32_t get_vhost(const top::vnetwork::xvnode_address_t &addr, std::vector<std::shared_ptr<xmock_vhost_t>> &vector_vhost);

private:
    bool m_is_start{false};
    base::xiothread_t* m_thread{nullptr};
    simple_timer* m_timer{nullptr};
    top::base::xiothread_t* m_forward_thread;
    std::shared_ptr<xmock_transport_t> m_forward_ptr;

    std::map<top::vnetwork::xvnode_address_t, std::shared_ptr<xmock_node_t>> m_addr_nodes;
    std::map<top::vnetwork::xcluster_address_t, std::vector<std::shared_ptr<xmock_node_t>>> m_addr_group;
    std::map<std::string, std::shared_ptr<xmock_node_t>> m_name_nodes;
    std::map<std::string, std::vector<std::shared_ptr<xmock_node_t>>> m_name_group;
};

class simple_timer : public base::xxtimer_t {
public:
    simple_timer(base::xcontext_t &_context, int32_t timer_thread_id, xmock_system_t *service)
    : base::xxtimer_t(_context, timer_thread_id) {
        m_service = service;
    }

protected:
    virtual ~simple_timer() {
    }

protected:
    virtual bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override
    {
        //printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
        m_service->on_timer();
        return true;
    }

    xmock_system_t *m_service;
};

}
}
