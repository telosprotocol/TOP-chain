// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xBFT/xconsobj.h"
#include "xbase/xbase.h"
#include "xbase/xthread.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xmbus/xmessage_bus.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xcommon/xlogic_time.h"
#include "xunit_service/xunit_log.h"
#include "xunit_service/xcons_utl.h"
#include "xrouter/xrouter_face.h"

#include <string>
#include <vector>

NS_BEG2(top, xunit_service)

enum {
    enum_rotate_mode_no_rotate = 0,
    enum_rotate_mode_rotate_by_last_block = 1,
    enum_rotate_mode_rotate_by_view_id = 2,
};

class xpdu_reactor_face {
public:
    virtual void on_pdu(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet) = 0;
    virtual bool is_running() = 0;
};

using xpdu_reactor_ptr = std::shared_ptr<xpdu_reactor_face>;

// internal network proxy
class xnetwork_proxy_face {
public:
    virtual bool send_out(uint32_t msg_type, const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) = 0;
    virtual bool send_out(common::xmessage_id_t const & id, const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t * block) = 0;
    virtual bool listen(const xvip2_t & xip, common::xmessage_category_t category, const xpdu_reactor_ptr & reactor) = 0;
    virtual bool unlisten(const xvip2_t & xip, common::xmessage_category_t category) = 0;
    virtual bool add(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network) = 0;
    virtual std::shared_ptr<vnetwork::xvnetwork_driver_face_t> find(const xvip2_t &addr) = 0;
    virtual bool erase(const xvip2_t & addr) = 0;
    virtual void send_receipt_msgs(const xvip2_t & from_addr, const std::vector<data::xcons_transaction_ptr_t> & receipts, std::vector<data::xcons_transaction_ptr_t> & non_shard_cross_receipts) = 0;
};

//compare function for table index data map
struct table_index_compare
{
    bool operator()(const base::xtable_index_t& ti_lhs, const base::xtable_index_t& ti_rhs) const
    {
        return ti_lhs.to_table_shortid() < ti_rhs.to_table_shortid();
    };
};
// system election face
class xelection_cache_face {
public:
    struct xelect_data {
        xvip2_t xip;
        common::xelection_round_t joined_version;
        uint64_t staking;

        xelect_data& operator=(const xelect_data & data) {
            if (this == &data) {
                return *this;
            } else {
                xip = data.xip;
                staking = data.staking;
                return *this;
            }
        }
    };

    using elect_set = std::vector<xelect_data>;

public:
    // load manager tables
    virtual int32_t get_tables(const xvip2_t & xip, std::vector<base::xtable_index_t> * tables) = 0;
    // load election data from db
    virtual int32_t get_election(const xvip2_t & xip, elect_set * elect_data, bool bself = true) = 0;
    // load group election data
    virtual int32_t get_group_election(const xvip2_t & xip, int32_t groupid, elect_set * elect_data) = 0;
    // load parent election data
    virtual int32_t get_parent_election(const xvip2_t & xip, elect_set * elect_data) = 0;
    // add elect data
    virtual bool add(const xvip2_t & xip, const elect_set & elect_data, const std::vector<uint16_t> & tables, const elect_set & parent_elect_data = {}, std::map<int32_t, elect_set> children = {}) = 0;
    // erase elect data
    virtual bool erase(const xvip2_t & xip) = 0;
};

// consensus leader election face
class xleader_election_face {
public:
    // judge node is leader according viewid account and existed data
    virtual const xvip2_t get_leader_xip(uint64_t viewId, const std::string & account, base::xvblock_t* prev_block, const xvip2_t & local, const xvip2_t & candidate, const common::xelection_round_t& version, uint16_t rotate_mode = enum_rotate_mode_rotate_by_last_block) = 0;

    // get election face which manager elect datas
    virtual xelection_cache_face * get_election_cache_face() = 0;
};

// system resource
class xresources_face {
public:
    // certificate auth face
    virtual base::xvcertauth_t * get_certauth() = 0;
    // work pool
    virtual base::xworkerpool_t * get_workpool() = 0;
    // work pool
    virtual base::xworkerpool_t * get_xbft_workpool() = 0;
    // network face
    virtual xnetwork_proxy_face * get_network() = 0;
    // block store
    virtual base::xvblockstore_t * get_vblockstore() = 0;
    // election face
    virtual xleader_election_face * get_election() = 0;
    // chain timer face
    virtual time::xchain_time_face_t * get_chain_timer() = 0;
    // elect data accessor face
    virtual election::cache::xdata_accessor_face_t * get_data_accessor() = 0;
    // node account
    virtual const std::string & get_account() = 0;
    virtual mbus::xmessage_bus_face_t* get_bus() = 0;
    virtual xtxpool_v2::xtxpool_face_t * get_txpool() = 0;
};

enum e_cons_type {
    e_timer,
    e_table,
};

class xproposal_maker_face {
 public:
    virtual bool                        can_make_proposal(data::xblock_consensus_para_t & proposal_para) = 0;
    virtual xblock_ptr_t                make_proposal(data::xblock_consensus_para_t & proposal_para) = 0;
    virtual int                         verify_proposal(base::xvblock_t* proposal_block, base::xvqcert_t * bind_clock_cert) = 0;
};

