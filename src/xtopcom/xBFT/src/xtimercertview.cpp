// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtimercertview.h"

#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblock_fork.h"

#include <inttypes.h>

#include <cassert>
#include <cinttypes>

NS_BEG2(top, xconsensus)

const uint32_t CLOCK_TIME_INTERVAL = 20000;  // 20s
const uint32_t ELASTIC_RANGE = 2;  // 2*clock
const uint64_t TOP_CLOCK_BEGIN_TIME = base::TOP_BEGIN_GMTIME;
const uint32_t PEER_TIMEOUT_TYPE = 0;
const uint32_t LOCAL_TIMEOUT_TYPE = 1;

bool xvote_cache_t::check(const xvip2_t& replica_xip, uint64_t clock) const {
    auto it = m_clocks.find(replica_xip);
    if (it == m_clocks.end()) {
        return true;
    }

    const std::set<uint64_t> &clock_set = it->second;
    if (clock_set.empty()) {
        return true;
    }

    // get upper
    auto itr = clock_set.rbegin();
    uint64_t self_last_clock = *itr;

    // node clock should monotonic increase
    // when all clocks of one node has been removed, the roll-back clock can be accepted.
    if (clock>self_last_clock && clock<=(self_last_clock+ELASTIC_RANGE)) {
        return true;
    }

    return false;
}

bool xvote_cache_t::add_qcert(const xvip2_t &replica_xip, uint64_t clock, const std::string &qcert_bin) {
    if (!check(replica_xip, clock))
        return false;

    // 1. add <xip,clock> to m_clocks
    {
        auto it = m_clocks.find(replica_xip);
        if (it == m_clocks.end()) {
            std::set<uint64_t> set;
            set.insert(clock);

            m_clocks[replica_xip] = set;
        } else {
            std::set<uint64_t> &set = it->second;
            set.insert(clock);
        }
    }

    // 2. add <xip,cert_str> to m_clock_votes
    auto it = m_clock_votes.find(clock);
    if (it == m_clock_votes.end()) {
        xvip_votes_t votes;
        votes.create_tm = base::xtime_utl::gmttime_ms();
        votes.validators[replica_xip] = qcert_bin;
        m_clock_votes[clock] = votes;
    } else {
        std::map<xvip2_t,std::string,xvip2_compare> &validators = it->second.validators;
        validators[replica_xip] = qcert_bin;
    }

    return true;
}

const std::map<xvip2_t,std::string,xvip2_compare>& xvote_cache_t::get_clock_votes(uint64_t clock) {
    auto it = m_clock_votes.find(clock);
    xassert(it != m_clock_votes.end());
    return it->second.validators;
}

void xvote_cache_t::clear_timeout_clock() {
    int64_t now = base::xtime_utl::gmttime_ms();

    std::map<uint64_t, xvip_votes_t>::iterator it = m_clock_votes.begin();
    for (; it!=m_clock_votes.end();) {
        uint64_t clock = it->first;
        xvip_votes_t &votes = it->second;

        if ((now - votes.create_tm) < CLOCK_TIME_INTERVAL) {
            ++it;
            continue;
        }

        std::map<xvip2_t,std::string,xvip2_compare> &validators = votes.validators;

        for (auto &it2: validators) {
            const xvip2_t &xip = it2.first;
            auto it3 = m_clocks.find(xip);
            assert(it3!=m_clocks.end());
            assert(!it3->second.empty());

            it3->second.erase(clock);

            xinfo("[xvote_cache_t::clear_timeout_clock] erase this %p, xip {%" PRIx64 ",%" PRIx64 "} clock %" PRIu64,
                this, xip.high_addr, xip.low_addr, clock);

            if (it3->second.empty()) {
                m_clocks.erase(xip);
            }
        }

        m_clock_votes.erase(it++);
    }
}

void xvote_cache_t::clear() {
    m_clock_votes.clear();
    m_clocks.clear();
}

int32_t xtimeout_msg_t::serialize_to_string(std::string & str) {
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    base::xdataunit_t::serialize_to(_stream);

    str.clear();
    str.assign((const char *)_stream.data(), _stream.size());
    return (int)str.size();
}

int32_t xtimeout_msg_t::serialize_from_string(const std::string & str) {
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)str.data(), (int32_t)str.size());
    base::xdataunit_t::serialize_from(_stream);
    return (int)str.size();
}

