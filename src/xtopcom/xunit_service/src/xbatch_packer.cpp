// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xbatch_packer.h"

#include "xBFT/xconsevent.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_type.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xblocktool.h"
#include "xelection/xdata_accessor_error.h"
#include "xunit_service/xcons_utl.h"
#include "xmbus/xevent_consensus.h"
#include "xmbus/xevent_behind.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

#define CONFIRM_DELAY_TOO_MUCH_TIME (20)

xbatch_packer::xbatch_packer(observer_ptr<mbus::xmessage_bus_face_t> const   &mb,
                             uint16_t                                         tableid,
                             const std::string &                              account_id,
                             std::shared_ptr<xcons_service_para_face> const & para,
                             std::shared_ptr<xblock_maker_face> const &       block_maker,
                             base::xcontext_t &                               _context,
                             const uint32_t                                   target_thread_id)
  : xcsaccount_t(_context, target_thread_id, account_id), m_mbus(mb), m_tableid(tableid), m_last_view_id(0), m_para(para), m_block_maker(block_maker), m_account_id(account_id) {
    auto cert_auth = m_para->get_resources()->get_certauth();
    register_plugin(cert_auth);
    auto store = m_para->get_resources()->get_vblockstore();
    set_vblockstore(store);
    register_plugin(store);
    base::xauto_ptr<xcsobject_t> ptr_engine_obj(create_engine(*this, xconsensus::enum_xconsensus_pacemaker_type_clock_cert));
    m_proposal_maker = block_maker->get_proposal_maker(account_id);
    m_raw_timer = get_thread()->create_timer((base::xtimersink_t*)this);
    m_raw_timer->start(m_timer_repeat_time_ms, m_timer_repeat_time_ms);
    xdbg("xbatch_packer::xbatch_packer,create,this=%p,account=%s,tableid=%d", this, account_id.c_str(), tableid);
}

xbatch_packer::~xbatch_packer() {
    if (m_raw_timer != nullptr) {
        m_raw_timer->release_ref();
    }
    xdbg("xbatch_packer::~xbatch_packer,destory,this=%p", this);
}

bool xbatch_packer::close(bool force_async) {
    xcsaccount_t::close(force_async);
    // xdbg("xbatch_packer::close, this=%p,refcount=%d", this, get_refcount());
    return true;
}

bool xbatch_packer::on_object_close() {
    // xdbg("xbatch_packer::on_object_close this=%p,refcount=%d", this, get_refcount());
    if (m_raw_timer != nullptr) {
        m_raw_timer->stop();
        m_raw_timer->close();
    }
    return xcsaccount_t::on_object_close();
}

uint16_t xbatch_packer::get_tableid() {
    return m_tableid;
}

