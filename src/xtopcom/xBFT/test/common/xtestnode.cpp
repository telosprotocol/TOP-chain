// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xBFT/xconsaccount.h"
#include "xbase/xutl.h"
#include "xtestnode.hpp"
#include "xtestnet.hpp"
#include "xtestclock.hpp"
#include "xcertauth/xcertauth_face.h"
#include "xvledger/xvledger.h"

#ifdef __MAC_PLATFORM__
    #include "xblockstore/xblockstore_face.h"
    #include "xtestdb.hpp"
#endif

namespace top
{
    namespace test
    {
        xtestnode_t::xtestnode_t(base::xcontext_t & _context,const int32_t target_thread_id,const uint32_t node_types,const std::string & node_param,const xvip2_t & node_address)
        :xcsobject_t(_context,target_thread_id,(base::enum_xobject_type)xconsensus::enum_xconsensus_object_type_node)
        {
            m_node_types = node_types;
            m_total_nodes = 0;
            m_latest_viewid = 0;
            m_test_account = NULL;
            reset_xip_addr(node_address);
            
            xveventbus_impl * mbus_store = new xveventbus_impl();
            base::xvchain_t::instance().set_xevmbus(mbus_store);
            
            char szBuff[32] = {0};
            const int inBufLen = sizeof(szBuff);
            snprintf(szBuff,inBufLen,"/0x%llx",node_address.low_addr);
            
            if(base::xvchain_t::instance().get_xdbstore() == NULL)
            {
                const std::string  default_path = szBuff;
                xstoredb_t* _persist_db = new xstoredb_t(default_path);
                base::xvchain_t::instance().set_xdbstore(_persist_db);
            }
            base::xvblockstore_t * blockstore_ptr = store::create_vblockstore(NULL);
            set_vblockstore(blockstore_ptr);
            register_plugin(blockstore_ptr);

            xdbg("xtestnode_t::create");
        }
        
        xtestnode_t::~xtestnode_t()
        {
            xdbg("xtestnode_t::destroy");
            if(m_test_account != NULL)
                m_test_account->release_ref();
        };
        
        bool    xtestnode_t::init(const xvip2_t & new_node_address,const std::string & account,const uint32_t total_nodes)
        {
            if(get_child_node() != NULL)
            {
                if(false == is_xip2_equal(new_node_address, get_xip2_addr()))
                {
                    std::function<void(void*)> _reset_addr_job = [this](void* _addr)->void{
                        xvip2_t * _new_addr = (xvip2_t*)_addr;
                        reset_xip_addr(*_new_addr);
                        m_test_account->reset_xip_addr(*_new_addr);
                        delete _new_addr;
                    };
                    xvip2_t *  clone_addrs = new xvip2_t;
                    *clone_addrs = new_node_address;
                    send_call(_reset_addr_job,(void*)clone_addrs);
                }
                return true;
            }
            if(m_total_nodes != total_nodes)
            {
                base::xatomic_t::xstore(m_total_nodes, total_nodes);
            }
            
            if(NULL == m_test_account)
            {
                xconsensus::xcsobject_t* last_object = this;
                if( (get_types() & enum_net_rtt_type_mask) != 0)
                {
                    xrtt_simulator * _rtt_plugin = new xrtt_simulator(*get_context(),get_thread_id(),(enum_net_rtt_type)((get_types() & enum_net_rtt_type_mask)));
                    
                    last_object->attach_child_node(_rtt_plugin, last_object->get_xip2_addr(),std::string());
                    _rtt_plugin->release_ref();
                    
                    last_object = _rtt_plugin;
                }
                
                if( (get_types() & enum_net_outoforder_type_mark) != 0)
                {
                    xoutoforder_simulator * _unorder_plugin = new xoutoforder_simulator(*get_context(),get_thread_id(),(enum_net_outoforder_type)((get_types() & enum_net_outoforder_type_mark)));
                    
                    last_object->attach_child_node(_unorder_plugin, last_object->get_xip2_addr(),std::string());
                    _unorder_plugin->release_ref();
                    
                    last_object = _unorder_plugin;
                }
                
                if( (get_types() & enum_net_lossrate_type_mask) != 0)
                {
                    xloss_simulator * _loss_plugin = new xloss_simulator(*get_context(),get_thread_id(),(enum_net_lossrate_type)((get_types() & enum_net_lossrate_type_mask)));
                    
                    last_object->attach_child_node(_loss_plugin, last_object->get_xip2_addr(),std::string());
                    _loss_plugin->release_ref();
                    
                    last_object = _loss_plugin;
                }
                m_test_account = new xtestaccount_t(*last_object,account);
                last_object->attach_child_node(m_test_account, get_xip2_addr(),std::string());
            }
            return true;
        }
        