xconspacemaker_t::xconspacemaker_t(xcscoreobj_t & parent_object)
  : xcspacemaker_t(parent_object), base::xxtimer_t(*parent_object.get_context(), parent_object.get_thread_id()) {
    xinfo("xconspacemaker_t::xconspacemaker_t,create,this=%p,parent=%p,account=%s", this, &parent_object, parent_object.get_account().c_str());
}

xconspacemaker_t::~xconspacemaker_t() {
    xinfo("xconspacemaker_t::~xconspacemaker_t,destroy,this=%p", this);
    if (m_latest_cert != nullptr) {
        m_latest_cert->release_ref();
    }
}

// packet received from parent object to child
bool xconspacemaker_t::on_pdu_event_down(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    auto const & from_addr = event.get_from_xip();
    xcspdu_fire * _evt_obj  = (xcspdu_fire*)&event;
    auto const & packet = _evt_obj->_packet;

    if (m_latest_cert != nullptr) {
        _evt_obj->set_clock(m_latest_cert->get_clock());   // carry clock height
        _evt_obj->set_cookie(m_latest_cert->get_clock());  // carry latest viewid
    }
    
    const std::string & latest_xclock_cert_bin = _evt_obj->_packet.get_xclock_cert();
    if(latest_xclock_cert_bin.empty() == false) //try to update clock cert
    {
        base::xauto_ptr<base::xvqcert_t> _clock_cert_obj(base::xvblock_t::create_qcert_object(latest_xclock_cert_bin));
        if(_clock_cert_obj)
        {
            //note:#1 safe rule, always cleans up flags carried by peer
            _clock_cert_obj->reset_block_flags(); //force remove
            if(_clock_cert_obj->is_deliver())
                _evt_obj->set_xclock_cert(_clock_cert_obj.get());
        }
    }

    switch (packet.get_msg_type()) {
    // timeout / sync / resp belong to pacemaker
    case enum_consensus_msg_type_timeout:
        return on_receive_timeout(from_addr, packet);
    // intercept other consensus down events
    case enum_consensus_msg_type_proposal:
    case enum_consensus_msg_type_vote:
    case enum_consensus_msg_type_commit:
        return filter_consensus_event(event);
    }

    return xcspacemaker_t::on_pdu_event_down(event, from_parent, cur_thread_id, timenow_ms);
}

bool xconspacemaker_t::on_receive_timeout(xvip2_t const & from_addr, base::xcspdu_t const & packet) {
    xinfo("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "}, self {%" PRIx64 ",%" PRIx64 "} packet clock %" PRIu64,
        from_addr.high_addr, from_addr.low_addr, get_xip2_high_addr(), get_xip2_low_addr(), packet.get_block_clock());

    uint64_t clock = packet.get_block_clock();

    if (m_latest_cert!=nullptr && clock<=m_latest_cert->get_clock()) {
        xwarn("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "} clock rollback local:%" PRIu64" peer:%" PRIu64,
            from_addr.high_addr, from_addr.low_addr, clock, m_latest_cert->get_clock());
        return true;
    }

    if (from_addr.low_addr == 0) {
        xwarn("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "} xip error. clock %" PRIu64,
            from_addr.high_addr, from_addr.low_addr, clock);
        return true;
    }

    base::xauto_ptr<xtimeout_msg_t> msg = new xtimeout_msg_t{};
    msg->serialize_from_string(packet.get_msg_body());
    if (msg->block != nullptr) {
        if (msg->block->get_cert()==nullptr ||
            msg->block->get_clock()!=clock || msg->block->get_viewid()!=clock || msg->block->get_height()!=clock) {
            xwarn("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "} content error clock %" PRIu64" block:%s",
                from_addr.high_addr, from_addr.low_addr, clock, msg->block->dump().c_str());
            return true;
        }
    } else {
        return true;
    }

    uint32_t expect_version = base::xvblock_fork_t::instance().is_forked(msg->block->get_clock()) ? base::xvblock_fork_t::get_block_fork_new_version() : base::xvblock_fork_t::get_block_fork_old_version();
    if (msg->block->get_block_version() != expect_version) {
        xwarn("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "} version unmatch clock %" PRIu64" block:%s,expect_version=0x%x,actual_version=0x%x",
            from_addr.high_addr, from_addr.low_addr, clock, msg->block->dump().c_str(), expect_version, msg->block->get_block_version());
        return true;
    }

    XMETRICS_GAUGE(metrics::cpu_ca_verify_sign_tc, 1);
    if (get_vcertauth()->verify_sign(from_addr, (base::xvblock_t *)msg->block) != base::enum_vcert_auth_result::enum_successful) {
        xwarn("[xconspacemaker_t::on_receive_timeout] from {%" PRIx64 ",%" PRIx64 "} verify failed clock %" PRIu64" block:%s",
            from_addr.high_addr, from_addr.low_addr, clock, msg->block->dump().c_str());
        return true;
    }

    base::xauto_ptr<xcscreate_block_evt>_event_obj(new xcscreate_block_evt(from_addr, msg->block, clock, PEER_TIMEOUT_TYPE));
    get_parent_node()->push_event_up(*_event_obj, this, 0, 0);

    return true;
}

