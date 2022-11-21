// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xbatch_packer.h"

#include "xblockmaker/xblockmaker_error.h"
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
#include "xchain_fork/xutility.h"
#include "xunit_service/xerror/xerror.h"
#include "xstatestore/xstatestore_face.h"
#include "xsync/xsync_on_demand.h"
#include "xbase/xutl.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

#define MIN_TRANSACTION_NUM_FOR_HIGH_TPS (30)
#define MIN_TRANSACTION_NUM_FOR_MIDDLE_TPS (20)
#define MIN_TRANSACTION_NUM_FOR_LOW_TPS (10)

#define TRY_MAKE_BLOCK_TIMER_INTERVAL (30)
#define TRY_HIGH_TPS_TIME_WINDOW (250)
#define TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW (500)
#define TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW (750)

xbatch_packer::xbatch_packer(base::xtable_index_t                             &tableid,
                             const std::string &                              account_id,
                             std::shared_ptr<xcons_service_para_face> const & para,
                             std::shared_ptr<xblock_maker_face> const &       block_maker,
                             base::xcontext_t &                               _context,
                             const uint32_t                                   target_thread_id)
  : xcsaccount_t(_context, target_thread_id, account_id), m_tableid(tableid), m_last_view_id(0), m_para(para), m_table_addr(account_id) {
    auto cert_auth = m_para->get_resources()->get_certauth();
    m_last_xip2.high_addr = -1;
    m_last_xip2.low_addr = -1;
    register_plugin(cert_auth);
    auto store = m_para->get_resources()->get_vblockstore();
    set_vblockstore(store);
    register_plugin(store);
    base::xauto_ptr<xcsobject_t> ptr_engine_obj(create_engine(*this, xconsensus::enum_xconsensus_pacemaker_type_clock_cert));
    ptr_engine_obj->register_plugin(para->get_resources()->get_xbft_workpool());  // used for xbft heavy work, such as verify_proposal and signature verify.
    m_proposal_maker = block_maker->get_proposal_maker(account_id);
    m_proposal_maker->set_certauth(cert_auth);
    m_raw_timer = get_thread()->create_timer((base::xtimersink_t*)this);
    xunit_info("xbatch_packer::xbatch_packer,create,this=%p,account=%s,tableid=%d", this, account_id.c_str(), tableid.to_table_shortid());
}

xbatch_packer::~xbatch_packer() {
    if (m_raw_timer != nullptr) {
        m_raw_timer->release_ref();
    }
    xunit_info("xbatch_packer::~xbatch_packer,destory,this=%p,account=%s", this,  get_account().c_str());
}

bool xbatch_packer::close(bool force_async) {
    xcsaccount_t::close(force_async);
     xunit_dbg("xbatch_packer::close, this=%p,refcount=%d, account=%s.", this, get_refcount(),  get_account().c_str());
    return true;
}

bool xbatch_packer::on_object_close() {
    // xunit_dbg("xbatch_packer::on_object_close this=%p,refcount=%d", this, get_refcount());
    if (m_raw_timer != nullptr) {
        m_raw_timer->stop();
        m_raw_timer->close();
    }
    return xcsaccount_t::on_object_close();
}

base::xtable_index_t xbatch_packer::get_tableid() {
    return m_tableid;
}

