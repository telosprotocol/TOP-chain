// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xcons_serivce.h"
#include "xunit_service/xcons_utl.h"

#include <cinttypes>

NS_BEG2(top, xunit_service)

xcons_service_t::xcons_service_t(std::shared_ptr<xcons_service_para_face> const & p_para,
                                 std::shared_ptr<xcons_dispatcher> const &        dispatcher,
                                 common::xmessage_category_t                      category)
  : m_para(p_para), m_dispatcher(dispatcher), m_category(category) {
    xunit_dbg("xcons_service_t::xcons_service_t,create,this=%p", this);
}

xcons_service_t::~xcons_service_t() {
    xunit_dbg("xcons_service_t::~xcons_service_t,destroy,this=%p", this);
}

common::xmessage_category_t xcons_service_t::get_msg_category() {
    return m_category;
}

bool xcons_service_t::start(const xvip2_t & xip, const common::xlogic_time_t& start_time) {
    // TODO(justin): add start implement
    // 1. get elect data from election data
    // 2. dispatcher subscribe xip[noversion]: tableids
    //   2.1 reset xip
    xkinfo("xcons_service_t::start %s this=%p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_dispatcher->start(xip, start_time);
    // 3. register network proxy notification
    auto network_proxy = m_para->get_resources()->get_network();
    network_proxy->listen(xip, get_msg_category(), shared_from_this());
    m_running = true;
    return m_running;
}

bool xcons_service_t::fade(const xvip2_t & xip) {
    return m_dispatcher->fade(xip);
}

bool xcons_service_t::unreg(const xvip2_t & xip) {
    // TODO(justin): add fade implement
    // 1. get elect data from election data
    // 2. unregister network proxy
    xunit_info("xcons_service_t::unreg %s this=%p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_dispatcher->unreg(xip);
    auto network_proxy = m_para->get_resources()->get_network();
    network_proxy->unlisten(xip, get_msg_category());
    // m_running = false;
    return true;
}

bool xcons_service_t::destroy(const xvip2_t & xip) {
    xunit_info("xcons_service_t::destroy %s this=%p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_dispatcher->destroy(xip);
    m_running = false;
    return !m_running;
}

void xcons_service_t::on_pdu(const xvip2_t & xip_from, const xvip2_t & xip_to, const base::xcspdu_t & packet) {
    xunit_dbg("xcons_service_t::on_pdu consrv_status:%d,pdu=%s,at_node:%s", m_running, packet.dump().c_str(), xcons_utl::xip_to_hex(xip_to).c_str());

    if (m_running) {
        auto pdu = (base::xcspdu_t *)(&packet);
        auto ret = m_dispatcher->dispatch(m_para->get_resources()->get_workpool(), pdu, xip_from, xip_to);
        if (!ret) {
            xunit_warn("xcons_service_t::on_pdu fail-dispatch msg. consrv=%p,pdu=%s,at_node:%s",
                  this, packet.dump().c_str(), xcons_utl::xip_to_hex(xip_to).c_str());
        }
    } else {
        xunit_warn("xcons_service_t::on_pdu fail-discard msg for uninited. consrv=%p,pdu=%s,at_node:%s",
                this, packet.dump().c_str(), xcons_utl::xip_to_hex(xip_to).c_str());
    }
}

bool xcons_service_t::is_running() {
    return m_running;
}

NS_END2