void xbatch_packer::set_xip(xblock_consensus_para_t & blockpara, const xvip2_t & leader) {
    auto zone_id = get_zone_id_from_xip2(leader);
    // if consensus zone
    if (zone_id == base::enum_chain_zone_consensus_index) {
        if (xcons_utl::is_auditor(leader)) {
            // leader is auditor xip, set auditor_xip to leader, validator to chid group xip
            // blockpara.auditor_xip = leader;
            xvip2_t child = get_child_xip(leader, get_account());
            auto group_id = child;
            reset_node_id_to_xip2(group_id);
            set_node_id_to_xip2(group_id, 0x3FF);
            // blockpara.validator_xip = group_id;
            blockpara.set_xip(group_id, leader);
            xdbg("[xunitservice] set auditor leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor()).c_str());
        } else {
            // leader is validator xip, set validator_xip to leader, auditor to parent group xip
            // blockpara.auditor_xip = xvip2_t{(uint64_t)0, (uint64_t)0};
            xvip2_t parent = get_parent_xip(leader);
            auto group_id = parent;
            reset_node_id_to_xip2(group_id);
            set_node_id_to_xip2(group_id, 0x3FF);
            // blockpara.auditor_xip = group_id;
            // blockpara.validator_xip = leader;
            blockpara.set_xip(leader, group_id);
            xdbg("[xunitservice] set validator leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor()).c_str());
        }
    } else {
        // rec/zec without auditor
        // blockpara.validator_xip = leader;
        // blockpara.auditor_xip = xvip2_t{(uint64_t)0, (uint64_t)0};  // TODO(justin) set real auditor xip
        blockpara.set_xip(leader, xvip2_t{(uint64_t)0, (uint64_t)0});
    }
}

void xbatch_packer::invoke_sync(const std::string & account, const std::string & reason) {
    // TODO use xevent_behind_on_demand_t
#if 0
    mbus::xevent_ptr_t block_event = std::make_shared<mbus::xevent_behind_check_t>(account, mbus::enum_behind_type_common, reason);
    m_para->get_resources()->get_bus()->push_event(block_event);
#endif
}

bool xbatch_packer::start_proposal(base::xblock_mptrs& latest_blocks) {
    uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
    xblock_consensus_para_t proposal_para(get_account(), m_last_view_clock, m_last_view_id, viewtoken, latest_blocks.get_latest_cert_block()->get_height() + 1);
    proposal_para.set_latest_blocks(latest_blocks);

    if (m_last_view_clock < m_start_time) {
        return false;
    }

    if (false == m_proposal_maker->can_make_proposal(proposal_para)) {
        xwarn("xbatch_packer::start_proposal fail-cannot make proposal.%s", proposal_para.dump().c_str());
        return false;
    }

    auto local_xip = get_xip2_addr();
    set_xip(proposal_para, local_xip);  // set leader xip

    xdbg("xbatch_packer::start_proposal leader_node %s", proposal_para.dump().c_str());
    xblock_ptr_t proposal_block = m_proposal_maker->make_proposal(proposal_para);
    if (proposal_block == nullptr) {
        xwarn("xbatch_packer::start_proposal fail-make_proposal.%s", proposal_para.dump().c_str());
        return false;
    }
    base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
    push_event_down(*_event_obj, this, 0, 0);
    // check viewid again, may changed
    if (m_last_view_id != proposal_block->get_viewid()) {
        xwarn("xbatch_packer::start_proposal fail-finally viewid changed. %s latest_viewid=%" PRIu64 "",
            proposal_para.dump().c_str(), proposal_block->get_viewid());
        return false;
    }

    XMETRICS_COUNTER_INCREMENT("cons_tableblock_start_leader", 1);
    xinfo("xbatch_packer::start_proposal succ-leader start consensus. block=%s this:%p node:%s xip:%s",
            proposal_block->dump().c_str(), this, m_para->get_resources()->get_account().c_str(), xcons_utl::xip_to_hex(local_xip).c_str());
    return true;
}

// view updated and the judge is_leader
// then start new consensus from leader
bool xbatch_packer::on_view_fire(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    auto view_ev = dynamic_cast<const xconsensus::xcsview_fire *>(&event);
    xassert(view_ev != nullptr);
    xassert(view_ev->get_viewid() >= m_last_view_id);
    xassert(view_ev->get_account() == get_account());
    m_is_leader = false;
    m_leader_packed = false;
    // fix: viewchange on different rounds
    if (view_ev->get_clock() < m_start_time) {
        return false;
    }

    if (view_ev->get_clock() + 2 < m_para->get_resources()->get_chain_timer()->logic_time()) {
        xinfo("xbatch_packer::on_view_fire clock delay:%llu:%llu", view_ev->get_clock(), m_para->get_resources()->get_chain_timer()->logic_time());
        return false;
    }

    XMETRICS_TIME_RECORD("cons_tableblock_view_change_time_consuming");
    m_last_view_id = view_ev->get_viewid();
    m_last_view_clock = view_ev->get_clock();

    m_unorder_cache.on_view_fire(m_last_view_id);

    auto accessor = m_para->get_resources()->get_data_accessor();
    auto leader_election = m_para->get_resources()->get_election();
    auto node_account = m_para->get_resources()->get_account();
    auto local_xip = get_xip2_addr();
    auto zone_id = get_zone_id_from_xip2(local_xip);
    if (zone_id != base::enum_chain_zone_consensus_index && zone_id != base::enum_chain_zone_beacon_index && zone_id != base::enum_chain_zone_zec_index) {
        xerror("xbatch_packer::on_view_fire fail-wrong zone id. zoneid=%d", zone_id);
        return false;
    }

    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(get_account());

    // check if this node is leader
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto version = accessor->version_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    xassert(!ec);
    if (ec) {
        xerror("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }
    uint16_t rotate_mode = enum_rotate_mode_rotate_by_view_id;
    xvip2_t leader_xip = leader_election->get_leader_xip(m_last_view_id, get_account(), latest_blocks.get_latest_cert_block(), local_xip, local_xip, version, rotate_mode);
    bool is_leader_node = xcons_utl::xip_equals(leader_xip, local_xip);
    if (!is_leader_node) {
        // backup do nothing
        xinfo("xbatch_packer::on_view_fire backup_node this:%p node:%s xip:%s,leader:%s,rotate_mode:%d",
                this, node_account.c_str(),
                xcons_utl::xip_to_hex(local_xip).c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), rotate_mode);

        xconsensus::xcspdu_fire* xcspdu_fire_event = m_unorder_cache.get_proposal_event(m_last_view_id);
        if (xcspdu_fire_event != nullptr) {
            recv_in(xcspdu_fire_event->get_from_xip(), xcspdu_fire_event->get_to_xip(), xcspdu_fire_event->_packet, cur_thread_id, timenow_ms);
            xcspdu_fire_event->release_ref();
        }
        return true;
    }

    m_is_leader = true;
    m_leader_packed = start_proposal(latest_blocks);
    return true;
}

bool  xbatch_packer::on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) {
    if (!m_is_leader || m_leader_packed) {
        return true;
    }
    // xdbg("xbatch_packer::on_timer_fire retry start proposal.this:%p node:%s", this, m_para->get_resources()->get_account().c_str());
    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(get_account());
    m_leader_packed = start_proposal(latest_blocks);
    return true;
}

bool  xbatch_packer::on_timer_start(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) {
    // xdbg("xbatch_packer::on_timer_start,this=%p", this);
    return true;
}

bool  xbatch_packer::on_timer_stop(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) {
    // xdbg("xbatch_packer::on_timer_stop,this=%p", this);
    return true;
}

bool xbatch_packer::on_pdu_event_up(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xconsensus::xcspdu_fire * _evt_obj = (xconsensus::xcspdu_fire *)&event;
    auto                      packet = _evt_obj->_packet;
    auto                      local_xip = get_xip2_addr();
    xassert(packet.get_block_account() == get_account());
    if (xcons_utl::is_broadcast_address(_evt_obj->get_to_xip())) {
        // leader broadcast msg
        auto to_xip = local_xip;
        set_node_id_to_xip2(to_xip, common::xbroadcast_slot_id_value);
        xdbg("xbatch_packer::on_pdu_event_up %s broadcast %x from: %s to:%s",
             get_account().c_str(),
             event.get_type(),
             xcons_utl::xip_to_hex(local_xip).c_str(),
             xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, to_xip, packet, cur_thread_id, timenow_ms);
    } else {
        xdbg("xbatch_packer::on_pdu_event_up %s send %x from: %s to:%s",
             get_account().c_str(),
             event.get_type(),
             xcons_utl::xip_to_hex(local_xip).c_str(),
             xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, _evt_obj->get_to_xip(), packet, cur_thread_id, timenow_ms);
    }
    return true;  // stop here
}

bool xbatch_packer::send_out(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    // xinfo("xbatch_packer::send_out pdu=%s,body_size:%d,this:%p",
    //      packet.dump().c_str(),
    //      packet.get_msg_body().size(),
    //      this);
    XMETRICS_PACKET_INFO("consensus_tableblock",
                        "pdu_send_out", packet.dump(),
                        "node_xip", xcons_utl::xip_to_hex(from_addr));

    auto network_proxy = m_para->get_resources()->get_network();
    xassert(network_proxy != nullptr);
    if (network_proxy != nullptr) {
        return network_proxy->send_out((uint32_t)xBFT_msg, from_addr, to_addr, packet, cur_thread_id, timenow_ms);
    }
    return false;
}

bool xbatch_packer::recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    XMETRICS_PACKET_INFO("consensus_tableblock",
                        "pdu_recv_in", packet.dump(),
                        // "from_xip", xcons_utl::xip_to_hex(from_addr),
                        // "to_xip", xcons_utl::xip_to_hex(to_addr),
                        "node_xip", xcons_utl::xip_to_hex(get_xip2_addr()));

    // proposal should pass to xbft, so xbft could realize local is beind in view/block and do sync
    m_unorder_cache.filter_event(m_last_view_id, from_addr, to_addr, packet);

    bool is_leader = false;
    auto type = packet.get_msg_type();
    bool valid = true;
    if (type == xconsensus::enum_consensus_msg_type_proposal) {
        auto latest_block = m_para->get_resources()->get_vblockstore()->get_latest_cert_block(get_account());
        if (latest_block->get_height() + 1 == packet.get_block_height()) {
            auto leader_election = m_para->get_resources()->get_election();
            auto accessor = m_para->get_resources()->get_data_accessor();
            std::error_code ec{election::xdata_accessor_errc_t::success};
            auto version = accessor->version_from(common::xip2_t{from_addr.low_addr, from_addr.high_addr}, ec);
            if (ec) {
                // TODO here may happen when many elect blocks sync
                xwarn("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(from_addr).c_str());
                return false;
            }
            xvip2_t leader_xip = leader_election->get_leader_xip(packet.get_block_viewid(), get_account(), latest_block.get(), to_addr, from_addr, version, enum_rotate_mode_rotate_by_view_id);
            if (!xcons_utl::xip_equals(leader_xip, from_addr)) {
                valid = false;
            }
        }
    }
    if (!valid) {
        // xwarn("xbatch_packer::recv_in fail-invalid msg,viewid=%ld,pdu=%s,at_node:%s,this:%p",
        //       m_last_view_id, packet.dump().c_str(), xcons_utl::xip_to_hex(to_addr).c_str(), this);
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "fail_proposal_invalid", packet.dump(),
                            "node_xip", xcons_utl::xip_to_hex(get_xip2_addr()));
        return false;
    }

    // xdbg_info("xbatch_packer::recv_in succ-valid msg,viewid=%ld,pdu=%s,at_node:%s,this:%p",
    //         m_last_view_id, packet.dump().c_str(), xcons_utl::xip_to_hex(to_addr).c_str(), this);
    return xcsaccount_t::recv_in(from_addr, to_addr, packet, cur_thread_id, timenow_ms);
}

int xbatch_packer::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert, xcsobject_t * _from_child) {
    return m_proposal_maker->verify_proposal(proposal_block, bind_clock_cert);
}

