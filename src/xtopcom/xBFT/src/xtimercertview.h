// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xbasic/xversion.h"
#include "xBFT/xconsengine.h"

NS_BEG2(top, xconsensus)

class xvip_votes_t {
public:
    int64_t create_tm;
    std::map<xvip2_t,std::string,xvip2_compare> validators;
};

class xvote_cache_t {
public:
    xvote_cache_t() = default;
    ~xvote_cache_t() = default;
    bool add_qcert(const xvip2_t &replica_xip, uint64_t clock, const std::string &qcert_bin);
    const std::map<xvip2_t,std::string,xvip2_compare>& get_clock_votes(uint64_t clock);
    void clear_timeout_clock();
    void clear();

private:
    bool check(const xvip2_t& replica_xip, uint64_t clock) const;

private:
    std::map<uint64_t, xvip_votes_t> m_clock_votes;
    std::map<xvip2_t, std::set<uint64_t>, xvip2_compare> m_clocks;
};

class xtimeout_msg_t : public base::xdataunit_t {
 public:
    xtimeout_msg_t()
    : base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    }
    explicit xtimeout_msg_t(base::xvblock_t * _block)
    : base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine), block(_block) {
        block->add_ref();
    }

 protected:
    virtual ~xtimeout_msg_t() {
        if (block != nullptr) {
            block->release_ref();
        }
    }

 public:
    virtual int32_t serialize_to_string(std::string & str) override;
    virtual int32_t serialize_from_string(const std::string & str) override;

 protected:
    // return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
    int32_t do_write(base::xstream_t & stream) override {
        const int32_t begin_size = stream.size();
        DEFAULT_SERIALIZE_PTR(block);
        return (stream.size() - begin_size);
    }

    int32_t do_read(base::xstream_t & stream) override {
        const int32_t begin_size = stream.size();
        DESERIALIZE_PTR(block) { block = (base::xvblock_t *)base::xdataunit_t::read_from(stream); }
        return (begin_size - stream.size());
    }

 public:
    base::xvblock_t * block{nullptr};
};

class xconspacemaker_t
  : public xcspacemaker_t
  , public base::xxtimer_t {
public:
    xconspacemaker_t(xcscoreobj_t & parent_object);

protected:
    virtual ~xconspacemaker_t();

public:
    virtual enum_xconsensus_pacemaker_type get_pacemaker_type() const override { return enum_xconsensus_pacemaker_type_timeout_cert; }
    // packet received from parent object to child
    virtual bool on_pdu_event_down(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

protected:
    // on_receive_timeout -> on_create_block_event -> on_receive_timeout_stage2
    bool on_receive_timeout(xvip2_t const & from_addr, base::xcspdu_t const & packet);
    void on_receive_timeout_stage2(xvip2_t const & from_addr, base::xvblock_t *model_block, base::xvblock_t *vote);

    // on_timeout -> on_create_block_event -> on_timeout_stage2
    void on_timeout(time_t cur_time);
    void on_timeout_stage2(base::xvblock_t *vote);

    bool merge_multi_sign(const xvip2_t & xip_addr, base::xvblock_t *block, const std::map<xvip2_t,std::string,xvip2_compare> &validators);

    void add_vote(const xvip2_t & xip_addr, base::xvblock_t *model_block, base::xvblock_t *vote);
    bool filter_consensus_event(base::xvevent_t const & event);

    virtual void        start_timer();

    virtual bool        close(bool force_async = true) override; //must call close before release object,otherwise object never be cleanup
    virtual bool        on_object_close() override;
    bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) override;
    void send_msg_for_block(const uint8_t msg_type, const std::string & msg_content, const uint16_t msg_nonce, const xvip2_t & from_addr, const xvip2_t & to_addr, uint64_t clock, uint64_t chainid);
    virtual uint64_t get_gmt_clock_second() const;

protected:
    bool reset_xip_addr(const xvip2_t & new_addr) override;

    bool on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;

    bool on_proposal_start(const base::xvevent_t & event, xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

    // call from lower layer to higher layer(parent)
    virtual bool on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

    // call from lower layer to higher layer(parent)
    virtual bool on_certificate_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

    // note: to return false may call parent'push_event_up,or stop further routing when return true
    virtual bool on_consensus_commit(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

    // note: to return false may call parent'push_event_up,or stop further routing when return true
    virtual bool on_consensus_update(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

protected:
    uint16_t            m_nonce{};
    xvote_cache_t       m_vote_cache;
    base::xvblock_t     *m_latest_cert{};   // latest block
    uint64_t            m_last_send_time{};
};

NS_END2
