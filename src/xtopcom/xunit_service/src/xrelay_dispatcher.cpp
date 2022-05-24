// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xrelay_dispatcher.h"

#include "xdata/xnative_contract_address.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xrelay_packer.h"

#include <inttypes.h>

NS_BEG2(top, xunit_service)

xrelay_dispatcher_t::xrelay_dispatcher_t(std::shared_ptr<xcons_service_para_face> const & para, std::shared_ptr<xblock_maker_face> const & block_maker)
  : xcons_dispatcher(e_timer), m_para(para), m_block_maker(block_maker) {
    static uint32_t tid = 0;
    auto wp = m_para->get_resources()->get_workpool();
    m_worker = wp->get_thread((++tid) % wp->get_count());
    // m_packer = new xrelay_packer(th->get_context(), th->get_thread_id(), m_para, m_block_maker);
    m_packer = new xrelay_packer(sys_contract_relay_table_block_addr, m_para, m_block_maker, base::xcontext_t::instance(), m_worker->get_thread_id());
    xunit_info("xrelay_dispatcher_t::xrelay_dispatcher_t,create,this=%p,pick_refcount=%d", this, m_packer->get_refcount());
}

xrelay_dispatcher_t::~xrelay_dispatcher_t() {
    xunit_info("xrelay_dispatcher_t::~xrelay_dispatcher_t,destroy,this=%p", this);
    xassert(m_packer == nullptr);
}

bool xrelay_dispatcher_t::dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to) {
    xunit_info("xrelay_dispatcher_t::dispatch,this=%p", this);
    xrelay_packer * packer = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_packer == nullptr) {
            return false;
        } else {
            packer = m_packer;
        }
    }

    return async_dispatch(pdu, xip_from, xip_to, (xconsensus::xcsaccount_t *)packer) == 0;
}

void xrelay_dispatcher_t::chain_timer(common::xlogic_time_t time) {
    assert(m_para);

    auto * blkstore = m_para->get_resources()->get_vblockstore();
    auto timer_block = blkstore->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_us_dispatcher_load_tc);

    auto clock = timer_block->get_height();

    xunit_dbg("xrelay_dispatcher_t::chain_timer this:%p logic time %" PRIu64 " TC %" PRIu64, this, time, clock);
    if (time <= clock) {
        xunit_dbg("xrelay_dispatcher_t::chain_timer call on_clock, this:%p logic time %" PRIu64 " TC %" PRIu64, this, time, clock);
        on_clock(timer_block.get());
    }
}

void xrelay_dispatcher_t::on_clock(base::xvblock_t * clock_block) {
    xrelay_packer * packer = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_packer == nullptr) {
            return;
        }
        packer = m_packer;
    }

    xunit_dbg("xrelay_dispatcher_t::on_clock this:%p TC %" PRIu64, this, clock_block->get_height());

    auto _call = [](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xrelay_packer *>(call.get_param1().get_object());
        auto block_ptr = dynamic_cast<base::xvblock_t *>(call.get_param2().get_object());
        packer->fire_clock(*block_ptr, 0, 0);
        return true;
    };
    base::xcall_t asyn_call((base::xcallback_t)_call, packer, clock_block);
    auto ret = m_worker->send_call(asyn_call);
    { xunit_dbg("xrelay_dispatcher_t::on_clock ret:%d TC: %" PRIu64, ret, clock_block->get_height()); }
}

std::string relay_watcher_name(const xvip2_t & xip) {
    return std::string("relay_dispatch_timer").append("_").append(std::to_string(xip.low_addr));
}

bool xrelay_dispatcher_t::start(const xvip2_t & xip, const common::xlogic_time_t & start_time) {
    xunit_info("xrelay_dispatcher_t::start %s %p start", xcons_utl::xip_to_hex(xip).c_str(), this);
    xrelay_packer * packer = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_packer == nullptr) {
            return false;
        }
        packer = m_packer;
    }

    auto async_reset = [xip, start_time](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xrelay_packer *>(call.get_param1().get_object());
        packer->set_start_time(start_time);
        packer->reset_xip_addr(xip);
        xunit_info("[xrelay_dispatcher_t::start] with xip {%" PRIx64 ", %" PRIx64 "} done", xip.high_addr, xip.low_addr);
        return true;
    };

    base::xcall_t asyn_call(async_reset, (xconsensus::xcsaccount_t *)packer);
    ((xconsensus::xcsaccount_t *)packer)->send_call(asyn_call);

    m_watcher_name = relay_watcher_name(xip);
    m_para->get_resources()->get_chain_timer()->watch(m_watcher_name, 1, std::bind(&xrelay_dispatcher_t::chain_timer, shared_from_this(), std::placeholders::_1));

    return true;
}

bool xrelay_dispatcher_t::fade(const xvip2_t & xip) {
    xrelay_packer * packer = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_packer == nullptr) {
            return false;
        }
        packer = m_packer;
    }
    xunit_info("xrelay_dispatcher_t::fade %s %p fade", xcons_utl::xip_to_hex(xip).c_str(), this);
    auto async_reset = [xip](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xrelay_packer *>(call.get_param1().get_object());
        packer->set_fade_xip_addr(xip);
        xunit_info("[xrelay_dispatcher_t::fade] with xip {%" PRIx64 ", %" PRIx64 "} done", xip.high_addr, xip.low_addr);
        return true;
    };
    base::xcall_t asyn_call(async_reset, (xconsensus::xcsaccount_t *)packer);
    ((xconsensus::xcsaccount_t *)packer)->send_call(asyn_call);
    return true;
}

bool xrelay_dispatcher_t::unreg(const xvip2_t & xip) {
    xunit_info("xrelay_dispatcher_t::unreg %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->unwatch(relay_watcher_name(xip));
    return true;
}

bool xrelay_dispatcher_t::destroy(const xvip2_t & xip) {
    xunit_info("xrelay_dispatcher_t::destroy %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->unwatch(m_watcher_name);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_packer != nullptr) {
        m_packer->close();
        m_packer->release_ref();
        m_packer = nullptr;
    }
    return true;
}

NS_END2
