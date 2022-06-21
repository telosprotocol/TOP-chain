// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xrelay_packer.h"

#include "xBFT/xconsevent.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xblockmaker/xrelay_proposal_maker.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_type.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblockextract.h"
#include "xelection/xdata_accessor_error.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_consensus.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xrelay_packer.h"
#include "xverifier/xverifier_utl.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)

#define MIN_TRANSACTION_NUM_FOR_FIRST_PACKING (10)

xrelay_packer::xrelay_packer(const std::string & account_id,
                             std::shared_ptr<xcons_service_para_face> const & para,
                             std::shared_ptr<xblock_maker_face> const & block_maker,
                             base::xcontext_t & _context,
                             const uint32_t target_thread_id)
  : xcsaccount_t(_context, target_thread_id, account_id), m_last_view_id(0), m_para(para), m_account_id(account_id) {
    auto cert_auth = m_para->get_resources()->get_certauth();
    m_last_xip2.high_addr = -1;
    m_last_xip2.low_addr = -1;
    register_plugin(cert_auth);
    auto store = m_para->get_resources()->get_vblockstore();
    set_vblockstore(store);
    register_plugin(store);
    base::xauto_ptr<xcsobject_t> ptr_engine_obj(create_engine(*this, xconsensus::enum_xconsensus_pacemaker_type_clock_cert));
    ptr_engine_obj->register_plugin(para->get_resources()->get_xbft_workpool());  // used for xbft heavy work, such as verify_proposal and signature verify.
    m_raw_timer = get_thread()->create_timer((base::xtimersink_t *)this);
    m_raw_timer->start(1000, 1000);
    m_proposal_maker = block_maker->get_proposal_maker(account_id);
    para->get_resources()->get_relay_chain_mgr()->start(target_thread_id);

    xunit_dbg("xrelay_packer::xrelay_packer,create,this=%p,account=%s", this, account_id.c_str());
}

xrelay_packer::~xrelay_packer() {
    if (m_raw_timer != nullptr) {
        m_raw_timer->release_ref();
    }
    xunit_dbg("xrelay_packer::~xrelay_packer,destory,this=%p", this);
}

bool xrelay_packer::close(bool force_async) {
    m_para->get_resources()->get_relay_chain_mgr()->stop();
    xcsaccount_t::close(force_async);
    // xunit_dbg("xrelay_packer::close, this=%p,refcount=%d", this, get_refcount());
    return true;
}

bool xrelay_packer::on_object_close() {
    // xunit_dbg("xrelay_packer::on_object_close this=%p,refcount=%d", this, get_refcount());
    if (m_raw_timer != nullptr) {
        m_raw_timer->stop();
        m_raw_timer->close();
    }
    return xcsaccount_t::on_object_close();
}

void xrelay_packer::set_xip(data::xblock_consensus_para_t & blockpara, const xvip2_t & leader) {
    blockpara.set_xip(leader, xvip2_t{(uint64_t)0, (uint64_t)0});
}

