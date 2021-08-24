// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtestaccount.hpp"
#include "xtestnet.hpp"
#include "xunitblock.hpp"
#include <map>
#include <queue>

namespace top {
namespace test {
enum enum_xtestnode_role {
    enum_xtestnode_role_mask = 0x0F,    //mask to check role
    enum_xtestnode_role_honest = 0x01,  //reaction according rules and data
    enum_xtestnode_role_evil = 0x02,    //malicious node
    enum_xtestnode_role_offline = 0x03, //not respond anything & not sendout anything
    enum_xtestnode_role_bug = 0x04,     //do random wrong thing,e.g
};

class xtestnode_t : public xconsensus::xcsobject_t {
    friend class xtestshard;

public:
    //node_types refer enum_xtestnode_type
    xtestnode_t(base::xcontext_t & _context,const int32_t target_thread_id, const uint32_t node_types, const std::string &node_param, const xvip2_t &node_address);

protected:
    virtual ~xtestnode_t();

private:
    xtestnode_t();
    xtestnode_t(const xtestnode_t &);
    xtestnode_t &operator=(const xtestnode_t &);

public:
    const uint32_t get_types() const { return m_node_types; }
    const uint32_t get_total_nodes() const { return m_total_nodes; }
    bool           check_type(enum_xtestnode_role type) const { return ((m_node_types & type) != 0); }

    virtual bool init(const xvip2_t & new_node_address,const std::string &account, const uint32_t total_nodes);
    virtual bool fire_clock(base::xvblock_t &latest_clock_block, int32_t cur_thread_id, uint64_t timenow_ms) override;

    virtual bool on_txs_recv(const std::string &account, const std::string &txs);

    bool                  start_consensus(base::xvblock_t *proposal_block);
            
    base::xvblock_t *     create_proposal_block(const std::string &account,const std::string &block_input, const std::string &block_output,const uint64_t new_viewid);
public:
    virtual int verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) override;

    //send packet from this object to parent layers
    virtual bool send_out(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms) override;

    //recv_in packet from this object to child layers
    virtual bool recv_in(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms) override;

    //on_view_change event
    virtual bool on_view_fire(const base::xvevent_t &event, xcsobject_t *from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) override;

protected: //guanrentee be called  at object'thread,triggered by push_event_up or push_event_down
    //note: to return false may call parent'push_event_up,or stop further routing when return true
    virtual bool    on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
    
    bool    fire_proposal();
    
private:
    uint32_t                                       m_node_types;
    uint32_t                                       m_total_nodes;
    uint64_t                                       m_latest_viewid;
    xtestaccount_t *                               m_test_account;
    std::map<std::string, std::deque<std::string>> m_txs_pool;
};
}; // namespace test
}; // namespace top
