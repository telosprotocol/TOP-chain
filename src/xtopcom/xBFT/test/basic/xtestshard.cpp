// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestshard.hpp"
#include "xcertauth/xcertauth_face.h"
#include "xbase/xutl.h"
#include "xcrypto/xckey.h"

namespace top
{
    namespace test
    {
        xtestshard::xtestshard(base::xworkerpool_t & shard_worker_pool,const std::multimap<uint32_t,std::string> & nodes_list)
        :xconsensus::xcsobject_t(shard_worker_pool.get_thread(0)->get_context(),shard_worker_pool.get_thread(0)->get_thread_id(),(base::enum_xobject_type)xconsensus::enum_xconsensus_object_type_base)
        {
            m_shard_base_addr = 1;
            m_total_nodes = (int)nodes_list.size();
            memset(m_nodes,0,sizeof(m_nodes));
            
            m_raw_timer = NULL;
            m_raw_timer = get_thread()->create_timer((xtimersink_t*)this);
            
            m_nodesvr_ptr = new base::xvnodehouse_t();
            register_plugin(m_nodesvr_ptr);
            register_plugin(&auth::xauthcontext_t::instance(*m_nodesvr_ptr));
            init_nodes_shard();
            
            base::xworkerpool_t_impl<2> *job_worker_pool = new base::xworkerpool_t_impl<2>(top::base::xcontext_t::instance());
            register_plugin(job_worker_pool);
            job_worker_pool->release_ref();
            
            uint32_t last_node_index = 0;
            const uint32_t total_shard_threads = shard_worker_pool.get_count();
            for(auto it = nodes_list.begin(); it != nodes_list.end(); ++it)
            {
                xvip2_t node_addr;
                node_addr.high_addr = m_shard_xipaddr.high_addr;
                node_addr.low_addr  = m_shard_xipaddr.low_addr | last_node_index;
                
                const uint32_t    node_type    = it->first;
                const std::string node_param   = it->second;
                
                m_nodes[last_node_index] = new xtestnode_t(shard_worker_pool.get_thread(last_node_index % total_shard_threads)->get_context(),shard_worker_pool.get_thread(last_node_index % total_shard_threads)->get_thread_id(),node_type,node_param,node_addr);
                attach_child_node(m_nodes[last_node_index],node_addr,std::string());
                ++last_node_index;
            }
            m_leader_slot = base::xtime_utl::get_fast_random(m_total_nodes);
            m_raw_timer->start(1000, 1000); //generate a clock block every 1 seconds
        }
    
        xtestshard::~xtestshard()
        {
            for(int i = 0; i < enum_max_nodes_count; ++i)
            {
                xtestnode_t* _node = m_nodes[i];
                if(_node != NULL)
                {
                    _node->close();
                    _node->release_ref();
                }
            }
            m_raw_timer->close(false);
            m_raw_timer->release_ref();
        }
        
        int    xtestshard::get_leader()
        {
            return m_leader_slot;
        }
        
        void   xtestshard::init_nodes_shard()
        {
            m_shard_xipaddr.high_addr = (((uint64_t)m_total_nodes) << 54) | m_shard_base_addr; //encode node'size of group
            m_shard_xipaddr.low_addr  = 1 << 10; //at group#1
            std::vector<base::xvnode_t*> _consensus_nodes;
            for(uint32_t i = 0; i < m_total_nodes; ++i)
            {
                xvip2_t node_addr;
                node_addr.high_addr = m_shard_xipaddr.high_addr;
                node_addr.low_addr  = m_shard_xipaddr.low_addr | i;
            
                //std::pair<std::string, std::string> pri_pub_keys = ((auth::xauthcontext_t*)get_vcertauth())->create_secp256k1_keypair();
                utl::xecprikey_t node_prv_key;
                const std::string node_account     = node_prv_key.to_account_address(base::enum_vaccount_addr_type_secp256k1_eth_user_account, 0);
                
                std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
                std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
                
                _consensus_nodes.push_back(new base::xvnode_t(node_account,node_addr,_node_pub_key_str,_node_prv_key_str));
            }
            base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new base::xvnodegroup_t(m_shard_xipaddr,0,_consensus_nodes));
            m_nodesvr_ptr->add_group(_consensus_group.get());
            for(auto it : _consensus_nodes)
                it->release_ref();
        }
    