void xbatch_packer::set_xip(data::xblock_consensus_para_t & blockpara, const xvip2_t & leader) {
    auto zone_id = get_zone_id_from_xip2(leader);
    // if consensus zone
    if (zone_id == base::enum_chain_zone_consensus_index || zone_id == base::enum_chain_zone_evm_index) {
        if (xcons_utl::is_auditor(leader)) {
            // leader is auditor xip, set auditor_xip to leader, validator to chid group xip
            // blockpara.auditor_xip = leader;
            xvip2_t child = get_child_xip(leader, get_account());
            auto group_id = child;
            reset_node_id_to_xip2(group_id);
            set_node_id_to_xip2(group_id, 0x3FF);
            // blockpara.validator_xip = group_id;
            blockpara.set_xip(group_id, leader);
            xunit_dbg("[xunitservice] set auditor leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor()).c_str());
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
            xunit_dbg("[xunitservice] set validator leader validator:%s auditor:%s", xcons_utl::xip_to_hex(blockpara.get_validator()).c_str(), xcons_utl::xip_to_hex(blockpara.get_auditor()).c_str());
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

xresources_face * xbatch_packer::get_resources() {
    return m_para->get_resources();
}

bool xbatch_packer::start_proposal(uint32_t min_tx_num) {
    if (nullptr == m_leader_cs_para) {
        xassert(false);
        return false;
    }
    data::xblock_consensus_para_t & proposal_para = *m_leader_cs_para;
    xunit_dbg_info("xbatch_packer::start_proposal leader begin make_proposal.%s", proposal_para.dump().c_str());
    data::xblock_ptr_t proposal_block = m_proposal_maker->make_proposal(proposal_para, min_tx_num);
    if (proposal_block == nullptr) {
        xunit_dbg("xbatch_packer::start_proposal fail-make_proposal.%s", proposal_para.dump().c_str());  // may has no txs for proposal
        return false;
    }

    set_vote_extend_data(proposal_block.get(), proposal_para.get_vote_extend_hash(), true);

    base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
    push_event_down(*_event_obj, this, 0, 0);
    // check viewid again, may changed
    if (m_last_view_id != proposal_block->get_viewid()) {
        xunit_warn("xbatch_packer::start_proposal fail-finally viewid changed. %s latest_viewid=%" PRIu64 "",
            proposal_para.dump().c_str(), proposal_block->get_viewid());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_view_changed, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return false;
    }

    XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 1);
    xunit_info("xbatch_packer::start_proposal succ-leader start consensus. block=%s this:%p node:%s xip:%s",
            proposal_block->dump().c_str(), this, m_para->get_resources()->get_account().c_str(), xcons_utl::xip_to_hex(proposal_para.get_leader_xip()).c_str());

    uint64_t cert_viewid = proposal_para.get_latest_cert_block()->get_viewid();
    if (cert_viewid + 1 != proposal_para.get_viewid()) {
        xunit_info("xbatch_packer::start_proposal table:%s no block viewids:%llu-%llu", get_account().c_str(), cert_viewid + 1, proposal_para.get_viewid() - 1);
    }
    
    XMETRICS_GAUGE(metrics::cons_packtx_with_threshold, (min_tx_num > 0) ? 1 : 0);
    return true;
}

void xbatch_packer::check_latest_cert_block(base::xvblock_t* _cert_block, const xconsensus::xcsview_fire* viewfire, std::error_code & ec) {
    if (_cert_block == nullptr) {
        ec = xunit_service::error::xenum_errc::packer_cert_block_invalid;
        xunit_warn("xbatch_packer::check_latest_cert_block fail-nullptr cert block,account=%s,viewid=%ld,clock=%ld", 
            get_account().c_str(), viewfire->get_viewid(), viewfire->get_clock());
        return;
    }
    if ( (viewfire->get_clock() < _cert_block->get_clock()) // proposal clock should >= cert clock
        || (viewfire->get_viewid() <= _cert_block->get_viewid())) { // proposal viewid should > cert viewid
        ec = error::xenum_errc::packer_view_behind;
        xunit_warn("xbatch_packer::check_latest_cert_block fail-view behind,account=%s,current=%ld,%ld,cert=%ld,%ld",
            get_account().c_str(), viewfire->get_viewid(), viewfire->get_clock(), _cert_block->get_viewid(), _cert_block->get_clock());
        return;
    }
}

bool xbatch_packer::connect_to_checkpoint() {
    auto local_xip = get_xip2_addr();
    common::xip2_t xip2(local_xip.low_addr, local_xip.high_addr);
    common::xnode_type_t node_type = common::node_type_from(xip2.zone_id(), xip2.cluster_id(), xip2.group_id());
    xdbg("connect_to_checkpoint node type:%s", common::to_string(node_type).c_str());

    if (common::has<common::xnode_type_t::rec>(node_type)
     || common::has<common::xnode_type_t::zec>(node_type)
     || common::has<common::xnode_type_t::consensus_auditor>(node_type)) {
        auto latest_cp_connect_height = m_para->get_resources()->get_vblockstore()->update_get_latest_cp_connected_block_height(get_account());
        auto latest_connect_height = m_para->get_resources()->get_vblockstore()->get_latest_connected_block_height(get_account());
        if (latest_cp_connect_height != latest_connect_height) {
            xinfo("connect_to_checkpoint checkpoint mismatch! cp_connect:%llu,connect:%llu,account:%s", latest_cp_connect_height, latest_connect_height, get_account().c_str());

            XMETRICS_GAUGE(metrics::cons_cp_check_succ, 0);
            return false;
        }
    }
    XMETRICS_GAUGE(metrics::cons_cp_check_succ, 1);
    return true;
}

void xbatch_packer::reset_leader_info() {
    m_is_leader = false;
    m_leader_packed = false;
    m_leader_cs_para = nullptr;
}

// view updated and the judge is_leader
// then start new consensus from leader
bool xbatch_packer::on_view_fire(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    auto view_ev = dynamic_cast<const xconsensus::xcsview_fire *>(&event);
    xassert(view_ev != nullptr);
    xassert(view_ev->get_viewid() >= m_last_view_id);
    xassert(view_ev->get_account() == get_account());
    m_raw_timer->stop();
    reset_leader_info();
    xdbg_info("xbatch_packer::on_view_fire account=%s,clock=%ld,viewid=%ld,start_time=%ld", get_account().c_str(), view_ev->get_clock(), view_ev->get_viewid(), m_start_time);
    auto local_xip = get_xip2_addr();
    if (xcons_utl::xip_equals(m_faded_xip2, local_xip)) {
        xdbg_info("xbatch_packer::on_view_fire local_xip equal m_fade_xip2 %s . fade round should not make proposal", xcons_utl::xip_to_hex(m_faded_xip2).c_str());
        return false;
    }

    // fix: viewchange on different rounds
    if (view_ev->get_clock() < m_start_time) {
        xunit_warn("xbatch_packer::on_view_fire fail-clock expired less than start time.account=%s,viewid=%ld,clock=%ld,start_time=%ld",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), m_start_time);
        XMETRICS_GAUGE(metrics::cons_view_fire_clock_delay, 1);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    if (view_ev->get_clock() + 2 < m_para->get_resources()->get_chain_timer()->logic_time()) {
        xunit_warn("xbatch_packer::on_view_fire fail-clock expired less than logic time.account=%s,viewid=%ld,clock=%ld,logic_time=%ld",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), m_para->get_resources()->get_chain_timer()->logic_time());
        XMETRICS_GAUGE(metrics::cons_view_fire_clock_delay, 1);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }
    clear_for_new_view();

    XMETRICS_TIME_RECORD("cons_tableblock_view_change_time_consuming");
    m_last_view_id = view_ev->get_viewid();
    m_last_view_clock = view_ev->get_clock();

    auto _cert_block = m_para->get_resources()->get_vblockstore()->get_latest_cert_block(get_account(), metrics::blockstore_access_from_us_on_view_fire);
    auto ret = check_state_sync(_cert_block.get());
    if (!ret) {
        xunit_warn("xbatch_packer::on_view_fire fail-check state sync,account=%s,viewid=%ld,clock=%ld,cert=%s",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), _cert_block->dump().c_str());        
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    auto const new_version = chain_fork::xutility_t::is_forked(fork_points::v1_7_0_block_fork_point, view_ev->get_clock());
    if (new_version) {
        auto ret = m_proposal_maker->account_index_upgrade();
        if (!ret) {
            xunit_warn("xbatch_packer::on_view_fire fail-account index upgrade,account=%s,viewid=%ld,clock=%ld,cert_height=%ld",
                get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(),_cert_block->get_height());        
            XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);            
            return false;
        }
    }

    std::error_code ec;
    check_latest_cert_block(_cert_block.get(), view_ev, ec);
    if (ec) {
        xunit_warn("xbatch_packer::on_view_fire fail-check latest cert block,account=%s,viewid=%ld,clock=%ld,cert=%s",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), _cert_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    auto accessor = m_para->get_resources()->get_data_accessor();
    auto leader_election = m_para->get_resources()->get_election();
    auto node_account = m_para->get_resources()->get_account();

    auto zone_id = get_zone_id_from_xip2(local_xip);
    if (zone_id != base::enum_chain_zone_consensus_index && zone_id != base::enum_chain_zone_beacon_index && zone_id != base::enum_chain_zone_zec_index &&
        zone_id != base::enum_chain_zone_evm_index && zone_id != base::enum_chain_zone_relay_index) {
        xerror("xbatch_packer::on_view_fire fail-wrong zone id. zoneid=%d", zone_id);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    // check if this node is leader
    // std::error_code ec{election::xdata_accessor_errc_t::success};
    auto election_epoch = accessor->election_epoch_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    if (ec) {
        xunit_warn("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }    

    uint16_t rotate_mode = enum_rotate_mode_rotate_by_view_id;
    xvip2_t leader_xip = leader_election->get_leader_xip(m_last_view_id, get_account(), _cert_block.get(), local_xip, local_xip, election_epoch, rotate_mode);
    bool is_leader_node = xcons_utl::xip_equals(leader_xip, local_xip);
    xunit_info("xbatch_packer::on_view_fire is_leader=%d account=%s,viewid=%ld,clock=%ld,cert_height=%ld,cert_viewid=%ld,this:%p node:%s xip:%s,leader:%s,rotate_mode:%d,timenow_ms:%llu",
            is_leader_node, get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), _cert_block->get_height(),
            _cert_block->get_viewid(), this, node_account.c_str(),
            xcons_utl::xip_to_hex(local_xip).c_str(), xcons_utl::xip_to_hex(leader_xip).c_str(), rotate_mode, timenow_ms);
    XMETRICS_GAUGE(metrics::cons_view_fire_is_leader, is_leader_node ? 1 : 0);
    if (!is_leader_node) {
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 1);
        return true;
    }

    if (view_ev->get_clock() < m_start_time) { // check again for xip changed
        xunit_warn("xbatch_packer::on_view_fire fail-clock expired less than start time.account=%s,viewid=%ld,clock=%ld,start_time=%ld",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), m_start_time);
        XMETRICS_GAUGE(metrics::cons_view_fire_clock_delay, 1);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    if (!connect_to_checkpoint()) {
        xunit_warn("xbatch_packer::on_view_fire fail-connect_to_checkpoint. account=%s,viewid=%ld,clock=%ld,cert_height=%ld", 
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), _cert_block->get_height());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);        
        return false;
    }

    m_leader_cs_para = m_proposal_maker->leader_set_consensus_para_basic(_cert_block.get(), view_ev->get_viewid(), view_ev->get_clock(), ec);    
    if (nullptr == m_leader_cs_para) {
        xunit_warn("xbatch_packer::on_view_fire fail-leader_set_consensus_para_basic. account=%s,viewid=%ld,clock=%ld,cert_height=%ld. error=%s",
            get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock(), _cert_block->get_height(), ec.message().c_str());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }
    set_xip(*m_leader_cs_para, local_xip);  // set leader xip
    set_election_round(true, *m_leader_cs_para);

    m_is_leader = true;
    m_leader_packed = start_proposal(calculate_min_tx_num(true, timenow_ms));
    if (!m_leader_packed) {
        m_raw_timer->start(m_pack_strategy.get_timer_interval(), 0);
    } else {
        m_pack_strategy.clear();
    }
    return true;
}

bool xbatch_packer::do_state_sync(uint64_t sync_height) {
    auto sync_block = get_vblockstore()->load_block_object(*this, sync_height, base::enum_xvblock_flag_committed, false);
    if (nullptr == sync_block) {
        xwarn("xbatch_packer::do_state_sync-fail load full table block.%s,height=%ld", get_account().c_str(), sync_height);
        return false;
    }

    std::error_code ec;
    auto const & state_root = data::xblockextract_t::get_state_root_from_block(sync_block.get());
    std::string table_bstate_hash_str = sync_block->get_fullstate_hash();
    xassert(!table_bstate_hash_str.empty());
    evm_common::xh256_t const table_bstate_hash(top::to_bytes(table_bstate_hash_str));
    evm_common::xh256_t const sync_block_hash(top::to_bytes(sync_block->get_block_hash()));
    xinfo("xbatch_packer::do_state_sync sync state begin.table:%s,height:%llu,root:%s", get_account().c_str(), sync_height, state_root.hex().c_str());
    get_resources()->get_state_downloader()->sync_state(m_table_addr, sync_height, sync_block_hash, table_bstate_hash, state_root, true, ec);
    if (ec) {
        xwarn("xbatch_packer::do_state_sync sync state fail.table:%s,height:%llu,root:%s ec:%s", get_account().c_str(), sync_height, state_root.hex().c_str(), ec.message().c_str());
    }
    XMETRICS_GAUGE(metrics::cons_invoke_sync_state_count, 1);
    return true;
}

bool xbatch_packer::check_state_sync(base::xvblock_t * cert_block) {
    if (cert_block == nullptr) {
        xassert(false);
        return false;
    }

    statestore::xtablestate_ext_ptr_t cert_tablestate = statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(cert_block);
    if (nullptr != cert_tablestate) {
        XMETRICS_GAUGE(metrics::cons_state_check_succ, 1);
        return true;
    }
    XMETRICS_GAUGE(metrics::cons_state_check_succ, 0);

    uint64_t latest_executed_height = statestore::xstatestore_hub_t::instance()->get_latest_executed_block_height(m_table_addr);
    if (base::xvchain_t::instance().is_storage_node()) {
        // storage node should not invoke sync, it need produce the whole mpt state
        xwarn("xbatch_packer::check_state_sync storgage node no need sync.block=%s,execute_height=%ld",cert_block->dump().c_str(), latest_executed_height);
        return false;
    }
    if (cert_block->get_last_full_block_height() == 0) {
        // no need sync,execute height will increase selfly
        xwarn("xbatch_packer::check_state_sync no need sync for no full.block=%s,execute_height=%ld,full_height=%ld",cert_block->dump().c_str(), latest_executed_height, cert_block->get_last_full_block_height());
        return false;
    }    
    if (get_resources()->get_state_downloader()->is_syncing(m_table_addr)) {
        xwarn("xbatch_packer::check_state_sync in syncing.block=%s,execute_height=%ld",cert_block->dump().c_str(), latest_executed_height);
        return false;
    }

    auto latest_committed_block = m_para->get_resources()->get_vblockstore()->load_block_object(get_account(), cert_block->get_height()-2, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_us_on_view_fire);
    if (nullptr == latest_committed_block) {
        // no need sync
        xwarn("xbatch_packer::check_state_sync fail-load commit block.block=%s",cert_block->dump().c_str());
        return false;
    }

    uint64_t latest_committed_height = latest_committed_block->get_height();
    uint64_t latest_full_height = latest_committed_block->get_block_class() == base::enum_xvblock_class_full ? latest_committed_block->get_height() : latest_committed_block->get_last_full_block_height();

    uint64_t need_state_sync_height = statestore::xstatestore_hub_t::instance()->get_need_sync_state_block_height(m_table_addr);
    if (need_state_sync_height != 0 && latest_executed_height < need_state_sync_height) {
        uint64_t sync_state_height = latest_full_height > need_state_sync_height ? latest_full_height : need_state_sync_height;
        xwarn("xbatch_packer::check_state_sync try sync state for need height.block=%s,execute=%ld,need=%ld,full=%ld,sync=%ld",cert_block->dump().c_str(), latest_executed_height, need_state_sync_height,latest_full_height,sync_state_height);
        do_state_sync(sync_state_height);
        return false;
    }

    if (latest_executed_height >= latest_full_height) {
        // execute height may behind, but no need sync
        xwarn("xbatch_packer::check_state_sync no need sync for self increase.block=%s,execute_height=%ld,commit_full_height=%ld",cert_block->dump().c_str(), latest_executed_height, latest_full_height);
        return false;
    }

    uint64_t _sync_table_state_height_gap = XGET_CONFIG(sync_table_state_height_gap);
    if (latest_executed_height + _sync_table_state_height_gap < latest_full_height) {
        xwarn("xbatch_packer::check_state_sync try sync state for need state sync.block=%s,execute=%ld,need=%ld,full=%ld",cert_block->dump().c_str(), latest_executed_height, need_state_sync_height,latest_full_height);
        do_state_sync(latest_full_height);
        return false;
    } else {
        // invoke block sync
        uint64_t lack_num = latest_full_height - latest_executed_height;
        uint32_t sync_num = lack_num < sync::max_request_block_count ? lack_num : sync::max_request_block_count;
        mbus::xevent_behind_ptr_t ev =
            make_object_ptr<mbus::xevent_behind_on_demand_t>(get_account(), latest_executed_height + 1, sync_num, false, "lack_of_table_block", "", false);
        base::xvchain_t::instance().get_xevmbus()->push_event(ev);
        xwarn("xbatch_packer::check_state_sync try sync table blocks.account=%s,full_height=%ld,commit_height=%ld,execute_height=%ld,try sync h %llu num %u",
            get_account().c_str(), latest_full_height, latest_committed_height, latest_executed_height, latest_executed_height + 1, sync_num);
        XMETRICS_GAUGE(metrics::cons_invoke_sync_block_count, 1);
    }
    return false;
}

bool  xbatch_packer::on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) {
    if (!m_is_leader || m_leader_packed) {
        return true;
    }
    // xunit_dbg("xbatch_packer::on_timer_fire retry start proposal.this:%p node:%s", this, m_para->get_resources()->get_account().c_str());
    m_leader_packed = start_proposal(calculate_min_tx_num(false, current_time_ms));
    if (!m_leader_packed) {
        m_raw_timer->start(m_pack_strategy.get_timer_interval(), 0);
    } else {
        m_pack_strategy.clear();
    }
    return true;
}

