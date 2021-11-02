// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "xunit_service/xtimer_dispatcher.h"
#include "xunit_service/xcons_utl.h"

NS_BEG2(top, xunit_service)

xtimer_dispatcher_t::xtimer_dispatcher_t(std::shared_ptr<xcons_service_para_face> const & para, std::shared_ptr<xblock_maker_face> const & block_maker)
  : xcons_dispatcher(e_timer), m_para(para), m_block_maker(block_maker) {
    static uint32_t tid = 0;
    auto wp = m_para->get_resources()->get_workpool();
    auto th = wp->get_thread((++tid) % wp->get_count());
    m_picker = new xtimer_picker_t(th->get_context(), th->get_thread_id(), m_para, m_block_maker);
    xunit_info("xtimer_dispatcher_t::xtimer_dispatcher_t,create,this=%p,pick_refcount=%d", this, m_picker->get_refcount());
}

xtimer_dispatcher_t::~xtimer_dispatcher_t() {
    xunit_info("xtimer_dispatcher_t::~xtimer_dispatcher_t,destroy,this=%p", this);
    xassert(m_picker == nullptr);
}

bool xtimer_dispatcher_t::dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to) {
    return async_dispatch(pdu, xip_from, xip_to, (xconsensus::xcsaccount_t *)m_picker) == 0;
}

bool xtimer_dispatcher_t::start(const xvip2_t & xip, const common::xlogic_time_t& start_time) {
    xunit_info("xtimer_dispatcher_t::start %s %p start", xcons_utl::xip_to_hex(xip).c_str(), this);
    auto async_reset = [xip](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xtimer_picker_t *>(call.get_param1().get_object());
        packer->reset_xip_addr(xip);
        xunit_info("[xtimer_dispatcher_t::start] with xip {%" PRIx64 ", %" PRIx64 "} done", xip.high_addr, xip.low_addr);
        return true;
    };
    base::xcall_t asyn_call(async_reset, (xconsensus::xcsaccount_t *)m_picker);
    ((xconsensus::xcsaccount_t *)m_picker)->send_call(asyn_call);
    return true;
}

bool xtimer_dispatcher_t::fade(const xvip2_t & xip) {
    xunit_info("xtimer_dispatcher_t::fade %s %p fade", xcons_utl::xip_to_hex(xip).c_str(), this);
    auto async_reset = [xip](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xtimer_picker_t *>(call.get_param1().get_object());
        packer->set_fade_xip_addr(xip);
        xunit_info("[xtimer_dispatcher_t::fade] with xip {%" PRIx64 ", %" PRIx64 "} done", xip.high_addr, xip.low_addr);
        return true;
    };
    base::xcall_t asyn_call(async_reset, (xconsensus::xcsaccount_t *)m_picker);
    ((xconsensus::xcsaccount_t *)m_picker)->send_call(asyn_call);
    return true;
}

bool xtimer_dispatcher_t::unreg(const xvip2_t & xip) {
    xunit_info("xtimer_dispatcher_t::unreg %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    return true;
}

bool xtimer_dispatcher_t::destroy(const xvip2_t & xip) {
    xunit_info("xtimer_dispatcher_t::destroy %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    if (m_picker != nullptr) {
        m_picker->close();
        m_picker->release_ref();
        m_picker = nullptr;
    }
    return true;
}

NS_END2