void xconspacemaker_t::on_receive_timeout_stage2(xvip2_t const & from_addr, base::xvblock_t *model_block, base::xvblock_t *vote) {
    add_vote(from_addr, model_block, vote);
}

// model_block is a block without signed-data.
// the content of the model_block is the target of the sign function and the multi-sign function.
// the content contains height, viewid, clock, xip_group, etc.
// all vote qcert is the sign data of the same content of model_block
// one clock slot one model_block
// block = (clocks's model_block) + (multisign)
void xconspacemaker_t::add_vote(const xvip2_t & xip_addr, base::xvblock_t *model_block, base::xvblock_t *vote) {
    uint64_t clock = model_block->get_clock();

    if (!model_block->get_cert()->is_validator(xip_addr)) {
        xwarn("[xconspacemaker_t::add_vote] invalid validator xip {%" PRIx64 ", %" PRIx64 "}", xip_addr.high_addr, xip_addr.low_addr);
        return;
    }

    if (!m_vote_cache.add_qcert(xip_addr, clock, vote->get_cert()->get_verify_signature())) {
        xwarn("[xconspacemaker_t::add_vote] add qcert failed. xip {%" PRIx64 ", %" PRIx64 "} clock %" PRIu64, xip_addr.high_addr, xip_addr.low_addr, clock);
        return;
    }

    const std::map<xvip2_t,std::string,xvip2_compare> &validators = m_vote_cache.get_clock_votes(clock);

    xinfo("[xconspacemaker_t::add_vote] xip {%" PRIx64 ", %" PRIx64 "}, clock %" PRIu64", version=0x%x,validators %d, threshold %d",
            xip_addr.high_addr, xip_addr.low_addr, clock, model_block->get_block_version(), validators.size(), model_block->get_cert()->get_validator_threshold());

    if (validators.size() < model_block->get_cert()->get_validator_threshold())
        return;

    if (!merge_multi_sign(xip_addr, model_block, validators)) {
        xwarn("[xvote_cache_t::add_vote] verify multi_sign failed %s", model_block->dump().c_str());
        return;
    }

    if (m_latest_cert != nullptr) {
        // for debug purpose
        if ((m_latest_cert->get_clock() + 1) != model_block->get_clock()) {
            XMETRICS_GAUGE(metrics::cons_pacemaker_tc_discontinuity, 1);
            xwarn("[xconspacemaker_t::add_vote] tc discontinuity latest=%llu,cur=%llu", m_latest_cert->get_clock(), model_block->get_clock());
        }
        m_latest_cert->release_ref();
    }

    m_latest_cert = model_block;
    m_latest_cert->add_ref();

    XMETRICS_GAUGE_SET_VALUE(metrics::clock_aggregate_height, m_latest_cert->get_height());

    xinfo("[xconspacemaker_t::add_vote] tc_aggregate_success %p view %" PRIu64" clock %" PRIu64, m_latest_cert, m_latest_cert->get_viewid(), m_latest_cert->get_clock());

    m_vote_cache.clear();

    base::xauto_ptr<xcstc_fire> _event_obj(new xcstc_fire(m_latest_cert));
    get_parent_node()->push_event_up(*_event_obj, this, 0, 0);

    std::function<void(void*)> _aysn_update_view = [this](void*)->void{
        fire_view(get_account(), this->m_latest_cert->get_viewid(), this->m_latest_cert->get_viewid(), 0, 0);
    };
    xcspacemaker_t::send_call(_aysn_update_view,(void*)NULL);
}