        //send clock event to child objects
        bool   xtestnode_t::fire_clock(base::xvblock_t & latest_clock_block,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            //get_vblockstore()->store_block(&latest_clock_block);
            return xcsobject_t::fire_clock(latest_clock_block,cur_thread_id,timenow_ms);
        }
        
        bool  xtestnode_t::on_txs_recv(const std::string & account,const std::string & txs)
        {
            std::deque<std::string> & queue = m_txs_pool[account];
            if(queue.size() > 256) //too much txs
                return false;
            
            queue.push_back(txs);
            return true;
        }
        
        base::xvblock_t*   xtestnode_t::create_proposal_block(const std::string & account,const std::string & block_input,const std::string & block_output,const uint64_t new_viewid)
        {
            base::xauto_ptr<base::xvblock_t> last_block = get_vblockstore()->get_latest_cert_block(account); //return ptr that has been added reference
            if(last_block == nullptr)//blockstore has been closed
                return NULL;
        
            base::xauto_ptr<base::xvblock_t> last_full_block = get_vblockstore()->get_latest_committed_block(account);
            base::xvblock_t* new_proposal_block = xunitblock_t::create_unitblock(account,last_block->get_height() + 1,0,new_viewid,last_block->get_block_hash(),last_full_block->get_block_hash(),last_full_block->get_height(),block_input,block_output);
            new_proposal_block->reset_prev_block(last_block.get()); //point previous block
            return new_proposal_block;
        }
        
        bool   xtestnode_t::start_consensus(base::xvblock_t * proposal_block)
        {
            if(get_child_node() == NULL)
            {
                xtestaccount_t * new_account = new xtestaccount_t(*this,proposal_block->get_account());
                attach_child_node(new_account, get_xip2_addr(),std::string());
                new_account->release_ref();
            }
            if(NULL != proposal_block)
            {
                return m_test_account->fire_proposal_start_event(proposal_block);
            }
            return false;
        }
                
        int    xtestnode_t::verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child)
        {
            base::xauto_ptr<base::xvblock_t> _latest_existing_block = get_vblockstore()->get_latest_cert_block(proposal_block->get_account());
            if(proposal_block->get_last_block_hash() != _latest_existing_block->get_block_hash())
            {
                return xconsensus::enum_xconsensus_error_fail;
            }
            return xconsensus::enum_xconsensus_code_successful;
        }
        
        //send packet from this object to parent layers
        bool    xtestnode_t::send_out(const xvip2_t & from_addr,const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            if( (get_types() & enum_xtestnode_role_mask) == enum_xtestnode_role_offline) //always drop packet
            {
                xdbg("xtestnode_t::send_out,offline node drop packet,from=%llu to %llu at node:%llu",from_addr.low_addr,to_addr.low_addr,get_xip2_low_addr());
                return true;
            } 
            return xcsobject_t::send_out(from_addr,to_addr,packet,cur_thread_id,timenow_ms);
        }
        
        //recv_in packet from this object to child layers
        bool    xtestnode_t::recv_in(const xvip2_t & from_addr,const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            if( (get_types() & enum_xtestnode_role_mask) == enum_xtestnode_role_offline) //always drop packet
            {
                xdbg("xtestnode_t::recv_in,offline node drop packet,from=%llu to %llu at node:%llu",from_addr.low_addr,to_addr.low_addr,get_xip2_low_addr());
                return true;
            }
            return xcsobject_t::recv_in(from_addr,to_addr,packet,cur_thread_id,timenow_ms);
        }
    
