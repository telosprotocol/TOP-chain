// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xcons_proxy.h"
#include "xunit_service/xcons_utl.h"

#include <cinttypes>
#include <vector>

NS_BEG2(top, xunit_service)

// default block service entry
xcons_proxy::xcons_proxy(const xvip2_t & xip, const std::shared_ptr<xcons_service_mgr_face> & cons_mgr) : m_cons_mgr(cons_mgr), m_xip(xip) {
    #ifdef DEBUG
    xdbg("[xunitservice] cons_proxy create %p addr:%s", this, xcons_utl::xip_to_hex(xip).c_str());
    #endif
}

// vnode start
bool xcons_proxy::start() {
    xkinfo("[xunitservice] cons_proxy start %p addr:%s", this, xcons_utl::xip_to_hex(m_xip).c_str());
    return m_cons_mgr->start(m_xip);
}

// vnode fade
bool xcons_proxy::fade() {
    xkinfo("[xunitservice] fade cons_proxy %p addr:%s", this, xcons_utl::xip_to_hex(m_xip).c_str());
    return m_cons_mgr->fade(m_xip);
}

// vnode outdated
bool xcons_proxy::outdated() {
    xkinfo("[xunitservice] outdated cons_proxy %p addr:%s", this, xcons_utl::xip_to_hex(m_xip).c_str());
    return true;
}

xvip2_t xcons_proxy::get_ip() {
    return m_xip;
}

NS_END2