bool xrelay_packer::start_proposal(base::xblock_mptrs & latest_blocks) {
    if (latest_blocks.get_latest_cert_block() == nullptr || latest_blocks.get_latest_locked_block() == nullptr ||
        latest_blocks.get_latest_committed_block() == nullptr) {  // TODO(jimmy) get_latest_blocks return bool future
        xunit_warn("xrelay_packer::start_proposal fail-get latest blocks,account=%s,viewid=%ld,clock=%ld", get_account().c_str(), m_last_view_id, m_last_view_clock);
        return false;
    }

    uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
    // uint64_t gmtime = base::xtime_utl::gettimeofday();
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    uint64_t now = static_cast<uint64_t>(val.tv_sec);
    data::xblock_consensus_para_t proposal_para(get_account(), m_last_view_clock, m_last_view_id, viewtoken, latest_blocks.get_latest_cert_block()->get_height() + 1, now);
    proposal_para.set_latest_blocks(latest_blocks);

    if (m_last_view_clock < m_start_time) {
        xunit_warn("xrelay_packer::start_proposal fail-view clock less than start.%s,start_time=%ld", proposal_para.dump().c_str(), m_start_time);
        return false;
    }

    // TODO(jimmy) performance optimize, get cert/lock/connectted directly in future
    auto latest_connected_block = m_para->get_resources()->get_vblockstore()->get_latest_connected_block(
        get_account(), metrics::blockstore_access_from_us_on_timer_fire);  // TODO(jimmy) just invoke connect flag update
    if (latest_connected_block == nullptr) {                               // TODO(jimmy) get_latest_blocks return bool future
        xunit_error("xrelay_packer::start_proposal fail-get latest connect block,%s", proposal_para.dump().c_str());
        return false;
    }
    if (latest_connected_block->get_height() != latest_blocks.get_latest_committed_block()->get_height()) {  // TODO(jimmy) get_latest_blocks return bool future
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        xunit_warn("xrelay_packer::start_proposal fail-latest connect not match committed block,%s,connect_height=%ld",
                   proposal_para.dump().c_str(),
                   latest_connected_block->get_height());
        return false;
    }

    if (false == m_proposal_maker->can_make_proposal(proposal_para)) {
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        xunit_warn("xrelay_packer::start_proposal fail-cannot make proposal.%s", proposal_para.dump().c_str());
        return false;
    }

    auto local_xip = get_xip2_addr();
    set_xip(proposal_para, local_xip);  // set leader xip

    auto network_proxy = m_para->get_resources()->get_network();
    if (network_proxy == nullptr) {
        xerror("xrelay_packer::start_proposal network_proxy is null");
        return false;
    }

    uint64_t election_round = 0;
    if (!network_proxy->get_election_round(local_xip, election_round)) {
        xerror("xrelay_packer::start_proposal get_election_round fail");
        return false;
    }

    proposal_para.set_election_round(election_round);

    xunit_dbg_info(
        "xrelay_packer::start_proposal leader begin make_proposal.%s cert_block_viewid=%ld", proposal_para.dump().c_str(), latest_blocks.get_latest_cert_block()->get_viewid());
    data::xblock_ptr_t proposal_block = m_proposal_maker->make_proposal(proposal_para, 0);
    if (proposal_block == nullptr) {
        xunit_dbg("xrelay_packer::start_proposal fail-make_proposal.%s", proposal_para.dump().c_str());  // may has no txs for proposal
        return false;
    }

    set_vote_extend_data(proposal_block.get());

    base::xauto_ptr<xconsensus::xproposal_start> _event_obj(new xconsensus::xproposal_start(proposal_block.get()));
    push_event_down(*_event_obj, this, 0, 0);
    // check viewid again, may changed
    if (m_last_view_id != proposal_block->get_viewid()) {
        xunit_warn("xrelay_packer::start_proposal fail-finally viewid changed. %s latest_viewid=%" PRIu64 "", proposal_para.dump().c_str(), proposal_block->get_viewid());
        return false;
    }

    XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 1);
    xunit_info("xrelay_packer::start_proposal succ-leader start consensus. block=%s this:%p node:%s xip:%s",
               proposal_block->dump().c_str(),
               this,
               m_para->get_resources()->get_account().c_str(),
               xcons_utl::xip_to_hex(local_xip).c_str());

    return true;
}