bool xconspacemaker_t::merge_multi_sign(const xvip2_t &xip_addr, base::xvblock_t *block, const std::map<xvip2_t,std::string,xvip2_compare> &validators) {
    XMETRICS_GAUGE(metrics::cpu_ca_merge_sign_tc, 1);
    // cert, is the target of the sign function, also is the target of the multi sign function.
    std::string sign = get_vcertauth()->merge_muti_sign(validators, block->get_cert());
    block->set_verify_signature(sign);
    block->reset_block_flags();
    block->set_block_flag(base::enum_xvblock_flag_authenticated);
    XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_tc, 1);
    return (get_vcertauth()->verify_muti_sign(block) == base::enum_vcert_auth_result::enum_successful);
}

uint64_t xconspacemaker_t::get_gmt_clock_second() const {
    time_t gmt = base::xtime_utl::gmttime();
    long long_gmt = gmt;
    uint64_t clock = long_gmt - TOP_CLOCK_BEGIN_TIME;
    return clock;
}

void xconspacemaker_t::on_timeout(time_t cur_time) {
    xvip2_t xip_addr = get_xip2_addr();

    if (xip_addr.low_addr == 0)
        return;

    m_vote_cache.clear_timeout_clock();

    uint64_t clock = get_gmt_clock_second();

    // consider clock rollback
    if (m_last_send_time > clock) {
        xwarn("[xconspacemaker_t::on_timeout] clock rollback {%" PRIx64 ",%" PRIx64 "} last %" PRIu64" clock %" PRIu64,
            xip_addr.high_addr, xip_addr.low_addr, m_last_send_time, clock);
        m_last_send_time = clock;
        return;
    }

    // if last_send_time and clock are in same 10s range, ignore
    if (m_last_send_time/10 == clock/10) {
        return;
    }

    m_last_send_time = clock;
    clock /= 10;

    if (m_latest_cert!=nullptr && clock<=m_latest_cert->get_clock()) {
        // may have been aggregate block from others's vote without myself's
        //xdbg("[xconspacemaker_t::on_timeout] clock shift clock %" PRIu64" last_clock %" PRIu64" now %" PRIu64, clock, m_latest_cert->get_clock(), m_last_send_time);
        return;
    }

    xinfo("[xconspacemaker_t::on_timeout] self {%" PRIx64 ",%" PRIx64 "} clock %" PRIu64" now(sec) %" PRIu64,
        xip_addr.high_addr, xip_addr.low_addr, clock, m_last_send_time);

    base::xauto_ptr<xcscreate_block_evt>_event_obj(new xcscreate_block_evt(xip_addr, nullptr, clock, LOCAL_TIMEOUT_TYPE));
    get_parent_node()->push_event_up(*_event_obj, this, 0, 0);
}

void xconspacemaker_t::on_timeout_stage2(base::xvblock_t *vote) {
    base::xvblock_t *model_block = vote;

    XMETRICS_GAUGE(metrics::cpu_ca_do_sign_tc, 1);
    xvip2_t xip_addr = get_xip2_addr();
    auto sign = get_vcertauth()->do_sign(xip_addr, vote, base::xtime_utl::get_fast_random64());
    vote->set_verify_signature(sign);
    XMETRICS_GAUGE(metrics::cpu_ca_verify_sign_tc, 1);
    xassert(get_vcertauth()->verify_sign(xip_addr, vote) == base::enum_vcert_auth_result::enum_successful);

    base::xauto_ptr<xtimeout_msg_t> msg = new xtimeout_msg_t{vote};

    std::string msg_stream;
    msg->serialize_to_string(msg_stream);

    // broadcast ...
    xvip2_t broadcast_addr = get_xip2_addr();
    reset_node_id_to_xip2(broadcast_addr);
    set_node_id_to_xip2(broadcast_addr, 0xFFF);

    send_msg_for_block(enum_consensus_msg_type_timeout, msg_stream, ++m_nonce, get_xip2_addr(), broadcast_addr, vote->get_clock(), vote->get_chainid());

    add_vote(xip_addr, model_block, vote);
}