bool  xbatch_packer::on_timer_start(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) {
    // xunit_dbg("xbatch_packer::on_timer_start,this=%p", this);
    return true;
}

bool  xbatch_packer::on_timer_stop(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) {
    // xunit_dbg("xbatch_packer::on_timer_stop,this=%p", this);
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
        xunit_dbg("xbatch_packer::on_pdu_event_up %s broadcast %x from: %s to:%s",
             get_account().c_str(),
             event.get_type(),
             xcons_utl::xip_to_hex(local_xip).c_str(),
             xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, to_xip, packet, cur_thread_id, timenow_ms);
    } else {
        xunit_dbg("xbatch_packer::on_pdu_event_up %s send %x from: %s to:%s",
             get_account().c_str(),
             event.get_type(),
             xcons_utl::xip_to_hex(local_xip).c_str(),
             xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, _evt_obj->get_to_xip(), packet, cur_thread_id, timenow_ms);
    }
    return true;  // stop here
}

bool xbatch_packer::send_out(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    xunit_info("xbatch_packer::send_out pdu=%s,body_size:%d,from_xip=%s,to_xip=%s,node_xip=%s,this:%p",
                packet.dump().c_str(), packet.get_msg_body().size(), xcons_utl::xip_to_hex(from_addr).c_str(),xcons_utl::xip_to_hex(to_addr).c_str(),xcons_utl::xip_to_hex(from_addr).c_str(), this);

    auto network_proxy = m_para->get_resources()->get_network();
    xassert(network_proxy != nullptr);
    if (network_proxy != nullptr) {
        return network_proxy->send_out((uint32_t)xBFT_msg, from_addr, to_addr, packet, cur_thread_id, timenow_ms);
    }
    return false;
}