// block maker face
class xblock_maker_face {
public:
    virtual base::xauto_ptr<base::xvblock_t> get_latest_block(const std::string & account) { return nullptr;}
    virtual base::xvblock_t *                make_block(const std::string & account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t & leader_xip) { return nullptr;}
    virtual int                              verify_block(base::xvblock_t * proposal_block) { return -1;}
    virtual std::shared_ptr<xproposal_maker_face>   get_proposal_maker(const std::string & account) {return nullptr;}
};

using xblock_maker_ptr = std::shared_ptr<xblock_maker_face>;

// consensuss parameter
class xconsensus_para_face {
public:
    // get pacemaker type
    virtual xconsensus::enum_xconsensus_pacemaker_type get_pacemaker_type() = 0;
    // get algorithm type
    // virtual xconsensus::enum_xconsensus_algorithm_type get_algorithm_type() = 0;
    virtual base::enum_xconsensus_threshold get_threshold() = 0;
    // add block maker
    virtual void add_block_maker(e_cons_type cons_type, const xblock_maker_ptr & block_maker) = 0;
    // get block maker
    virtual xblock_maker_ptr get_block_maker(e_cons_type cons_type) = 0;
};

// block service create parameter & depends resources
class xcons_service_para_face {
public:
    // get system resources
    virtual xresources_face * get_resources() = 0;
    // get consensus para
    virtual xconsensus_para_face * get_consensus_para() = 0;
};

using xcons_service_para_ptr = std::shared_ptr<xcons_service_para_face>;

// consensus engine face
class xcons_service_face {
public:
    virtual common::xmessage_category_t get_msg_category() = 0;
    virtual bool                        start(const xvip2_t & xip, const common::xlogic_time_t& start_time) = 0;
    virtual bool                        fade(const xvip2_t & xip) = 0;
    virtual bool                        unreg(const xvip2_t & xip) = 0;
    virtual bool                        destroy(const xvip2_t & xip) = 0;
};
#if 0
class xcons_proxy_face {
public:
    virtual bool    start(const common::xlogic_time_t& start_time) = 0;
    virtual bool    fade() = 0;
    virtual bool    outdated() = 0;
    virtual xvip2_t get_ip() = 0;
};

using xcons_proxy_face_ptr = std::shared_ptr<xcons_proxy_face>;
#endif
const int32_t max_mailbox_num = 8192;
// block dispatcher
class xcons_dispatcher {
public:
    explicit xcons_dispatcher(e_cons_type cons_type) : m_cons_type(cons_type) {}
    // virtual ~xcons_dispatcher() {}
    virtual e_cons_type get_cons_type() { return m_cons_type; }
    // dispatch events
    virtual bool dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to) = 0;

    virtual void on_clock(base::xvblock_t * clock_block) {}

    // start
    virtual bool start(const xvip2_t & xip, const common::xlogic_time_t& start_time) = 0;

    virtual bool fade(const xvip2_t & xip) = 0;

    virtual bool unreg(const xvip2_t & xip) = 0;

    virtual bool destroy(const xvip2_t & xip) = 0;

protected:

    template <typename T>
    int async_dispatch(base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to, T * picker) {
        // TODO(jimmy) for debug
        int64_t in, out;
        int32_t queue_size = picker->count_calls(in, out);
        bool discard = queue_size >= max_mailbox_num;
        if (discard) {
            XMETRICS_GAUGE(metrics::mailbox_us_total, 0);
            xunit_warn("xnetwork_proxy::async_dispatch,recv_in is_mailbox_over_limit pdu=%s,in=%lld,out=%lld,queue_size=%d,at_node:%s %p", pdu->dump().c_str(), in, out, queue_size, xcons_utl::xip_to_hex(xip_to).c_str(), picker);
            return -1;
        } else {
            XMETRICS_GAUGE(metrics::mailbox_us_total, 1);
            xunit_info("xnetwork_proxy::async_dispatch,recv_in pdu=%s,in=%lld,out=%lld,queue_size=%d,at_node:%s %p", pdu->dump().c_str(), in, out, queue_size, xcons_utl::xip_to_hex(xip_to).c_str(), picker);
        }

        auto handler = [xip_from, xip_to](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            auto packer = dynamic_cast<T *>(call.get_param1().get_object());
            auto raw_pdu = dynamic_cast<base::xcspdu_t *>(call.get_param2().get_object());
            return packer->recv_in(xip_from, xip_to, *raw_pdu, cur_thread_id, timenow_ms);
        };
        base::xcall_t asyn_call(handler, picker, pdu);
        return picker->send_call(asyn_call);
    }

protected:
    e_cons_type m_cons_type;
};

using xcons_dispatcher_ptr = std::shared_ptr<xcons_dispatcher>;

class xcons_dispatcher_builder_face {
public:
    virtual xcons_dispatcher_ptr build(observer_ptr<mbus::xmessage_bus_face_t> const &mb, xcons_service_para_ptr const &, e_cons_type cons_type) = 0;
};

using xcons_dispatcher_builder_ptr = std::shared_ptr<xcons_dispatcher_builder_face>;

// block service builder
class xcons_service_mgr_face {
public:
    // build block service and hold it external
    virtual void create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network) = 0;
    // destroy useless cons services
    virtual bool destroy(const xvip2_t & xip) = 0;

    // init reference data
    virtual bool start(const xvip2_t & xip, const common::xlogic_time_t& start_time) = 0;

    virtual bool fade(const xvip2_t & xip) = 0;

    // uninit reference data
    virtual bool unreg(const xvip2_t & xip) = 0;
};

using xcons_service_mgr_ptr = std::shared_ptr<xcons_service_mgr_face>;

NS_END2
