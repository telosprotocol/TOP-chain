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
  : xcsaccount_t(_context, target_thread_id, account_id), m_mbus(mb), m_tableid(tableid), m_last_view_id(0), m_para(para), m_block_maker(block_maker) {
    auto cert_auth = m_para->get_resources()->get_certauth();
    register_plugin(cert_auth);
    auto store = m_para->get_resources()->get_vblockstore();
    set_vblockstore(store);
    register_plugin(store);
    base::xauto_ptr<xcsobject_t> ptr_engine_obj(create_engine(*this, xconsensus::enum_xconsensus_pacemaker_type_clock_cert));
    m_txpool_table = make_observer(block_maker->get_txpool_table(account_id));
    xdbg("xbatch_packer::xbatch_packer,create,this=%p,account=%s,tableid=%d", this, account_id.c_str(), tableid);
    m_proposal_maker = block_maker->get_proposal_maker(account_id);
}

xbatch_packer::~xbatch_packer() {
    xdbg("xbatch_packer::~xbatch_packer,destory,this=%p", this);
}

uint16_t xbatch_packer::get_tableid() {
    return m_tableid;
}

void xbatch_packer::set_xip(xblock_maker_para_t & blockpara, const xvip2_t & leader) {
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
            xdbg("[xunitservice] set auditor leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator_xip()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor_xip()).c_str());
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
            xdbg("[xunitservice] set validator leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator_xip()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor_xip()).c_str());
        }
    } else {
        // rec/zec without auditor
        // blockpara.validator_xip = leader;
        // blockpara.auditor_xip = xvip2_t{(uint64_t)0, (uint64_t)0};  // TODO(justin) set real auditor xip
        blockpara.set_xip(leader, xvip2_t{(uint64_t)0, (uint64_t)0});
    }
}

void xbatch_packer::invoke_sync(const std::string & account, const std::string & reason) {
    mbus::xevent_ptr_t block_event = std::make_shared<mbus::xevent_behind_origin_t>(account, mbus::enum_behind_type_common, reason);
    m_para->get_resources()->get_bus()->push_event(block_event);
}

// view updated and the judge is_leader
// then start new consensus from leader
bool xbatch_packer::on_view_fire(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    auto view_ev = dynamic_cast<const xconsensus::xcsview_fire *>(&event);
    xassert(view_ev != nullptr);
    xassert(view_ev->get_viewid() >= m_last_view_id);
    xassert(view_ev->get_account() == get_account());

    XMETRICS_TIME_RECORD("cons_tableblock_view_change_time_consuming");
    m_last_view_id = view_ev->get_viewid();

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

#if 1
    xblock_maker_para_t proposal_para(view_ev->get_clock(), view_ev->get_viewid());
    if (false == m_proposal_maker->can_make_proposal(proposal_para)) {
        xwarn("xbatch_packer::on_view_fire fail-cannot make proposal.account=%s,viewid=%ld", get_account().c_str(), view_ev->get_viewid());
        return false;
    }

    // check if this node is leader
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto version = accessor->version_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    xassert(!ec);
    if (ec) {
        xerror("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }
    uint16_t rotate_mode = enum_rotate_mode_rotate_by_view_id;
    xvip2_t leader_xip = leader_election->get_leader_xip(m_last_view_id, get_account(), proposal_para.get_latest_cert_block().get(), local_xip, local_xip, version, rotate_mode);
    bool is_leader_node = xcons_utl::xip_equals(leader_xip, local_xip);
    if (!is_leader_node) {
        // backup do nothing
        xinfo("JIMMY xbatch_packer::on_view_fire backup_node account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ", this:%p node:%s xip:%s,leader:%s,rotate_mode:%d",
                get_account().c_str(), m_last_view_id, proposal_para.get_latest_cert_block()->get_height(), this, node_account.c_str(),
                xcons_utl::xip_to_hex(local_xip).c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), rotate_mode);

        xconsensus::xcspdu_fire* xcspdu_fire_event = m_unorder_cache.get_proposal_event(m_last_view_id);
        if (xcspdu_fire_event != nullptr) {
            recv_in(xcspdu_fire_event->get_from_xip(), xcspdu_fire_event->get_to_xip(), xcspdu_fire_event->_packet, cur_thread_id, timenow_ms);
            xcspdu_fire_event->release_ref();
        }
        return true;
    }
    set_xip(proposal_para, local_xip);  // set leader xip

    xdbg("JIMMY xbatch_packer::on_view_fire leader_node account=%s,viewid=%ld,clock=%ld,cert=%s",
        get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), proposal_para.get_latest_cert_block()->dump().c_str());
    xblock_ptr_t proposal_block = m_proposal_maker->make_proposal(proposal_para);
    if (proposal_block == nullptr) {
        xwarn("xbatch_packer::on_view_fire fail-make_proposal.account=%s,viewid=%ld,clock=%ld,cert=%s",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), proposal_para.get_latest_cert_block()->dump().c_str());
        return false;
    }
    base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
    if (!push_event_down(*_event_obj, this, 0, 0)) {
        xwarn("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",leader fail-push proposal event down.",
            get_account().c_str(), view_ev->get_viewid(), proposal_para.get_latest_cert_block()->get_height());
        return false;
    }
    // check viewid again, may changed
    if (view_ev->get_viewid() != proposal_block->get_viewid()) {
        xwarn("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",latest_viewid=%" PRIu64 ",leader fail-viewid changed.",
            get_account().c_str(), view_ev->get_viewid(), proposal_para.get_latest_cert_block()->get_height(), proposal_block->get_viewid());
        return false;
    }