// view updated and the judge is_leader
// then start new consensus from leader
bool xrelay_packer::on_view_fire(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    auto view_ev = dynamic_cast<const xconsensus::xcsview_fire *>(&event);
    xassert(view_ev != nullptr);
    xassert(view_ev->get_viewid() >= m_last_view_id);
    xassert(view_ev->get_account() == get_account());
    m_is_leader = false;
    m_leader_packed = false;
    xdbg_info("xrelay_packer::on_view_fire account=%s,clock=%ld,viewid=%ld,start_time=%ld", get_account().c_str(), view_ev->get_clock(), view_ev->get_viewid(), m_start_time);

    auto local_xip = get_xip2_addr();
    if (xcons_utl::xip_equals(m_faded_xip2, local_xip)) {
        xdbg_info("xrelay_packer::on_view_fire local_xip equal m_fade_xip2 %s . fade round should not make proposal", xcons_utl::xip_to_hex(m_faded_xip2).c_str());
        return false;
    }

    // fix: viewchange on different rounds
    if (view_ev->get_clock() < m_start_time) {
        xunit_warn("xrelay_packer::on_view_fire fail-clock expired less than start time.account=%s,viewid=%ld,clock=%ld,start_time=%ld",
                   get_account().c_str(),
                   view_ev->get_viewid(),
                   view_ev->get_clock(),
                   m_start_time);
        XMETRICS_GAUGE(metrics::cons_view_fire_clock_delay, 1);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    if (view_ev->get_clock() + 2 < m_para->get_resources()->get_chain_timer()->logic_time()) {
        xunit_warn("xrelay_packer::on_view_fire fail-clock expired less than logic time.account=%s,viewid=%ld,clock=%ld,logic_time=%ld",
                   get_account().c_str(),
                   view_ev->get_viewid(),
                   view_ev->get_clock(),
                   m_para->get_resources()->get_chain_timer()->logic_time());
        XMETRICS_GAUGE(metrics::cons_view_fire_clock_delay, 1);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    // clear relay multisign for new view.
    m_relay_multisign.clear();

    XMETRICS_TIME_RECORD("cons_tableblock_view_change_time_consuming");
    m_last_view_id = view_ev->get_viewid();
    m_last_view_clock = view_ev->get_clock();
    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(get_account(), metrics::blockstore_access_from_us_on_view_fire);
    if (latest_blocks.get_latest_cert_block() == nullptr) {
        xunit_warn("xrelay_packer::on_view_fire fail-invalid latest blocks,account=%s,viewid=%ld,clock=%ld", get_account().c_str(), view_ev->get_viewid(), view_ev->get_clock());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }
    if (m_last_view_clock < latest_blocks.get_latest_cert_block()->get_clock()) {
        xunit_warn("xrelay_packer::on_view_fire fail-clock less than cert block,account=%s,viewid=%ld,clock=%ld,prev=%ull",
                   get_account().c_str(),
                   view_ev->get_viewid(),
                   view_ev->get_clock(),
                   latest_blocks.get_latest_cert_block()->get_clock());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    auto accessor = m_para->get_resources()->get_data_accessor();
    auto leader_election = m_para->get_resources()->get_election();
    auto node_account = m_para->get_resources()->get_account();

    auto zone_id = get_zone_id_from_xip2(local_xip);
    if (zone_id != base::enum_chain_zone_relay_index) {
        xerror("xrelay_packer::on_view_fire fail-wrong zone id. zoneid=%d", zone_id);
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }

    // check if this node is leader
    std::error_code ec{election::xdata_accessor_errc_t::success};
    auto election_epoch = accessor->election_epoch_from(common::xip2_t{local_xip.low_addr, local_xip.high_addr}, ec);
    if (ec) {
        xunit_warn("xrelay_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(local_xip).c_str());
        XMETRICS_GAUGE(metrics::cons_view_fire_succ, 0);
        return false;
    }
    XMETRICS_GAUGE(metrics::cons_view_fire_succ, 1);

    uint16_t rotate_mode = enum_rotate_mode_rotate_by_view_id;
    xvip2_t leader_xip = leader_election->get_leader_xip(m_last_view_id, get_account(), latest_blocks.get_latest_cert_block(), local_xip, local_xip, election_epoch, rotate_mode);
    bool is_leader_node = xcons_utl::xip_equals(leader_xip, local_xip);
    xunit_info("xrelay_packer::on_view_fire is_leader=%d account=%s,viewid=%ld,clock=%ld,cert_height=%ld,cert_viewid=%ld,this:%p node:%s xip:%s,leader:%s,rotate_mode:%d",
               is_leader_node,
               get_account().c_str(),
               view_ev->get_viewid(),
               view_ev->get_clock(),
               latest_blocks.get_latest_cert_block()->get_height(),
               latest_blocks.get_latest_cert_block()->get_clock(),
               this,
               node_account.c_str(),
               xcons_utl::xip_to_hex(local_xip).c_str(),
               xcons_utl::xip_to_hex(leader_xip).c_str(),
               rotate_mode);
    XMETRICS_GAUGE(metrics::cons_view_fire_is_leader, is_leader_node ? 1 : 0);
    if (!is_leader_node) {
        return true;
    }

    m_is_leader = true;

    if (latest_blocks.get_latest_cert_block()->get_height() == 0) {
        m_leader_packed = start_proposal(latest_blocks);
    }
    return true;
}

bool xrelay_packer::on_timer_fire(const int32_t thread_id,
                                  const int64_t timer_id,
                                  const int64_t current_time_ms,
                                  const int32_t start_timeout_ms,
                                  int32_t & in_out_cur_interval_ms) {
    m_para->get_resources()->get_relay_chain_mgr()->on_timer();
    if (!m_is_leader || m_leader_packed) {
        return true;
    }

    // if (m_wait_count < 10) {
    //     m_wait_count++;
    //     return true;
    // } else {
    //     m_wait_count = 0;
    // }

    // xunit_dbg("xrelay_packer::on_timer_fire retry start proposal.this:%p node:%s", this, m_para->get_resources()->get_account().c_str());
    base::xblock_mptrs latest_blocks = m_para->get_resources()->get_vblockstore()->get_latest_blocks(get_account(), metrics::blockstore_access_from_us_on_timer_fire);
    m_leader_packed = start_proposal(latest_blocks);
    return true;
}

bool xrelay_packer::on_timer_start(const int32_t errorcode,
                                   const int32_t thread_id,
                                   const int64_t timer_id,
                                   const int64_t cur_time_ms,
                                   const int32_t timeout_ms,
                                   const int32_t timer_repeat_ms) {
    // xunit_dbg("xrelay_packer::on_timer_start,this=%p", this);
    return true;
}

bool xrelay_packer::on_timer_stop(const int32_t errorcode,
                                  const int32_t thread_id,
                                  const int64_t timer_id,
                                  const int64_t cur_time_ms,
                                  const int32_t timeout_ms,
                                  const int32_t timer_repeat_ms) {
    // xunit_dbg("xrelay_packer::on_timer_stop,this=%p", this);
    return true;
}

bool xrelay_packer::on_pdu_event_up(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xconsensus::xcspdu_fire * _evt_obj = (xconsensus::xcspdu_fire *)&event;
    auto packet = _evt_obj->_packet;
    auto local_xip = get_xip2_addr();
    xassert(packet.get_block_account() == get_account());
    if (xcons_utl::is_broadcast_address(_evt_obj->get_to_xip())) {
        // leader broadcast msg
        auto to_xip = local_xip;
        set_node_id_to_xip2(to_xip, common::xbroadcast_slot_id_value);
        xunit_dbg("xrelay_packer::on_pdu_event_up %s broadcast %x from: %s to:%s",
                  get_account().c_str(),
                  event.get_type(),
                  xcons_utl::xip_to_hex(local_xip).c_str(),
                  xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, to_xip, packet, cur_thread_id, timenow_ms);
    } else {
        xunit_dbg("xrelay_packer::on_pdu_event_up %s send %x from: %s to:%s",
                  get_account().c_str(),
                  event.get_type(),
                  xcons_utl::xip_to_hex(local_xip).c_str(),
                  xcons_utl::xip_to_hex(_evt_obj->get_to_xip()).c_str());
        return send_out(local_xip, _evt_obj->get_to_xip(), packet, cur_thread_id, timenow_ms);
    }
    return true;  // stop here
}

bool xrelay_packer::send_out(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    xunit_info(
        "xrelay_packer::send_out pdu=%s,body_size:%d,node_xip=%s,this:%p", packet.dump().c_str(), packet.get_msg_body().size(), xcons_utl::xip_to_hex(from_addr).c_str(), this);

    auto network_proxy = m_para->get_resources()->get_network();
    xassert(network_proxy != nullptr);
    if (network_proxy != nullptr) {
        return network_proxy->send_out((uint32_t)xrelay_BFT_msg, from_addr, to_addr, packet, cur_thread_id, timenow_ms);
    }
    return false;
}

bool xrelay_packer::verify_proposal_packet(const xvip2_t & from_addr, const xvip2_t & local_addr, const base::xcspdu_t & packet) {
    bool valid = false;
    // step 1: verify viewid =》 [ local_viewid <= proposal_viewid < (local_viewid + 8)]
    auto proposal_view_id = packet.get_block_viewid();
    if (proposal_view_id >= m_last_view_id && proposal_view_id < (m_last_view_id + 8)) {
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
            xunit_warn("xrelay_packer::on_view_fire xip=%s version from error", xcons_utl::xip_to_hex(from_addr).c_str());
        }
    } else {
        XMETRICS_GAUGE(metrics::cons_fail_backup_view_not_match, 1);
    }
    return valid;
}

bool xrelay_packer::recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    xunit_info("xrelay_packer::recv_in, consensus_tableblock  pdu_recv_in=%s, clock=%llu, viewid=%llu, node_xip=%s.",
               packet.dump().c_str(),
               m_last_view_clock,
               m_last_view_id,
               xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
    XMETRICS_TIME_RECORD("cons_tableblock_recv_in_time_consuming");

    // proposal should pass to xbft, so xbft could realize local is beind in view/block and do sync
    bool is_leader = false;
    auto type = packet.get_msg_type();
    bool valid = true;
    if (type == xconsensus::enum_consensus_msg_type_proposal) {
        valid = verify_proposal_packet(from_addr, to_addr, packet);
    }
    if (!valid) {
        xunit_warn(
            "xrelay_packer::recv_in fail-invalid msg,viewid=%ld,pdu=%s,at_node:%s,this:%p", m_last_view_id, packet.dump().c_str(), xcons_utl::xip_to_hex(to_addr).c_str(), this);
        return false;
    }
    return xcsaccount_t::recv_in(from_addr, to_addr, packet, cur_thread_id, timenow_ms);
}

void xrelay_packer::set_vote_extend_data(base::xvblock_t * proposal_block) {
    top::uint256_t hash_0;
    top::uint256_t sign_hash = proposal_block->get_vote_extend_hash();
    if (sign_hash == hash_0) {
        return;
    }

    auto prikey_str = get_vcertauth()->get_prikey(get_xip2_addr());
    uint8_t priv_content[xverifier::PRIKEY_LEN];
    memcpy(priv_content, prikey_str.data(), prikey_str.size());
    top::utl::xecprikey_t ecpriv(priv_content);

    auto signature = ecpriv.sign(sign_hash);
    std::string signature_str = std::string((char *)signature.get_compact_signature(), signature.get_compact_signature_size());
    xdbg("xrelay_packer::set_vote_extend_data signer:%s", get_vcertauth()->get_signer(get_xip2_addr()).c_str());
    if (m_is_leader) {
        top::utl::xecpubkey_t pub_key_obj = ecpriv.get_public_key();
        std::string pubkey_str = std::string((char *)(pub_key_obj.data() + 1), 64);
        m_relay_multisign[pubkey_str] = std::make_pair(get_xip2_addr(), signature_str);
    } else {
        proposal_block->set_vote_extend_data(signature_str);
    }

    // // for test
    // top::utl::xecpubkey_t pub_key_obj = ecpriv.get_public_key();

    // uint8_t signature_content[65];
    // memcpy(signature_content, signature_str.data(), signature_str.size());

    // utl::xecdsasig_t signature1(signature_content);
    // bool verify_ret = pub_key_obj.verify_signature(signature1, sign_hash);
    // if (verify_ret) {
    //     xdbg("nathan test verify_signature ok");
    // } else {
    //     xerror("nathan test verify_signature fail");
    // }
}

int xrelay_packer::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert, xcsobject_t * _from_child) {
    XMETRICS_TIME_RECORD("cons_tableblock_verify_proposal_time_consuming");
    auto ret = m_proposal_maker->verify_proposal(proposal_block, bind_clock_cert);
    if (ret == xsuccess) {
        set_vote_extend_data(proposal_block);
    }

    return ret;
}

bool xrelay_packer::verify_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, std::string & result) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }

    if (vote_extend_data.empty()) {
        xerror("xrelay_packer::verify_vote_extend_data,fail-vote_extend_data empty for proposal=%s", proposal_block->dump().c_str());
        return true;
    }

    auto & inner_hash = proposal_block->get_vote_extend_hash();

    const std::string account_addr_of_node = get_vcertauth()->get_signer(replica_xip);
    utl::xkeyaddress_t key_address(account_addr_of_node);

    utl::xecdsasig_t signature_obj((uint8_t *)vote_extend_data.c_str());

    uint8_t addr_type = 0;
    uint16_t net_id = 0;
    if (false == key_address.get_type_and_netid(addr_type, net_id)) {
        xerror("xrelay_packer::verify_vote_extend_data get type and netid fail,proposal=%s,node:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
        return false;
    }

    uint8_t out_publickey_data[65] = {0};
    if (top::utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, inner_hash, out_publickey_data)) {
        top::utl::xecpubkey_t verify_key(out_publickey_data);
        if (verify_key.to_address(addr_type, net_id) != account_addr_of_node) {
            xerror("xrelay_packer::verify_vote_extend_data adress not feat,proposal=%s,node:%s, verify_key.to_address(addr_type, net_id):%s",
                   proposal_block->dump().c_str(),
                   account_addr_of_node.c_str(),
                   verify_key.to_address(addr_type, net_id).c_str());
            return false;
        }

    } else {
        xerror("xrelay_packer::verify_vote_extend_data get_publickey_from_signature fail,proposal=%s,node:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
        return false;
    }

    result = std::string((char *)(out_publickey_data + 1), 64);
    xdbg("xrelay_packer::verify_vote_extend_data verify sign succ.proposal_block:%s,signer:%s", proposal_block->dump().c_str(), account_addr_of_node.c_str());
    return true;
}

