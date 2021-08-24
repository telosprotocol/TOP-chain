// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xunit_service/xcons_service_mgr.h"

#include "xcommon/xnode_type.h"
#include "xunit_service/xcons_proxy.h"
#include "xunit_service/xleader_election.h"
#include "xunit_service/xtableblockservice.h"
#include "xunit_service/xtimer_service.h"

#include <cinttypes>

NS_BEG2(top, xunit_service)

xcons_service_mgr::xcons_service_mgr(observer_ptr<mbus::xmessage_bus_face_t> const    &mb,
                                     const std::shared_ptr<xnetwork_proxy_face> &     network_proxy,
                                     const xcons_dispatcher_builder_ptr &             dispatcher_builder,
                                     const std::shared_ptr<xcons_service_para_face> & para)
  : m_mbus(mb), m_dispachter_builder(dispatcher_builder), m_network_proxy(network_proxy), m_para(para) {
    xunit_dbg("xcons_service_mgr::xcons_service_mgr,create,this=%p", this);
}

xcons_service_mgr::~xcons_service_mgr() {
    xunit_dbg("xcons_service_mgr::~xcons_service_mgr,destroy,this=%p", this);
}

// create consensus proxy by networkdriver
xcons_proxy_face_ptr xcons_service_mgr::create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network) {
    auto xip = xcons_utl::to_xip2(network->address(), true);
    auto key_ = xcons_utl::erase_version(xip);
    xassert(m_network_proxy != nullptr);
    // add network first
    m_network_proxy->add(network);
    // auto pelection = m_para->get_resources()->get_election()->get_election_cache_face();
    // auto accessor = m_para->get_resources()->get_data_accessor();
    // auto now = m_para->get_resources()->get_chain_timer()->logic_time();
    // xelection_wrapper::on_network_start(pelection, xip, network, accessor, now);
    {
        // reuse consensus service for one address without version
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        iter = m_cons_map.find(key_);
        if (iter != m_cons_map.end()) {
            xkinfo(" [xunitservice] consrv_mgr existed consensus proxy %s, key:%s, addr:%s", xcons_utl::xip_to_hex(xip).c_str(), xcons_utl::xip_to_hex(key_).c_str(), network->address().to_string().c_str());
            return std::make_shared<xcons_proxy>(xip, this->shared_from_this());
        }
    }

    // create consensus service for new address without version
    auto                                             node_type = network->type();
    std::vector<std::shared_ptr<xcons_service_face>> services;
    if ((node_type & common::xnode_type_t::rec) == common::xnode_type_t::rec) {
        auto dispatcher = m_dispachter_builder->build(m_mbus, m_para, e_timer);
        xunit_dbg("[xcons_service_mgr::create] create timer service for rec, {%" PRIu64 ", %" PRIu64 "}, dispatcher obj %p", xip.high_addr, xip.low_addr, dispatcher.get());
        if (dispatcher != nullptr) {
            std::shared_ptr<xcons_service_face> timer_service = std::make_shared<xtimer_service_t>(m_para, dispatcher);
            services.push_back(timer_service);
        }
    }

    auto dispatcher = m_dispachter_builder->build(m_mbus, m_para, e_table);
    if (dispatcher != nullptr) {
        std::shared_ptr<xcons_service_face> table_service = std::make_shared<xtableblockservice>(m_para, dispatcher);
        services.push_back(table_service);
    }

    xassert(!services.empty());

    {
        // store to map
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cons_map[key_] = services;
    }
    xkinfo(" [xunitservice] consrv_mgr create consensus proxy %s addr:%s key:%s", xcons_utl::xip_to_hex(xip).c_str(), network->address().to_string().c_str(), xcons_utl::xip_to_hex(key_).c_str());
    return std::make_shared<xcons_proxy>(xip, this->shared_from_this());
}

// destroy useless cons services by networkdriver, call by vnode manager while detemine some service useless
// must call uninit before
bool xcons_service_mgr::destroy(const xvip2_t & xip) {
    auto key_ = xcons_utl::erase_version(xip);
    xunit_info("xcons_service_mgr::destroy %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    std::vector<std::shared_ptr<xcons_service_face>> services;
    {
        // erase useless consensus service
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        iter = m_cons_map.find(key_);
        if (iter != m_cons_map.end()) {
            services.insert(services.begin(), iter->second.begin(), iter->second.end());
            m_cons_map.erase(iter);
        }
    }

    // destroy all reference service
    if (!services.empty()) {
        for (auto service : services) {
            xunit_dbg("xcons_service_mgr::destroy destroy service %s", xcons_utl::xip_to_hex(xip).c_str());
            service->fade(xip);
            service->destroy(xip);
        }
        services.clear();
    } else {
        xunit_info("xcons_service_mgr::destroy xip not found %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    }
    // erase reference network
    m_network_proxy->erase(xip);
    auto pelection = m_para->get_resources()->get_election()->get_election_cache_face();
    xelection_wrapper::on_network_destory(pelection, xip);
    return true;
}

bool xcons_service_mgr::find(const xvip2_t & xip, std::vector<std::shared_ptr<xcons_service_face>> * p_services) {
    {
        // erase useless consensus service
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        iter = m_cons_map.find(xip);
        if (iter != m_cons_map.end()) {
            p_services->insert(p_services->begin(), iter->second.begin(), iter->second.end());
        }
        return p_services->empty();
    }
}

// init consensus service
bool xcons_service_mgr::start(const xvip2_t & xip, const common::xlogic_time_t& start_time) {
    auto pelection = m_para->get_resources()->get_election()->get_election_cache_face();
    auto accessor = m_para->get_resources()->get_data_accessor();
    auto now = m_para->get_resources()->get_chain_timer()->logic_time();
    auto network = m_network_proxy->find(xip);
    if (!xelection_wrapper::on_network_start(pelection, xip, network, accessor, now)) {
        m_network_proxy->erase(xip);
        return false;
    }
    auto                                             key_ = xcons_utl::erase_version(xip);
    xkinfo(" [xunitservice] consrv_mgr start consensus proxy:%s, key:%s", xcons_utl::xip_to_hex(xip).c_str(), xcons_utl::xip_to_hex(key_).c_str()) ;
    std::vector<std::shared_ptr<xcons_service_face>> services;
    // destroy all reference service
    if (!find(key_, &services)) {
        for (auto service : services) {
            service->start(xip, start_time);
        }
        return true;
    }
    return false;
}

// uninit data
bool xcons_service_mgr::fade(const xvip2_t & xip) {
    xkinfo(" [xunitservice] consrv_mgr fade consensus proxy %s", xcons_utl::xip_to_hex(xip).c_str());
    auto                                             key_ = xcons_utl::erase_version(xip);
    std::vector<std::shared_ptr<xcons_service_face>> services;
    // destroy all reference service
    if (!find(key_, &services)) {
        for (auto service : services) {
            service->fade(xip);
        }
        // erase reference network
        m_network_proxy->erase(xip);
        return true;
    }
    return false;
}

NS_END2