#ifdef ENABLE_METRICS
    XMETRICS_COUNTER_INCREMENT("cons_tableblock_start_leader", 1);
    m_cons_start_time_ms = base::xtime_utl::gmttime_ms();
#endif
    xinfo("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ", succ-leader start consensus. block=%s this:%p node:%s xip:%s",
            get_account().c_str(), m_last_view_id, proposal_block->dump().c_str(), this, node_account.c_str(), xcons_utl::xip_to_hex(local_xip).c_str());
    return true;
#endif


#if 0
    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(*this);
    base::xvblock_t* latest_cert_block = latest_blocks.get_latest_cert_block();

    xinfo("xbatch_packer::on_view_fire account=%s,xip:%s,viewid=%" PRIu64 ",lastest_cert_block=%s",
          get_account().c_str(), xcons_utl::xip_to_hex(local_xip).c_str(),
          m_last_view_id, latest_cert_block->dump().c_str());
    if (m_last_view_id <= latest_cert_block->get_viewid()) {
        xwarn("xbatch_packer::on_view_fire fail-wrong viewid. viewid=%" PRIu64 ",lastest_cert_block=%s", m_last_view_id, latest_cert_block->dump().c_str());
        return false;
    }
    // when latest cert block changed, then verify latest blocks and update lock blocks again
    if (m_latest_cert_block_hash != latest_cert_block->get_block_hash()) {
        if (!data::xblocktool_t::verify_latest_blocks(latest_blocks)) {
            invoke_sync(latest_blocks.get_latest_cert_block()->get_account(), "unit service check behind");
            xwarn("xbatch_packer::on_view_fire fail-verify latest blocks. viewid=%" PRIu64 ",lastest_cert_block=%s", m_last_view_id, latest_cert_block->dump().c_str());
            return false;
        }
        m_txpool_table->update_lock_blocks(latest_blocks);
        m_can_make_empty_block = data::xblocktool_t::can_make_next_empty_block(latest_blocks, m_empty_block_max_num);
        m_latest_cert_block_hash = latest_cert_block->get_block_hash();
    }

    // if need not make next block, then return.
    auto committed_block = latest_blocks.get_latest_committed_block();
    m_txpool_table->update_committed_table_block(dynamic_cast<xblock_t*>(committed_block));
    std::vector<std::string> unit_accounts = m_txpool_table->get_accounts();
    if (!m_can_make_empty_block && unit_accounts.empty()) {
        xinfo("xbatch_packer::on_view_fire no need make next block. account=%s,viewid=%" PRIu64 ",height=%" PRIu64 "",
                get_account().c_str(), m_last_view_id, latest_cert_block->get_height());
        return true;
    }

    // check if this node is leader
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto version = accessor->version_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    if (ec) {
        // TODO here may happen when many elect blocks sync
        xwarn("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        return false;
    }
    uint16_t rotate_mode = enum_rotate_mode_rotate_by_view_id;
    auto timer_block = m_para->get_resources()->get_vblockstore()->get_latest_cert_block(sys_contract_beacon_timer_addr);
    xvip2_t leader_xip = leader_election->get_leader_xip(m_last_view_id, get_account(), latest_cert_block, local_xip, local_xip, version, rotate_mode);
    if (!xcons_utl::xip_equals(leader_xip, local_xip)) {
        // backup do nothing
        xinfo("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",backup this:%p node:%s xip:%s,leader:%s,rotate_mode:%d",
                get_account().c_str(), m_last_view_id, latest_cert_block->get_height(), this, node_account.c_str(),
                xcons_utl::xip_to_hex(local_xip).c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), rotate_mode);

        xconsensus::xcspdu_fire* xcspdu_fire_event = m_unorder_cache.get_proposal_event(m_last_view_id);
        if (xcspdu_fire_event != nullptr) {
            recv_in(xcspdu_fire_event->get_from_xip(), xcspdu_fire_event->get_to_xip(), xcspdu_fire_event->_packet, cur_thread_id, timenow_ms);
            xcspdu_fire_event->release_ref();
        }

        return true;
    }

    // ensure other node receive drand
    // consider too old drand block
    auto drand_block = m_para->get_resources()->get_vblockstore()->get_latest_committed_block(sys_drand_addr);
    if (drand_block->get_clock() == 0) {
        xwarn("xbatch_packer::on_view_fire no valid drand. chainid: %d account=%s,viewid=%" PRIu64 ",height=%" PRIu64 "",
              drand_block->get_chainid(),
              get_account().c_str(),
              m_last_view_id,
              latest_cert_block->get_height());
        return false;
    }

    xassert(timer_block->get_clock() != 0);
    xblock_maker_para_t blockpara;
    blockpara.clock = timer_block->get_clock();
    blockpara.viewid = m_last_view_id;
    blockpara.latest_blocks = &latest_blocks;
    blockpara.timer_block = timer_block.get();
    blockpara.drand_block = drand_block.get();
    blockpara.can_make_empty_block = m_can_make_empty_block;
    blockpara.unit_accounts = std::move(unit_accounts);
    blockpara.txpool_table = m_txpool_table.get();
    blockpara.drand_height = drand_block->get_height();
    set_xip(blockpara, local_xip);
    base::xauto_ptr<base::xvblock_t> proposal_block(m_block_maker->make_block(get_account(), blockpara, local_xip));
    if (proposal_block == nullptr) {
        xwarn("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",leader fail-make block null.",
            get_account().c_str(), m_last_view_id, latest_cert_block->get_height());
        return false;
    }

    base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
    if (!push_event_down(*_event_obj, this, 0, 0)) {
        xwarn("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",leader fail-push proposal event down.",
            get_account().c_str(), m_last_view_id, latest_cert_block->get_height());
        return false;
    }
    // check viewid again, may changed
    if (m_last_view_id != proposal_block->get_viewid()) {
        xwarn("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ",height=%" PRIu64 ",latest_viewid=%" PRIu64 ",leader fail-viewid changed.",
            get_account().c_str(), m_last_view_id, latest_cert_block->get_height(), proposal_block->get_viewid());
        return false;
    }