        //packet sending to parent object from child ,and do broadcast if to_addr is empty,
        bool  xtestshard::on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus::xcspdu_fire * _evt_obj = (xconsensus::xcspdu_fire *)&event;
            send_out(_evt_obj->get_from_xip(),_evt_obj->get_to_xip(),_evt_obj->_packet,cur_thread_id,timenow_ms);
            return true;//stop here
        }
        
        //on_view_change event
        bool  xtestshard::on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            m_leader_slot = base::xtime_utl::get_fast_random(m_total_nodes);
            return true;
        }
        
        bool  xtestshard::send_out(const xvip2_t & from_addr,const xvip2_t & to_addr,base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            if( (to_addr.high_addr == (uint64_t)-1) && (to_addr.low_addr == (uint64_t)-1) ) //broad cast to all except self
            {
                for(uint32_t i = 0; i < m_total_nodes; ++i)
                {
                    xtestnode_t* _node = m_nodes[i];
                    if(_node != NULL)
                    {
                        xvip2_t this_address = _node->get_xip2_addr();
                        if( (this_address.low_addr != from_addr.low_addr) ||  (this_address.high_addr != from_addr.high_addr) )
                            _node->recv_in(from_addr, this_address, packet, cur_thread_id, timenow_ms);
                    }
                }
            }
            else
            {
                for(uint32_t i = 0; i < m_total_nodes; ++i)
                {
                    xtestnode_t* _node = m_nodes[i];
                    if(_node != NULL)
                    {
                        xvip2_t this_address = _node->get_xip2_addr();
                        if( (this_address.low_addr == to_addr.low_addr) && (this_address.high_addr == to_addr.high_addr) )
                        {
                            _node->recv_in(from_addr, this_address, packet, cur_thread_id, timenow_ms);
                            break;
                        }
                    }
                }
            }
            return true;
        }
        
        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xtestshard::on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus::xcspdu_fire * _evt_obj = (xconsensus::xcspdu_fire *)&event;
            recv_in(_evt_obj->get_from_xip(),_evt_obj->get_to_xip(),_evt_obj->_packet,cur_thread_id,timenow_ms);
            return true; //stop here
        }
        
        //packet received from parent object to child
        bool xtestshard::recv_in(const xvip2_t & from_addr,const xvip2_t & to_addr,base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            if( (to_addr.high_addr == (uint64_t)-1) && (to_addr.low_addr == (uint64_t)-1) ) //broad cast to all except self
            {
                for(uint32_t i = 0; i < m_total_nodes; ++i)
                {
                    xtestnode_t* _node = m_nodes[i];
                    if(_node != NULL)
                    {
                        xvip2_t this_address = _node->get_xip2_addr();
                        if( (this_address.low_addr != from_addr.low_addr) ||  (this_address.high_addr != from_addr.high_addr) )
                            _node->recv_in(from_addr, this_address, packet, cur_thread_id, timenow_ms);
                    }
                }
            }
            else
            {
                for(uint32_t i = 0; i < m_total_nodes; ++i)
                {
                    xtestnode_t* _node = m_nodes[i];
                    if(_node != NULL)
                    {
                        xvip2_t this_address = _node->get_xip2_addr();
                        if( (this_address.low_addr == from_addr.low_addr) && (this_address.high_addr == from_addr.high_addr) )
                        {
                            _node->recv_in(from_addr, this_address, packet, cur_thread_id, timenow_ms);
                            break;
                        }
                    }
                }
            }
            return true;
        }
        
        //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
        bool    xtestshard::on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode)
        { 
            return true;//always true for group like book
        }
        
        //notify has child-node left from this node,
        bool    xtestshard::on_child_node_leave(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode)
        {
            return true;//always true for group like book
        }
        
        bool    xtestshard::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //attached into io-thread
        {
            return true;
        }
        bool    xtestshard::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)
        {
            return true;
        }
        
        bool   xtestshard::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            base::xauto_ptr<base::xvblock_t>  new_clock_block(xtestclocker_t::on_clock_fire());
            for(uint32_t i = 0; i < m_total_nodes; ++i)
            {
                xtestnode_t* _node = m_nodes[i];
                if(_node != NULL)
                {
                    _node->fire_clock(*new_clock_block, 0, 0);
                }
            }
            if( (new_clock_block->get_clock() % 6 == 0) ) //every 60 seconds do one election round
            {
                //m_shard_base_addr = m_shard_base_addr + 1;
                //init_nodes_shard();
            }
            return true;
        }
           
        bool  xtestshard::on_txs_recv(const std::string & account,const std::string & txs)
        {
            for(uint32_t i = 0; i < m_total_nodes; ++i)
            {
                xtestnode_t* _node = m_nodes[i];
                if(_node != NULL)
                {
                    xvip2_t node_addr;
                    node_addr.high_addr = m_shard_xipaddr.high_addr;
                    node_addr.low_addr  = m_shard_xipaddr.low_addr | (i);
                    _node->init(node_addr,account,get_total_nodes());
                }
            }
            
            for(uint32_t i = 0; i < m_total_nodes; ++i)
            {
                xtestnode_t* _node = m_nodes[i];
                if(_node != NULL)
                {
                    _node->on_txs_recv(account, txs);
                }
            }
            return true;
        }
    }
};