bool xbatch_packer::verify_proposal_packet(const xvip2_t & from_addr, const xvip2_t & local_addr, const base::xcspdu_t & packet) {
    bool valid = false;
    // step 1: verify viewid =》 [ local_viewid <= proposal_viewid < (local_viewid + 8)]
    auto proposal_view_id = packet.get_block_viewid();
    if (proposal_view_id >= m_last_view_id &&  proposal_view_id < (m_last_view_id + 8)) {
        // step 2: verify leader
        auto leader_election = m_para->get_resources()->get_election();
        auto accessor = m_para->get_resources()->get_data_accessor();
        std::error_code ec{election::xdata_accessor_errc_t::success};
        auto election_epoch = accessor->election_epoch_from(common::xip2_t{from_addr.low_addr, from_addr.high_addr}, ec);
        if (!ec) {
            xvip2_t leader_xip =
                leader_election->get_leader_xip(packet.get_block_viewid(), get_account(), nullptr, local_addr, from_addr, election_epoch, enum_rotate_mode_rotate_by_view_id);
            if (xcons_utl::xip_equals(leader_xip, from_addr)) {
                valid = true;
            }
        } else {
            // TODO here may happen when many elect blocks sync
            xunit_warn("xbatch_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(from_addr).c_str());
        }
    } else {
        XMETRICS_GAUGE(metrics::cons_fail_backup_view_not_match, 1);
    }
    return valid;
}

bool xbatch_packer::recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    xunit_info("xbatch_packer::recv_in, consensus_tableblock  pdu_recv_in=%s, clock=%llu, viewid=%llu, from_xip=%s,to_xip=%s,node_xip=%s.",
                packet.dump().c_str(), m_last_view_clock, m_last_view_id, xcons_utl::xip_to_hex(from_addr).c_str(),xcons_utl::xip_to_hex(to_addr).c_str(),xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
    XMETRICS_TIME_RECORD("cons_tableblock_recv_in_time_consuming");

    // proposal should pass to xbft, so xbft could realize local is beind in view/block and do sync
    bool is_leader = false;
    auto type = packet.get_msg_type();
    bool valid = true;
    if (type == xconsensus::enum_consensus_msg_type_proposal) {
        valid = verify_proposal_packet(from_addr, to_addr, packet);
    }
    if (!valid) {
        xunit_warn("xbatch_packer::recv_in fail-invalid msg,viewid=%ld,pdu=%s,at_node:%s,this:%p",
              m_last_view_id, packet.dump().c_str(), xcons_utl::xip_to_hex(to_addr).c_str(), this);
        return false;
    }
    return xcsaccount_t::recv_in(from_addr, to_addr, packet, cur_thread_id, timenow_ms);
}

int xbatch_packer::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert, xcsobject_t * _from_child) {
    XMETRICS_TIME_RECORD("cons_tableblock_verify_proposal_time_consuming");
    if (!connect_to_checkpoint()) {
        return blockmaker::xblockmaker_error_proposal_cannot_connect_to_cp;
    }

    data::xblock_consensus_para_t proposal_para(get_account(), proposal_block->get_clock(), proposal_block->get_viewid(), proposal_block->get_viewtoken(), proposal_block->get_height(), proposal_block->get_second_level_gmtime());
    set_election_round(false, proposal_para);
    auto ret = m_proposal_maker->verify_proposal(proposal_para, proposal_block, bind_clock_cert);
    if (ret == xsuccess) {
        ret = set_vote_extend_data(proposal_block, proposal_para.get_vote_extend_hash(), false);
    }
    return ret;
}