#ifdef ENABLE_METRICS
    XMETRICS_COUNTER_INCREMENT("cons_tableblock_start_leader", 1);
    m_cons_start_time_ms = base::xtime_utl::gmttime_ms();
#endif
    xinfo("xbatch_packer::on_view_fire account=%s,viewid=%" PRIu64 ", succ-leader start consensus. block=%s this:%p node:%s xip:%s",
            get_account().c_str(), m_last_view_id, proposal_block->dump().c_str(), this, node_account.c_str(), xcons_utl::xip_to_hex(local_xip).c_str());
    return true;
#endif
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
            auto timer_block = m_para->get_resources()->get_vblockstore()->get_latest_cert_block(sys_contract_beacon_timer_addr);
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

#if 0
    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(*this);
    auto committed_block = latest_blocks.get_latest_committed_block();
    m_txpool_table->update_committed_table_block(dynamic_cast<xblock_t*>(committed_block));

    XMETRICS_COUNTER_INCREMENT("cons_tableblock_verify_backup", 1);

    base::xvqcert_t * bind_drand_cert = nullptr;
    if (proposal_block->get_block_class() == base::enum_xvblock_class_light) {
        bind_drand_cert = get_bind_drand(proposal_block);
        if (bind_drand_cert == nullptr) {
            XMETRICS_PACKET_INFO("consensus_tableblock",
                                "proposal_verify_fail", proposal_block->dump(),
                                "error_code", "enum_xconsensus_error_not_found_drand",
                                "node_xip", data::xdatautil::xip_to_hex(get_xip2_addr()));
            return xconsensus::enum_xconsensus_error_not_found_drand;
        }
    }

    auto ret = m_block_maker->verify_block(proposal_block, bind_clock_cert, bind_drand_cert, m_txpool_table.get(), committed_block->get_height(), get_xip2_addr());
    if (ret) {
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "proposal_verify_fail", proposal_block->dump(),
                            "error_code", ret,
                            "highqc_height", latest_blocks.get_latest_cert_block()->get_height(),
                            "lock_height", latest_blocks.get_latest_locked_block()->get_height(),
                            "commit_height", latest_blocks.get_latest_committed_block()->get_height(),
                            "node_xip", data::xdatautil::xip_to_hex(get_xip2_addr()));
    } else {
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "proposal_verify_succ", proposal_block->dump(),
                            "error_code", ret);
    }

    if (bind_drand_cert != nullptr) {
        bind_drand_cert->release_ref();
    }

    return ret;