void xrelay_packer::add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return;
    }

    m_relay_multisign[result] = std::make_pair(replica_xip, vote_extend_data);
    xdbg("xrelay_packer::add_vote_extend_data proposal:%s m_inner_voted_datas size:%u", proposal_block->dump().c_str(), m_relay_multisign.size());
}

bool xrelay_packer::proc_vote_complate(base::xvblock_t * proposal_block) {
    if ((proposal_block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }

    if (m_relay_multisign.empty()) {
        xerror("xrelay_packer::proc_vote_complate m_relay_multisign is empty,proposal:%s", proposal_block->dump().c_str());
        return false;
    }
    auto network_proxy = m_para->get_resources()->get_network();
    if (network_proxy == nullptr) {
        xerror("xrelay_packer::proc_vote_complate network_proxy is null,proposal:%s", proposal_block->dump().c_str());
        return false;
    }

    std::error_code ec;
    data::xrelay_block relay_block;
    data::xblockextract_t::unpack_relayblock(proposal_block, false, relay_block, ec);
    if (ec) {
        xwarn("xrelay_packer::proc_vote_complate last_relay_block decodeBytes,proposal:%s,error %s; err msg %s",
              proposal_block->dump().c_str(),
              ec.category().name(),
              ec.message().c_str());
        return false;
    }
    uint64_t election_round = relay_block.get_inner_header().get_epochID();

    // order multisign by election info.
    std::vector<data::xrelay_election_node_t> reley_election;
    auto ret = m_para->get_resources()->get_relay_chain_mgr()->get_elect_cache(election_round, reley_election);
    if (!ret) {
        xerror("xrelay_packer::proc_vote_complate get elect cache fail.proposal:%s,round:%lu", proposal_block->dump().c_str(), election_round);
        return false;
    }

    base::xstream_t _stream(base::xcontext_t::instance());
    uint16_t size = reley_election.size();
    _stream << size;

    uint32_t num = 0;
    uint32_t i = 0;
    for (auto & node : reley_election) {
        auto it = m_relay_multisign.find(node.get_pubkey_str());
        if (it == m_relay_multisign.end()) {
            std::string empty_str = "";
            _stream << empty_str;
            xdbg("xrelay_packer::proc_vote_complate not found in election.proposal:%s,elect idx:%u", proposal_block->dump().c_str(), i);
        } else {
            _stream << it->second.second;
            num++;
            xdbg("xrelay_packer::proc_vote_complate found in election.proposal:%s,elect idx:%u", proposal_block->dump().c_str(), i);
        }
        i++;
    }
    if (num != m_relay_multisign.size()) {
        xerror("xrelay_packer::proc_vote_complate relay multisign not match with election.proposal:%s,match num:%u, multisign num:%u",
               proposal_block->dump().c_str(),
               num,
               m_relay_multisign.size());
        return false;
    }

    xdbg("xrelay_packer::proc_vote_complate.proposal:%s,sign num:%u, election num:%u", proposal_block->dump().c_str(), num, reley_election.size());
    std::string extend_data = std::string((char *)_stream.data(), _stream.size());
    proposal_block->set_extend_data(extend_data);
    return true;
}

bool xrelay_packer::verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data) {
    if ((block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_vote) == 0) {
        return true;
    }
    std::error_code ec;
    data::xrelay_block relay_block;
    data::xblockextract_t::unpack_relayblock(block, false, relay_block, ec);
    if (ec) {
        xwarn(
            "xrelay_proposal_maker_t:make_proposal last_relay_block decodeBytes block:%s,error %s; err msg %s", block->dump().c_str(), ec.category().name(), ec.message().c_str());
        return false;
    }

    uint64_t election_round = relay_block.get_inner_header().get_epochID();
    auto hash = relay_block.get_block_hash();
    uint256_t hash256 = from_bytes<uint256_t>(hash.to_bytes());

    std::vector<data::xrelay_election_node_t> reley_election;
    auto ret = m_para->get_resources()->get_relay_chain_mgr()->get_elect_cache(election_round, reley_election);
    if (!ret) {
        xerror("xrelay_packer::verify_commit_msg_extend_data get elect cache fail.block:%s,round:%lu", block->dump().c_str(), election_round);
        return false;
    }

    // check if multisignature is match with election data.
    base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)extend_data.data(), static_cast<uint32_t>(extend_data.size())};
    uint16_t size = 0;
    stream >> size;
    if (size != reley_election.size()) {
        xerror("xrelay_packer::verify_commit_msg_extend_data block:%,smultisign size not match:%u:%u", block->dump().c_str(), size, reley_election.size());
        return false;
    }

    uint32_t num = 0;
    for (uint16_t i = 0; i < size; i++) {
        std::string signature;
        stream >> signature;

        if (signature == "") {
            xdbg("xrelay_packer::verify_commit_msg_extend_data,block:%,signature[%u] is empty", block->dump().c_str(), i);
            continue;
        }

        utl::xecdsasig_t signature_obj((uint8_t *)signature.c_str());
        uint8_t out_publickey_data[65] = {0};
        if (!top::utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, hash256, out_publickey_data)) {
            xerror("xrelay_packer::verify_commit_msg_extend_data get_publickey_from_signature fail,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        if (std::string((char *)(out_publickey_data + 1), 64) != reley_election[i].get_pubkey_str()) {
            xerror("xrelay_packer::verify_commit_msg_extend_data pubkey not match,block:%s,i:%d", block->dump().c_str(), i);
            return false;
        }

        num++;
        xdbg("xrelay_packer::verify_commit_msg_extend_data,block:%s,num:%u,signature[%u]:%s", block->dump().c_str(), num, i, signature.c_str());
    }
    // todo(nathan): signature number check.

    xdbg("xrelay_packer::verify_commit_msg_extend_data ok,block:%s,elect size:%d,sign num:%d", block->dump().c_str(), size, num);
    return true;
}