// get parent group xip
xvip2_t xbatch_packer::get_parent_xip(const xvip2_t & local_xip) {
    auto                            leader_election = m_para->get_resources()->get_election();
    auto                            election_store = leader_election->get_election_cache_face();
    xelection_cache_face::elect_set elect_set_;
    auto                            ret = election_store->get_parent_election(local_xip, &elect_set_);
    if (ret > 0) {
        auto xip = elect_set_[0].xip;
        reset_node_id_to_xip2(xip);
        set_node_id_to_xip2(xip, 0xFFF);
        return xip;
    }
    return xvip2_t{(uint64_t)-1, (uint64_t)-1};
}

xvip2_t xbatch_packer::get_child_xip(const xvip2_t & local_xip, const std::string & account) {
    auto                            child_group_id = xcons_utl::get_groupid_by_account(local_xip, account);
    xelection_cache_face::elect_set elect_set_;
    auto                            leader_election = m_para->get_resources()->get_election();
    auto                            election_store = leader_election->get_election_cache_face();
    auto                            ret = election_store->get_group_election(local_xip, child_group_id, &elect_set_);
    if (ret > 0) {
        auto xip = elect_set_[0].xip;
        return xip;
    }
#ifdef DEBUG
    if (xcons_utl::is_auditor(local_xip)) {
        xassert(false);
    }
#endif  // 0
    return xvip2_t{(uint64_t)0, (uint64_t)0};
}

