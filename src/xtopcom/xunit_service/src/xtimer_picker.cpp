// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xtimer_picker.h"

#include "xdata/xemptyblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xunit_service/xcons_utl.h"
#include "xcommon/xmessage_id.h"
#include "xBFT/src/xtimercertview.h"
#include "xmbus/xevent_timer.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xblockbuild.h"
#include "xelection/xdata_accessor_error.h"
#include "xcertauth/src/xsigndata.h"

#include <inttypes.h>

NS_BEG2(top, xunit_service)

xtimer_picker_t::xtimer_picker_t(base::xcontext_t &                               _context,
                                 const int32_t                                    target_thread_id,
                                 std::shared_ptr<xcons_service_para_face> const & para,
                                 std::shared_ptr<xblock_maker_face> const &       block_maker)
  : xconsensus::xcsaccount_t(_context, target_thread_id, sys_drand_addr)
  , m_params(para)
  , m_leader_selector(para->get_resources()->get_election())
  , m_block_maker(block_maker)
  , m_bus(para->get_resources()->get_bus()) {
    auto cert_auth = para->get_resources()->get_certauth();
    xconsensus::xcsaccount_t::register_plugin(cert_auth);
    auto store = para->get_resources()->get_vblockstore();
    set_vblockstore(store);
    xconsensus::xcsaccount_t::register_plugin(store);
    base::xauto_ptr<xcscoreobj_t> auto_engine = create_engine(*this, xconsensus::enum_xconsensus_pacemaker_type_timeout_cert);

    // init view
    auto last_block = get_vblockstore()->get_latest_cert_block(get_account(), metrics::blockstore_access_from_us_timer_picker_constructor);
    assert(last_block != nullptr);

    m_cur_view = last_block->get_viewid();

    // get latest cert clock for drop old tc timeout msg
    base::xvaccount_t _timer_vaddress(sys_contract_beacon_timer_addr);
    auto last_clock_blk = get_vblockstore()->get_latest_cert_block(_timer_vaddress, metrics::blockstore_access_from_us_timer_picker_constructor);
    assert(last_clock_blk != nullptr);
    m_latest_cert_clock = last_clock_blk->get_clock();
    xunit_info("xtimer_picker_t::xtimer_picker_t,create,this=%p,%s,engine_refcount=%d,latest_cert_clock=%" PRIu64 , this, last_block->dump().c_str(), auto_engine->get_refcount(), m_latest_cert_clock);
}

xtimer_picker_t::~xtimer_picker_t() {
    xunit_info("xtimer_picker_t::~xtimer_picker_t,destroy,this=%p", this);
}

bool xtimer_picker_t::set_fade_xip_addr(const xvip2_t & new_addr) {
    xdbg("xtimer_picker_t::set_fade_xip_addr set fade xip from %s to %s", xcons_utl::xip_to_hex(m_faded_xip2).c_str(), xcons_utl::xip_to_hex(new_addr).c_str());
    m_faded_xip2 = new_addr;
    return true;
}

bool xtimer_picker_t::on_view_fire(const base::xvevent_t & event, xconsensus::xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    if (xcons_utl::xip_equals(m_faded_xip2, get_xip2_addr())) {
        xdbg_info("xtimer_picker_t::on_view_fire local_xip equal m_fade_xip2 %s . fade round should not make proposal", xcons_utl::xip_to_hex(m_faded_xip2).c_str());
        return false;
    }

    auto const & view_event = (xconsensus::xcsview_fire const &)event;
    if(m_cur_view < view_event.get_viewid()) {
        m_cur_view = view_event.get_viewid();
        auto local_xip = get_xip2_addr();
        common::xelection_round_t version{0};
        xvip2_t leader_xip = m_leader_selector->get_leader_xip(m_cur_view, get_account(), nullptr, local_xip, local_xip, version, enum_rotate_mode_no_rotate);
        if (xcons_utl::xip_equals(leader_xip, local_xip)) {
            auto blk = m_block_maker->make_block(get_account(), view_event.get_clock(), m_cur_view, 0, get_xip2_addr());
            if (blk == nullptr) {
                return false;
            }
            base::xauto_ptr<base::xvblock_t> proposal_block(blk);
            xunit_info("[xtimer_picker_t::on_timer_fire] newview(leader) proposal self %" PRIx64 ", view %" PRIu64 ", height %" PRIu64 ", cur_view %" PRIu64 "\n",
                get_xip2_low_addr(),
                proposal_block->get_viewid(),
                proposal_block->get_height(),
                m_cur_view);
            xunit_info("[xtimer_picker_t::on_timer_fire] newview(leader) proposal node=%" PRIx64 ",proposal=%s",
                get_xip2_low_addr(), proposal_block->dump().c_str());

            base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
            push_event_down(*_event_obj, this, 0, 0);
        } else {
            xunit_info("[xtimer_picker_t::on_timer_fire] newview(backup) cur_view %" PRIu64, m_cur_view);
        }
    }
    return xconsensus::xcsaccount_t::on_view_fire(event, from_parent, cur_thread_id, timenow_ms);
}