bool xconspacemaker_t::filter_consensus_event(base::xvevent_t const & event) {
#if 0
    auto const & e = ((xcspdu_fire const &)event);
    auto const & packet = e._packet;
    xdbg("[xconspacemaker_t::filter_consensus_event] msg type %d, packet view %" PRIu64 ", cur view %" PRIu64, packet.get_msg_type(), packet.get_block_viewid(), m_cur_view);
    if (m_cur_view < packet.get_block_viewid()) {
        std::string cert_str = packet.get_vblock_cert();
        if (cert_str.empty()) {
            return true;  // since can't verify, do not go on
        }
        base::xauto_ptr<base::xvqcert_t> cert(base::xvblockstore_t::create_qcert_object(cert_str));
        if(cert == nullptr) {
            return true; // can't create cert
        }
        if(cert->get_viewid() > m_cur_view) {
            if (get_vcertauth()->verify_muti_sign(cert.get(),get_account()) != base::enum_vcert_auth_result::enum_successful) {
                return true; // don't go on
            }
            xinfo("[xconspacemaker_t::filter_consensus_event] call update_cur_view, msg type %d, packet view %" PRIu64 ", cert view %" PRIu64 ", cur view %" PRIu64, packet.get_msg_type(), packet.get_block_viewid(), cert->get_viewid(), m_cur_view);
            update_cur_view(cert->get_viewid()); // at least viewid
            //create_tc(cert.get());
            return false;
        }
        return false;
    } else if (m_cur_view > packet.get_block_viewid()) {
        return true;  // do not go on
    } else {
        return false;  // go on
    }
#endif
    return false;
}

void xconspacemaker_t::start_timer() {
    base::xxtimer_t::stop();
    base::xxtimer_t::start(1000, 1000);//1s, repeat
}

bool xconspacemaker_t::on_timer_fire(const int32_t thread_id,
                                     const int64_t timer_id,
                                     const int64_t current_time_ms,
                                     const int32_t start_timeout_ms,
                                     int32_t &     in_out_cur_interval_ms) {
    //xdbg("xconspacemaker_t::on_timer_fire this=%p,current_time=%ld", this, current_time_ms);
    on_timeout((time_t)(current_time_ms / 1000));  // convert to seconds
    return true;
}

bool xconspacemaker_t::close(bool force_async) {
    base::xxtimer_t::stop();
    xcspacemaker_t::close(force_async);
    base::xxtimer_t::close(force_async);
    xinfo("xconspacemaker_t::close, this=%p,refcount=%d", this, get_refcount());
    return true;
}

bool xconspacemaker_t::on_object_close() {
    base::xxtimer_t::on_object_close();  // stop timmer and set close flag
    xcspacemaker_t::on_object_close();
    xinfo("xconspacemaker_t::on_object_close this=%p,refcount=%d", this, get_refcount());
    return true;
}

bool xconspacemaker_t::reset_xip_addr(const xvip2_t & new_addr) {
    xinfo("[xconspacemaker_t::reset_xip_addr] new xip {%" PRIu64 ", %" PRIu64 "}", new_addr.high_addr, new_addr.low_addr);
    bool r = xcscoreobj_t::reset_xip_addr(new_addr);

    m_vote_cache.clear();

    if (new_addr.high_addr != 0 && new_addr.low_addr != 0) {
        start_timer();
        xinfo("[xconspacemaker_t::reset_xip_addr]");
    }
    // TODO how to stop?
    return r;
}

void xconspacemaker_t::send_msg_for_block(const uint8_t msg_type, const std::string & msg_content, const uint16_t msg_nonce, const xvip2_t & from_addr, const xvip2_t & to_addr, uint64_t clock, uint64_t chainid) {
    base::xauto_ptr<xcspdu_fire> _event_obj(new xcspdu_fire());

    _event_obj->set_from_xip(from_addr);
    _event_obj->set_to_xip(to_addr);
    _event_obj->_packet.set_block_chainid((uint32_t)chainid);
    _event_obj->_packet.set_block_account(get_account());
    _event_obj->_packet.set_block_height(0);
    _event_obj->_packet.set_block_clock(clock);
    _event_obj->_packet.set_block_viewid(0);
    _event_obj->_packet.set_block_viewtoken(0);

    _event_obj->_packet.reset_message(msg_type, get_default_msg_ttl(), msg_content, msg_nonce, from_addr.low_addr, to_addr.low_addr);
    _event_obj->set_route_path(base::enum_xevent_route_path_up);
    get_parent_node()->push_event_up(*_event_obj, this, xcspacemaker_t::get_thread_id(), xcspacemaker_t::get_time_now());
}