bool xbatch_packer::reset_xip_addr(const xvip2_t & new_addr) {
    xdbg("xbatch_packer::reset_xip_addr %s node:%s this:%p", xcons_utl::xip_to_hex(new_addr).c_str(), m_para->get_resources()->get_account().c_str(), this);
    return xcsaccount_t::reset_xip_addr(new_addr);
}

bool xbatch_packer::set_start_time(const common::xlogic_time_t& start_time) {
    xdbg("xbatch_packer::set_start_time %d node:%s this:%p", start_time, m_para->get_resources()->get_account().c_str(), this);
    m_start_time = start_time;
    return true;
}

bool xbatch_packer::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() != xconsensus::enum_xconsensus_code_successful) {
        if (is_leader) {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_leader_finish_fail", 1);
        } else {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_backup_finish_fail", 1);
        }
        // xwarn("xbatch_packer::on_proposal_finish fail. leader:%d,error_code:%d,proposal=%s,at_node:%s",
        //     is_leader,
        //     _evt_obj->get_error_code(),
        //     _evt_obj->get_target_proposal()->dump().c_str(),
        //     xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "proposal_finish_fail", _evt_obj->get_target_proposal()->dump(),
                            "is_leader", is_leader,
                            "error_code", _evt_obj->get_error_code(),
                            "node_xip", xcons_utl::xip_to_hex(get_xip2_addr()));
    } else {
        if (is_leader) {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_leader_finish_succ", 1);
        } else {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_backup_finish_succ", 1);
        }
        // xinfo("xbatch_packer::on_proposal_finish succ. leader:%d,proposal=%s,at_node:%s",
        //     is_leader,
        //     _evt_obj->get_target_proposal()->dump().c_str(),
        //     xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "proposal_finish_succ", _evt_obj->get_target_proposal()->dump(),
                            "is_leader", is_leader,
                            "node_xip", xcons_utl::xip_to_hex(get_xip2_addr()));

        base::xvblock_t *vblock = _evt_obj->get_target_proposal();
        xassert(vblock->is_input_ready(true));
        xassert(vblock->is_output_ready(true));
        vblock->add_ref();
        mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_consensus_data_t>(vblock, is_leader);
        m_mbus->push_event(ev);
    }
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

bool xbatch_packer::on_consensus_commit(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_consensus_commit(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xconsensus_commit * _evt_obj = (xconsensus::xconsensus_commit *)&event;
    xdbg("xbatch_packer::on_consensus_commit, %s class=%d, at_node:%s",
        _evt_obj->get_target_commit()->dump().c_str(), _evt_obj->get_target_commit()->get_block_class(), xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

NS_END2