bool xtimer_picker_t::on_pdu_event_up(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xconsensus::xcspdu_fire const& _evt_obj = (xconsensus::xcspdu_fire const&)event;
    auto local_xip = get_xip2_addr();
    if (xcons_utl::is_broadcast_address(_evt_obj.get_to_xip())) {
        // leader broadcast msg
        auto to_xip = local_xip;
        set_node_id_to_xip2(to_xip, common::xbroadcast_slot_id_value);
        xunit_dbg("[xtimer_picker_t::on_pdu_event_up] broadcast from {%" PRIx64 ", %" PRIx64 "} to {%" PRIx64 ", %" PRIx64 "}", local_xip.high_addr, local_xip.low_addr, to_xip.high_addr, to_xip.low_addr);
        return send_out(local_xip, to_xip, _evt_obj._packet, cur_thread_id, timenow_ms);
    } else {
        xunit_dbg("[xtimer_picker_t::on_pdu_event_up] from {%" PRIx64 ", %" PRIx64 "} to {%" PRIx64 ", %" PRIx64 "}", local_xip.high_addr, local_xip.low_addr, _evt_obj.get_to_xip().high_addr, _evt_obj.get_to_xip().low_addr);
        send_out(local_xip, _evt_obj.get_to_xip(), _evt_obj._packet, cur_thread_id, timenow_ms);
    }
    return true;  // stop here
}

// create tc block for timecertview
bool xtimer_picker_t::on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) {
    xunit_dbg("[timer_picker::on_create_block_event]");
    xconsensus::xcscreate_block_evt const & e = (xconsensus::xcscreate_block_evt const &)event;
    auto clock = e.get_clock();
    auto context_id = e.get_context_id();

    // TODO(jimmy) move to tc pacemaker directly
    data::xemptyblock_build_t bbuild(sys_contract_beacon_timer_addr, clock);
    base::xauto_ptr<base::xvblock_t> _block = bbuild.build_new_block();

    // base::xauto_ptr<base::xvblock_t> _block = data::xemptyblock_t::create_emptyblock(sys_contract_beacon_timer_addr, clock, base::enum_xvblock_level_root, clock, clock, base::enum_xvblock_type_clock);
    // _block->get_cert()->set_viewtoken(-1);
    // _block->get_cert()->set_drand(-1);
    // _block->get_cert()->set_nonce(-1);

    base::xauto_ptr<xconsensus::xcscreate_block_evt> _event_obj(new xconsensus::xcscreate_block_evt(e.get_xip(), e.get_vote(), _block.get(), context_id));
    get_child_node()->push_event_down(*_event_obj, this, 0, 0);

    return true;  // stop here
}