// get parent group xip
xvip2_t xbatch_packer::get_parent_xip(const xvip2_t & local_xip) {
    auto leader_election = m_para->get_resources()->get_election();
    auto election_store = leader_election->get_election_cache_face();
    xelection_cache_face::elect_set elect_set_;
    auto ret = election_store->get_parent_election(local_xip, &elect_set_);
    if (ret > 0) {
        auto xip = static_cast<xvip2_t>(elect_set_.front().address.xip2());
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
        auto xip = static_cast<xvip2_t>(elect_set_.front().address.xip2());
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
    // reset_leader_info();  // fade xip should not be leader

    // for simple, keep leadership. because in most cases,leadership not change when round changed.
    if (m_is_leader) {
        xinfo("xbatch_packer::reset_xip_addr round changed,keep leadership,account:%s,old xip:%s,new xip:%s",
              get_account().c_str(),
              xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
              xcons_utl::xip_to_hex(new_addr).c_str());
        XMETRICS_GAUGE(metrics::cons_round_changed_keep_leadership, 1);
        set_xip(*m_leader_cs_para, new_addr);  // set leader xip
        set_election_round(true, *m_leader_cs_para);
    }

    if (!is_xip2_empty(get_xip2_addr())) {
        m_last_xip2 = get_xip2_addr();
    }
    xinfo("xbatch_packer::reset_xip_addr %s %s,last xip:%s node:%s this:%p", get_account().c_str(), xcons_utl::xip_to_hex(new_addr).c_str(), xcons_utl::xip_to_hex(m_last_xip2).c_str(), m_para->get_resources()->get_account().c_str(), this);
    return xcsaccount_t::reset_xip_addr(new_addr);
}

bool xbatch_packer::set_fade_xip_addr(const xvip2_t & new_addr) {
    if (xcons_utl::xip_equals(new_addr, get_xip2_addr())) {
        reset_leader_info();  // fade xip should not be leader
    }
    xdbg("xbatch_packer::set_fade_xip_addr set fade xip from %s to %s", xcons_utl::xip_to_hex(m_faded_xip2).c_str(), xcons_utl::xip_to_hex(new_addr).c_str());
    m_faded_xip2 = new_addr;
    return true;
}

bool xbatch_packer::set_start_time(const common::xlogic_time_t& start_time) {
    xunit_dbg("xbatch_packer::set_start_time %d node:%s this:%p", start_time, m_para->get_resources()->get_account().c_str(), this);
    m_start_time = start_time;
    return true;
}

bool xbatch_packer::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_auditor())
                  || xcons_utl::xip_equals(m_last_xip2, _evt_obj->get_target_proposal()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(m_last_xip2, _evt_obj->get_target_proposal()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() != xconsensus::enum_xconsensus_code_successful) {

        // accumulated table failed value
        auto fork_tag = "cons_table_failed_accu_" + get_account();
        XMETRICS_COUNTER_INCREMENT( fork_tag , 1);

        XMETRICS_GAUGE(metrics::cons_tableblock_total_succ, 0);
        if (is_leader) {
            XMETRICS_GAUGE(metrics::cons_tableblock_leader_succ, 0);
            auto error_tag = "cons_table_failed_error_code_" + std::to_string(_evt_obj->get_error_code());
            XMETRICS_COUNTER_INCREMENT(error_tag, 1);  
        } else {
            XMETRICS_GAUGE(metrics::cons_tableblock_backup_succ, 0);
        }
         xunit_warn("xbatch_packer::on_proposal_finish fail. leader:%d,error_code:%d,proposal=%s,at_node:%s,m_last_xip2:%s",
             is_leader,
             _evt_obj->get_error_code(),
             _evt_obj->get_target_proposal()->dump().c_str(),
             xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
             xcons_utl::xip_to_hex(m_last_xip2).c_str());
    } else {

        // reset to 0
        auto fork_tag = "cons_table_failed_accu_" + get_account();
        XMETRICS_COUNTER_SET( fork_tag , 0);

        xunit_info("xbatch_packer::on_proposal_finish succ. leader:%d,proposal=%s,state_root=%s,at_node:%s,m_last_xip2:%s",
            is_leader,
            _evt_obj->get_target_proposal()->dump().c_str(),
            data::xblockextract_t::get_state_root_from_block(_evt_obj->get_target_proposal()).hex().c_str(),
            xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
            xcons_utl::xip_to_hex(m_last_xip2).c_str());

        base::xvblock_t *vblock = _evt_obj->get_target_proposal();
        xassert(vblock->is_body_and_offdata_ready(false));

        if (vblock->get_excontainer() != nullptr) {
            vblock->get_excontainer()->commit(vblock);
        }
        vblock->add_ref();
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_consensus_data_t>(vblock, is_leader);
        m_para->get_resources()->get_bus()->push_event(ev);

        XMETRICS_GAUGE(metrics::cons_tableblock_total_succ, 1);
        if (is_leader) {
            XMETRICS_GAUGE(metrics::cons_tableblock_leader_succ, 1);
            if (vblock->get_height() > 2) {
                send_receipts(vblock);
            }
        } else {
            XMETRICS_GAUGE(metrics::cons_tableblock_backup_succ, 1);
        }
    }
    return false;  // throw event up again to let txs-pool or other object start new consensus
}
bool  xbatch_packer::on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //call from lower layer to higher layer(parent)
{
    xcsaccount_t::on_replicate_finish(event, from_child, cur_thread_id, timenow_ms);

    xconsensus::xreplicate_finish * _evt_obj = (xconsensus::xreplicate_finish*)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_block()->get_cert()->get_validator())
                  || xcons_utl::xip_equals(xip, _evt_obj->get_target_block()->get_cert()->get_auditor());    
    if(_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful)
    {
        base::xvblock_t *vblock = _evt_obj->get_target_block();
        xassert(vblock->is_body_and_offdata_ready(false));
        vblock->add_ref();
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_consensus_data_t>(vblock, is_leader);
        m_para->get_resources()->get_bus()->push_event(ev);
    }
    return true; //stop handle anymore
}
bool xbatch_packer::on_consensus_commit(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_consensus_commit(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xconsensus_commit * _evt_obj = (xconsensus::xconsensus_commit *)&event;
    xunit_dbg("xbatch_packer::on_consensus_commit, %s class=%d, at_node:%s",
        _evt_obj->get_target_commit()->dump().c_str(), _evt_obj->get_target_commit()->get_block_class(), xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

void xbatch_packer::make_receipts_and_send(data::xblock_t * commit_block, data::xblock_t * cert_block) {
    // broadcast receipt id state to all shards
    if (commit_block->get_block_class() == base::enum_xvblock_class_full || commit_block->get_block_class() == base::enum_xvblock_class_nil) {
        return;
    }

    auto network_proxy = m_para->get_resources()->get_network();
    xassert(network_proxy != nullptr);
    if (network_proxy == nullptr) {
        xunit_warn("xbatch_packer::make_receipts_and_send get network fail, can not send receipts");
        return;
    }

    std::vector<data::xcons_transaction_ptr_t> all_cons_txs = data::xblocktool_t::create_all_txreceipts(commit_block, cert_block);
    if (all_cons_txs.empty()) {
        xunit_info("xbatch_packer::make_receipts_and_send no receipt created,commit_block:%s", commit_block->dump().c_str());
        return;
    }

    xunit_info("xbatch_packer::make_receipts_and_send commit_block:%s,cert_block:%s,txreceipts_size=%zu", commit_block->dump().c_str(), cert_block->dump().c_str(), all_cons_txs.size());
    std::vector<data::xcons_transaction_ptr_t> non_shard_cross_receipts;
    network_proxy->send_receipt_msgs(get_xip2_addr(), all_cons_txs, non_shard_cross_receipts);

    for (auto & tx : non_shard_cross_receipts) {
        xtxpool_v2::xtx_para_t para;
        std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(tx, para);
        m_para->get_resources()->get_txpool()->push_receipt(tx_ent, true, false);
        XMETRICS_GAUGE(metrics::txpool_received_self_send_receipt_num, 1);
    }
}

uint32_t xbatch_packer::calculate_min_tx_num(bool first_packing, uint64_t time_ms) {
    xunit_dbg("xbatch_packer::calculate_min_tx_num account:%s,viewid:%llu,first_packing:%d,time_ms:%llu", get_account().c_str(), m_last_view_id, first_packing, time_ms);
    if (first_packing) {
        return m_pack_strategy.get_tx_num_threshold_first_time(time_ms);
    } else {
        return m_pack_strategy.get_tx_num_threshold(time_ms);
    }
}

int32_t xbatch_packer::set_vote_extend_data(base::xvblock_t * proposal_block, const uint256_t & hash, bool is_leader) {
    // do nothing for normal batch packer
    return xsuccess;
}

void xbatch_packer::clear_for_new_view() {
    // do nothing for normal batch packer
}

void xbatch_packer::send_receipts(base::xvblock_t *vblock) {
    base::xauto_ptr<base::xvblock_t> commit_block =
        m_para->get_resources()->get_vblockstore()->load_block_object(*this, vblock->get_height() - 2, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_us_on_proposal_finish);
    if (commit_block != nullptr) {
        m_para->get_resources()->get_vblockstore()->load_block_input(*this, commit_block.get());
        make_receipts_and_send(dynamic_cast<data::xblock_t *>(commit_block.get()), dynamic_cast<data::xblock_t *>(vblock));
    }
}

bool xbatch_packer::set_election_round(bool is_leader, data::xblock_consensus_para_t & proposal_para) {
    return true;
}

void xpack_strategy_t::clear() {
    m_vc_time_ms = 0;
}

uint32_t xpack_strategy_t::get_tx_num_threshold_first_time(uint64_t vc_time_ms) {
    m_vc_time_ms = vc_time_ms;
    return MIN_TRANSACTION_NUM_FOR_HIGH_TPS;
}

int32_t xpack_strategy_t::get_timer_interval() const {
    return TRY_MAKE_BLOCK_TIMER_INTERVAL;
}

uint32_t xpack_strategy_t::get_tx_num_threshold(uint64_t cur_time) const {
    if (m_vc_time_ms == 0 || cur_time <= m_vc_time_ms) {
        xerror("xpack_strategy_t::get_tx_num_threshold vc_time:%llu and cur_time:%llu invalid", m_vc_time_ms, cur_time);
        return 0;
    }
    uint64_t time_diff = cur_time - m_vc_time_ms;

    if (time_diff <= TRY_HIGH_TPS_TIME_WINDOW) {
        return MIN_TRANSACTION_NUM_FOR_HIGH_TPS;
    } else if (time_diff <= TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW) {
        return MIN_TRANSACTION_NUM_FOR_MIDDLE_TPS;
    } else if (time_diff <= TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW) {
        return MIN_TRANSACTION_NUM_FOR_LOW_TPS;
    } else {
        return 0;
    }
}

NS_END2