#endif
}

base::xvqcert_t* xbatch_packer::get_bind_drand(base::xvblock_t *proposal_block) {
    base::xvqcert_t * bind_drand_cert = nullptr;
    uint64_t drand_height = proposal_block->get_cert()->get_drand_height();
    if (drand_height == 0) {
        xwarn("xbatch_packer::verify_proposal, not found drand(%llu). proposal=%s", drand_height, proposal_block->dump().c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> drand_block = get_vblockstore()->load_block_object(sys_drand_addr, drand_height, false);
    if (drand_block == nullptr) {
        //xwarn("xbatch_packer::verify_proposal, not found drand(%llu). proposal=%s", drand_height, proposal_block->dump().c_str());
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "fail_find_drand", proposal_block->dump(),
                            "drand_height", drand_height,
                            "node_xip", xcons_utl::xip_to_hex(get_xip2_addr()));
        return nullptr;
    }

    bind_drand_cert = drand_block->get_cert();
    bind_drand_cert->add_ref();

    return bind_drand_cert;
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

bool xbatch_packer::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() != xconsensus::enum_xconsensus_code_successful) {
#ifdef ENABLE_METRICS
        if (is_leader) {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_leader_finish_fail", 1);
        } else {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_backup_finish_fail", 1);
        }
#endif
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
#ifdef ENABLE_METRICS
        if (is_leader) {
            uint64_t now = base::xtime_utl::gmttime_ms();
            uint64_t time_consuming = (now > m_cons_start_time_ms) ? (now - m_cons_start_time_ms) : 0;
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_leader_finish_succ", 1);
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_succ_time_consuming", time_consuming);
        } else {
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_backup_finish_succ", 1);
        }
#endif
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
    xinfo("xbatch_packer::on_consensus_commit, %s class=%d, at_node:%s",
        _evt_obj->get_target_commit()->dump().c_str(), _evt_obj->get_target_commit()->get_block_class(), xcons_utl::xip_to_hex(get_xip2_addr()).c_str());

#ifdef ENABLE_METRICS
    if (_evt_obj->get_target_commit()->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        uint64_t now = base::xtime_utl::gmttime();
        if (now > _evt_obj->get_target_commit()->get_cert()->get_gmtime() + CONFIRM_DELAY_TOO_MUCH_TIME) {
            xwarn("commit delay too much: %s, cert time:%llu,now:%llu", _evt_obj->get_target_commit()->dump().c_str(),
                  _evt_obj->get_target_commit()->get_cert()->get_gmtime(), now);
            XMETRICS_COUNTER_INCREMENT("cons_tableblock_commit_delay_too_much", 1);
        }
    }
#endif
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

NS_END2