// get parent group xip
xvip2_t xrelay_packer::get_parent_xip(const xvip2_t & local_xip) {
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

xvip2_t xrelay_packer::get_child_xip(const xvip2_t & local_xip, const std::string & account) {
    auto child_group_id = xcons_utl::get_groupid_by_account(local_xip, account);
    xelection_cache_face::elect_set elect_set_;
    auto leader_election = m_para->get_resources()->get_election();
    auto election_store = leader_election->get_election_cache_face();
    auto ret = election_store->get_group_election(local_xip, child_group_id, &elect_set_);
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

bool xrelay_packer::reset_xip_addr(const xvip2_t & new_addr) {
    if (!is_xip2_empty(get_xip2_addr())) {
        m_last_xip2 = get_xip2_addr();
    }
    xunit_dbg("xrelay_packer::reset_xip_addr %s,last xip:%s node:%s this:%p",
              xcons_utl::xip_to_hex(new_addr).c_str(),
              xcons_utl::xip_to_hex(m_last_xip2).c_str(),
              m_para->get_resources()->get_account().c_str(),
              this);
    return xcsaccount_t::reset_xip_addr(new_addr);
}

bool xrelay_packer::set_fade_xip_addr(const xvip2_t & new_addr) {
    xdbg("xrelay_packer::set_fade_xip_addr set fade xip from %s to %s", xcons_utl::xip_to_hex(m_faded_xip2).c_str(), xcons_utl::xip_to_hex(new_addr).c_str());
    m_faded_xip2 = new_addr;
    return true;
}

bool xrelay_packer::set_start_time(const common::xlogic_time_t & start_time) {
    xunit_dbg("xrelay_packer::set_start_time %d node:%s this:%p", start_time, m_para->get_resources()->get_account().c_str(), this);
    m_start_time = start_time;
    return true;
}

bool xrelay_packer::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_validator()) ||
                     xcons_utl::xip_equals(xip, _evt_obj->get_target_proposal()->get_cert()->get_auditor()) ||
                     xcons_utl::xip_equals(m_last_xip2, _evt_obj->get_target_proposal()->get_cert()->get_validator()) ||
                     xcons_utl::xip_equals(m_last_xip2, _evt_obj->get_target_proposal()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() != xconsensus::enum_xconsensus_code_successful) {
        // accumulated table failed value
        auto fork_tag = "cons_table_failed_accu_" + get_account();
        XMETRICS_COUNTER_INCREMENT(fork_tag, 1);

        XMETRICS_GAUGE(metrics::cons_tableblock_total_succ, 0);
        if (is_leader) {
            XMETRICS_GAUGE(metrics::cons_tableblock_leader_succ, 0);
            auto error_tag = "cons_table_failed_error_code_" + std::to_string(_evt_obj->get_error_code());
            XMETRICS_COUNTER_INCREMENT(error_tag, 1);
        } else {
            XMETRICS_GAUGE(metrics::cons_tableblock_backup_succ, 0);
        }
        xunit_warn("xrelay_packer::on_proposal_finish fail. leader:%d,error_code:%d,proposal=%s,at_node:%s,m_last_xip2:%s",
                   is_leader,
                   _evt_obj->get_error_code(),
                   _evt_obj->get_target_proposal()->dump().c_str(),
                   xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
                   xcons_utl::xip_to_hex(m_last_xip2).c_str());
    } else {
        // reset to 0
        auto fork_tag = "cons_table_failed_accu_" + get_account();
        XMETRICS_COUNTER_SET(fork_tag, 0);

        xunit_info("xrelay_packer::on_proposal_finish succ. leader:%d,proposal=%s,at_node:%s,m_last_xip2:%s",
                   is_leader,
                   _evt_obj->get_target_proposal()->dump().c_str(),
                   xcons_utl::xip_to_hex(get_xip2_addr()).c_str(),
                   xcons_utl::xip_to_hex(m_last_xip2).c_str());

        base::xvblock_t * vblock = _evt_obj->get_target_proposal();
        xdbgassert(vblock->is_input_ready(true));
        xdbgassert(vblock->is_output_ready(true));
        vblock->add_ref();
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_consensus_data_t>(vblock, is_leader);
        m_para->get_resources()->get_bus()->push_event(ev);

        XMETRICS_GAUGE(metrics::cons_tableblock_total_succ, 1);
        if (is_leader) {
            XMETRICS_GAUGE(metrics::cons_tableblock_leader_succ, 1);
        } else {
            XMETRICS_GAUGE(metrics::cons_tableblock_backup_succ, 1);
        }
    }
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

bool xrelay_packer::on_replicate_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms)  // call from lower layer
                                                                                                                                                          // to higher layer(parent)
{
    xcsaccount_t::on_replicate_finish(event, from_child, cur_thread_id, timenow_ms);

    xconsensus::xreplicate_finish * _evt_obj = (xconsensus::xreplicate_finish *)&event;
    auto xip = get_xip2_addr();
    bool is_leader = xcons_utl::xip_equals(xip, _evt_obj->get_target_block()->get_cert()->get_validator()) ||
                     xcons_utl::xip_equals(xip, _evt_obj->get_target_block()->get_cert()->get_auditor());
    if (_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful) {
        base::xvblock_t * vblock = _evt_obj->get_target_block();
        xassert(vblock->is_input_ready(true));
        xassert(vblock->is_output_ready(true));
        vblock->add_ref();
        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_consensus_data_t>(vblock, is_leader);
        m_para->get_resources()->get_bus()->push_event(ev);
    }
    return true;  // stop handle anymore
}
bool xrelay_packer::on_consensus_commit(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    xcsaccount_t::on_consensus_commit(event, from_child, cur_thread_id, timenow_ms);
    xconsensus::xconsensus_commit * _evt_obj = (xconsensus::xconsensus_commit *)&event;
    xunit_dbg("xrelay_packer::on_consensus_commit, %s class=%d, at_node:%s",
              _evt_obj->get_target_commit()->dump().c_str(),
              _evt_obj->get_target_commit()->get_block_class(),
              xcons_utl::xip_to_hex(get_xip2_addr()).c_str());
    return false;  // throw event up again to let txs-pool or other object start new consensus
}

NS_END2
