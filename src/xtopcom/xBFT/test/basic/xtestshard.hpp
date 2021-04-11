// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"
#include "xbase/xtimer.h"
#include "xtestnode.hpp"
#include "xtestclock.hpp"
#include "xtestca.hpp"


namespace top
{
    namespace test
    {
        class xtestshard : public xconsensus::xcsobject_t,public xtestclocker_t, public base::xtimersink_t
        {
            enum {enum_max_nodes_count = 256};
        public:
            //reserved for test purpose
            xtestshard(base::xworkerpool_t & _worker_pool,const std::multimap<uint32_t,std::string> & nodes_list);
        protected:
            virtual ~xtestshard();
        private:
            xtestshard();
            xtestshard(const xtestshard &);
            xtestshard & operator = (const xtestshard &);

        public:
            int                     get_leader();
            int                     get_total_nodes() const {return m_total_nodes;}
 
            bool  on_txs_recv(const std::string & account,const std::string & txs);
            
            virtual bool   send_out(const xvip2_t & from_addr,const xvip2_t & to_addr,base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms);
            
            virtual bool  recv_in(const xvip2_t & from_addr,const xvip2_t & to_addr,base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms);
            
        protected:
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool    on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool    on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //on_view_change event
            virtual bool    on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        private:
            //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
            virtual bool    on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode) override;
            
            //notify has child-node left from this node,
            virtual bool    on_child_node_leave(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode) override;
            
            virtual bool    on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool    on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool    on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
        private:
            void        init_nodes_shard();
        private:
            uint32_t            m_leader_slot;
            uint32_t            m_total_nodes;
            uint64_t            m_shard_base_addr;            //simulator election round
            xvip2_t             m_shard_xipaddr;
            xtestnode_t*        m_nodes[enum_max_nodes_count];//max as 256 nodes
            base::xvnodesrv_t*  m_nodesvr_ptr;
        private:
            base::xtimer_t*     m_raw_timer;
        };
    }
};