bool xtimer_picker_t::check_first_round_tc_broadcast_leader(base::xvblock_t* tc_block) {
    auto local_xip = get_xip2_addr();
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto accessor = m_params->get_resources()->get_data_accessor();
    auto election_epoch = accessor->election_epoch_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    if (ec) {
        xunit_warn("xtimer_picker_t::check_first_round_tc_broadcast_leader xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }

    xvip2_t leader_xip = m_leader_selector->get_leader_xip(tc_block->get_viewid(), get_account(), nullptr, local_xip, local_xip, election_epoch, enum_rotate_mode_no_rotate);
    if (xcons_utl::xip_equals(leader_xip, local_xip)) {
        xdbg("xtimer_picker_t::check_first_round_tc_broadcast_leader selected.height=%ld,leader_xip=%s",tc_block->get_height(),xcons_utl::xip_to_hex(local_xip).c_str());
        return true;
    } else {
        xdbg("xtimer_picker_t::check_first_round_tc_broadcast_leader not me.height=%ld,leader_xip=%s,local_xip=%s",tc_block->get_height(),xcons_utl::xip_to_hex(leader_xip).c_str(),xcons_utl::xip_to_hex(local_xip).c_str());
    }
    return false;
}

bool xtimer_picker_t::check_second_round_tc_broadcast_leader(base::xvblock_t* tc_block) {
    base::xvaccount_t tc_account(sys_contract_beacon_timer_addr);
    base::xauto_ptr<base::xvblock_t> last_tc = get_vblockstore()->get_latest_cert_block(tc_account);
    if (nullptr == last_tc || last_tc->get_height() == 0) {
        return false;
    }

    if (last_tc->get_height() >= tc_block->get_height()) {
        xdbg("xtimer_picker_t::second_round_tc_broadcast already in db,dbtc_height=%ld,newtc_height=%ld",last_tc->get_height(),tc_block->get_height());
        return false;
    }

    auto local_xip = get_xip2_addr();
    if (!tc_block->get_cert()->is_validator(local_xip)) {
        xwarn("xtimer_picker_t::second_round_tc_broadcast xip not in same group.height=%ld,tc_leader=%s,local_xip=%s", tc_block->get_height(), xcons_utl::xip_to_hex(last_tc->get_cert()->get_validator()).c_str(), xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }

    const std::string& verify_sig = last_tc->get_cert()->get_verify_signature();
    top::auth::xmutisigdata_t aggregated_sig_obj;
    if (aggregated_sig_obj.serialize_from_string(verify_sig) < 0) {
        xerror("xtimer_picker_t::second_round_tc_broadcast serialize signature fail.size=%zu", verify_sig.size());
        return false;
    }

    int local_xip_slot = get_node_id_from_xip2(local_xip);
    int slot_offset = tc_block->get_height();
    top::auth::xnodebitset& nodebits = aggregated_sig_obj.get_nodebitset();
    if (local_xip_slot >= nodebits.get_alloc_bits()) {
        xerror("xtimer_picker_t::second_round_tc_broadcast fail-local xip slot too large.local_xip_slot=%d,bits=%d",local_xip_slot,nodebits.get_alloc_bits());
        return false;
    }

    for(int i = 0; i < nodebits.get_alloc_bits(); ++i) {
        int slot = (i + slot_offset) % nodebits.get_alloc_bits();
        xassert(slot < nodebits.get_alloc_bits());
        if (nodebits.is_set(slot)) {  // find first online slot as leader node to broadcast
            if (local_xip_slot == slot) {
                xdbg("xtimer_picker_t::second_round_tc_broadcast selected.height=%ld,leader_slot=%d,local_slot=%d,bits=%d",tc_block->get_height(),slot,local_xip_slot,nodebits.get_alloc_bits());
                return true;
            } else {
                xdbg("xtimer_picker_t::second_round_tc_broadcast not me.height=%ld,leader_slot=%d,local_slot=%d",tc_block->get_height(),slot,local_xip_slot);
                return false;
            }
        } else {
            xdbg("xtimer_picker_t::second_round_tc_broadcast bit not set.height=%ld,slot=%d",tc_block->get_height(),slot);
        }
    }
    xerror("xtimer_picker_t::second_round_tc_broadcast not find slot.height=%ld",tc_block->get_height());
    return false;
}

// leader broadcast tc block to all nodes
bool  xtimer_picker_t::on_time_cert_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
{
    auto local_xip = get_xip2_addr();
    xconsensus::xcstc_fire const & e = (xconsensus::xcstc_fire const &)event;
    auto tc_block = e.get_tc_block();
    m_latest_cert_clock = tc_block->get_clock();

    bool is_broadcast_leader = false;
    if (e.get_broadcast_round() == 0) {  // first broadcast round elect leader
        is_broadcast_leader = check_first_round_tc_broadcast_leader(tc_block);
    }
    if (e.get_broadcast_round() == 1) {  //second broadcast round elect leader
        is_broadcast_leader = check_second_round_tc_broadcast_leader(tc_block);
    }

    xdbg("xtimer_picker_t::on_time_cert_event height=%ld,round=%d,is_leader=%d", m_latest_cert_clock, e.get_broadcast_round(),is_broadcast_leader);

    if (is_broadcast_leader) {
        auto    network_proxy = m_params->get_resources()->get_network();
        if (network_proxy != nullptr) {
            xvip2_t to_addr{(uint64_t)-1, (uint64_t)-1};  // broadcast to all
            xunit_info("[xtimer_picker_t::on_time_cert_event] broadcast to all nodes,round=%d,height=%ld,xip=%s",
                e.get_broadcast_round(),tc_block->get_height(),xcons_utl::xip_to_hex(local_xip).c_str());
            network_proxy->send_out(xmessage_block_broadcast_id, local_xip, to_addr, tc_block);
            XMETRICS_GAUGE_SET_VALUE(metrics::clock_leader_broadcast_height, tc_block->get_height());
        }
    }
    return true;  // stop here
}

bool xtimer_picker_t::send_out(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    auto network_proxy = m_params->get_resources()->get_network();
    if (network_proxy != nullptr) {
        xunit_dbg("[timer_picker] sendout src %" PRIx64 ".%" PRIx64 " dst %" PRIx64 ".%" PRIx64, from_addr.low_addr, from_addr.high_addr, to_addr.low_addr, to_addr.high_addr);
        return network_proxy->send_out((uint32_t)xTimer_msg, from_addr, to_addr, packet, cur_thread_id, timenow_ms);
    }
    return true;
}

bool xtimer_picker_t::recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    auto local_xip = get_xip2_addr();
    auto type = packet.get_msg_type();
    // reject for local msg && mismatch dest xip
    // fix TOP-3720 & TOP-3743. 
    // XIP's network version is deprecated now and will be used for other purpose in the future.
    // so for consensusing logic time, only epoch matched msg can be processed.
    // Epoch is defined in XIP's high part.
    if (xcons_utl::xip_equals(from_addr, local_xip) 
        || !xcons_utl::xip_equals(to_addr, local_xip)
        || (from_addr.high_addr != to_addr.high_addr)) {
        xunit_warn("[xtimer_picker_t::recv_in] recv invalid msg %x from:%s to:%s at node=%s",
              type,
              xcons_utl::xip_to_hex(from_addr).c_str(),
              xcons_utl::xip_to_hex(to_addr).c_str(),
              xcons_utl::xip_to_hex(local_xip).c_str());
        return true;
    }

    bool valid = true;
    common::xelection_round_t version{0};
    xvip2_t leader_xip;
    if (type == xconsensus::enum_consensus_msg_type_proposal ||
        type == xconsensus::enum_consensus_msg_type_proposal_v2 ||
        type == xconsensus::enum_consensus_msg_type_commit) {
        // check if sender is leader
        leader_xip = m_leader_selector->get_leader_xip(packet.get_block_viewid(), get_account(), nullptr, get_xip2_addr(), from_addr, version, enum_rotate_mode_no_rotate);
        valid = xcons_utl::xip_equals(leader_xip, from_addr);
    } else if(type == xconsensus::enum_consensus_msg_type_vote) {
        // check if I am leader
        leader_xip = m_leader_selector->get_leader_xip(packet.get_block_viewid(), get_account(), nullptr, local_xip, local_xip, version, enum_rotate_mode_no_rotate);
        valid = xcons_utl::xip_equals(leader_xip, local_xip);
    } else if (type == xconsensus::enum_consensus_msg_type_timeout) {
        if (packet.get_block_clock() <= m_latest_cert_clock) {
            xunit_dbg("xtimer_picker_t::recv_in enum_consensus_msg_type_timeout packet clock is old.clock %" PRIx64 ":%" PRIx64, packet.get_block_clock(), m_latest_cert_clock);
            return false;
        }
    }
    if (valid) {
        return xcsaccount_t::recv_in(from_addr, to_addr, packet, cur_thread_id, timenow_ms);
    } else {
        xunit_warn("[xtimer_picker_t::recv_in] recv invalid msg %x from %" PRIx64 ":%" PRIx64, type, from_addr.high_addr, from_addr.low_addr);
        return valid;
    }
}

int xtimer_picker_t::verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) {
    return m_block_maker->verify_block(proposal_block);
}

bool xtimer_picker_t::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xconsensus::xcsaccount_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful) {
        auto high_qc = _evt_obj->get_target_proposal();

        XMETRICS_GAUGE_SET_VALUE(metrics::cons_drand_highqc_height, high_qc->get_height());

        xunit_info("xtimer_picker_t::on_proposal_finish succ. leader:%d,proposal=%s", is_leader, high_qc->dump().c_str());
        if (is_leader) {
            // TODO(jimmy) leader broadcast to all nodes, should take more nodes
            xvip2_t to_addr{(uint64_t)-1, (uint64_t)-1};
            auto    network_proxy = m_params->get_resources()->get_network();
            xassert(network_proxy != nullptr);
            if (network_proxy != nullptr) {
                network_proxy->send_out(xmessage_block_broadcast_id, get_xip2_addr(), to_addr, high_qc);
            }
        }
    } else {
        xunit_warn("xtimer_picker_t::on_proposal_finish fail. leader:%d,error_code:%d,proposal=%s",
            is_leader,
            _evt_obj->get_error_code(),
            _evt_obj->get_target_proposal()->dump().c_str());
    }
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

NS_END2