bool xconspacemaker_t::on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) {
    xconsensus::xcscreate_block_evt const & e = (xconsensus::xcscreate_block_evt const &)event;
    base::xvblock_t* block = e.get_block();

    // get unique target xip
    xvip2_t target_xip = get_xip2_addr();
    reset_node_id_to_xip2(target_xip);
    set_node_id_to_xip2(target_xip, 0xFFF);
    block->get_cert()->set_validator(target_xip);

    uint32_t context_id = e.get_context_id();
    if (context_id == PEER_TIMEOUT_TYPE) {

        base::xvblock_t* model_block = block;
        base::xvblock_t* vote = e.get_vote();
        on_receive_timeout_stage2(e.get_xip(), model_block, vote);

    } else if (context_id == LOCAL_TIMEOUT_TYPE) {

        base::xvblock_t* vote = block;
        on_timeout_stage2(vote);
    }

    return true;
}

bool xconspacemaker_t::on_proposal_start(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    
    xproposal_start * _evt_obj = (xproposal_start *)&event;
    if( (_evt_obj->get_clock_cert() == NULL) && (m_latest_cert != NULL) )
    {
        _evt_obj->set_clock_cert(m_latest_cert->get_cert());
    }
#if 0
    if (_evt_obj->get_proposal() != NULL)  // proposal from leader
    {
        assert(_evt_obj->get_proposal()->get_height() > 0);
        _evt_obj->get_proposal()->get_cert()->set_viewid(m_cur_view);
        if(_evt_obj->get_proposal()->get_cert()->get_clock() == 0) //application not set,let make it up
            _evt_obj->get_proposal()->get_cert()->set_clock(m_cur_view);

        //give random nonce for clock block
        _evt_obj->get_proposal()->get_cert()->set_nonce(base::xtime_utl::get_fast_random64());

        if(_evt_obj->get_proposal()->get_height() > m_cur_view) {
            xinfo("[xconspacemaker_t::on_proposal_start] proposal height %" PRIu64 ", view %" PRIu64 ", clock %" PRIu64,
                _evt_obj->get_proposal()->get_height(),
                _evt_obj->get_proposal()->get_cert()->get_viewid(),
                _evt_obj->get_proposal()->get_cert()->get_clock());
            update_cur_view(_evt_obj->get_proposal()->get_height() + 1); // at least height + 1 for next round
            return true; // cancel proposal since view is not correct
        }
    }
#endif
    return false;  // let lower layer continue get notified
}

// call from lower layer to higher layer(parent)
bool xconspacemaker_t::on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
#if 0
    xproposal_finish * _evt_obj = (xproposal_finish *)&event;
    if (_evt_obj->get_error_code() == enum_xconsensus_code_successful) {
        update_cur_view(_evt_obj->get_latest_cert());  //latest one go first
        update_cur_view(_evt_obj->get_target_proposal());
    }
    return xcspacemaker_t::on_proposal_finish(event, from_child, cur_thread_id, timenow_ms);  // let upper layer continue get notified
#endif
    return false;
}

// call from lower layer to higher layer(parent)
bool xconspacemaker_t::on_certificate_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    // xcertificate_finish* _evt_obj = (xcertificate_finish*)&event;
    return true;  // certificate_finish is internal event, so stop throw up
}

// note: to return false may call parent'push_event_up,or stop further routing when return true
bool xconspacemaker_t::on_consensus_commit(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    // xconsensus_commit* _evt_obj = (xconsensus_commit*)&event;
    return false;  // let upper layer continue get notified
}

// note: to return false may call child'push_event_down/up,or stop further routing when return true
bool xconspacemaker_t::on_consensus_update(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {
    // xconsensus_update* _evt_obj = (xconsensus_update*)&event;
    return false;  // let lower layer continue get notified
}

NS_END2