        bool    xtestnode_t::fire_proposal()
        {
            xdbg("xtestnode_t::fire_proposal,elect to leader by viewid=%lld and total nodes=%d at node=%llx",m_latest_viewid,m_total_nodes,get_xip2_low_addr());
            
            std::string txs;
            if(m_txs_pool[m_test_account->get_account()].empty() == false)
            {
                txs = m_txs_pool[m_test_account->get_account()].front();
                m_txs_pool[m_test_account->get_account()].pop_front();
            }
            
            base::xvblock_t*  proposal_block = create_proposal_block(m_test_account->get_account(),txs,txs,m_latest_viewid);
            if(proposal_block != NULL)
            {
                xvip2_t leader_xip =  get_xip2_addr();
                proposal_block->get_cert()->set_validator(leader_xip);
                
                start_consensus(proposal_block);
                
                proposal_block->release_ref();//safe to release it
                return true;
            }
            return false;
        }
        
        //on_view_change event
        bool    xtestnode_t::on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus::xcsview_fire* _ev_obj = (xconsensus::xcsview_fire*)&event;
            m_latest_viewid = _ev_obj->get_viewid();
            
            const uint64_t elect_leader_viewid = m_latest_viewid;
            if( (get_xip2_low_addr() & 0x3FF) == ((elect_leader_viewid % m_total_nodes)) )//it is leader
            {
                xdbg("xtestnode_t::on_view_fire,elect to leader by viewid=%lld and total nodes=%d at node=%llx",m_latest_viewid,m_total_nodes,get_xip2_low_addr() & 0x3FF);
                
                fire_proposal();
            }
            return true;
        }
 
        bool  xtestnode_t::on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //triggered by push_event_up
        {
            switch(event.get_type())
            {
                case xconsensus::enum_xcsevent_type_on_proposal_finish:
                {
                    xconsensus::xproposal_finish * _evt_obj = (xconsensus::xproposal_finish*)&event;
                    if(_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful)
                    {
                        base::xvnodesrv_t * nodesvr = (base::xvnodesrv_t *)query_plugin(std::string("*/") + base::xvnodesrv_t::name());
                        if(nodesvr != NULL)
                        {
                            auto _block = _evt_obj->get_target_proposal();
                            std::vector<base::xvoter> voters = auth::xauthcontext_t::query_validators(*_block, *nodesvr);
                            xdbg("xauthcontext_t::query_validators,return size:%d",(int)voters.size());
                            for(auto it = voters.begin(); it != voters.end(); ++it)
                            {
                                base::xvoter & node = *it;
                                if(node.is_voted)
                                {
                                    if(node.is_leader)
                                        xdbg("xauthcontext_t::query_validators-> leader={%s} YES for block={height:%" PRIu64 ",viewid:%" PRIu64 "}",node.account.c_str(),_block->get_height(),_block->get_viewid());
                                    else
                                        xdbg("xauthcontext_t::query_validators-> replica={%s} YES for block={height:%" PRIu64 ",viewid:%" PRIu64 "}",node.account.c_str(),_block->get_height(),_block->get_viewid());
                                }
                                else
                                {
                                    if(node.is_leader)
                                        xdbg("xauthcontext_t::query_validators-> leader={%s} NO for block={height:%" PRIu64 ",viewid:%" PRIu64 "}",node.account.c_str(),_block->get_height(),_block->get_viewid());
                                    else
                                        xdbg("xauthcontext_t::query_validators-> replica={%s} NO for block={height:%" PRIu64 ",viewid:%" PRIu64 "}",node.account.c_str(),_block->get_height(),_block->get_viewid());
                                }
                            }
                        }
                    }
                    break;
                }
            }
            return xconsensus::xcsobject_t::on_event_up(event,from_child,cur_thread_id,timenow_ms);
        }

    };
};
