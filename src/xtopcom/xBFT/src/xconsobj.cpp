// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xconsobj.h"
#include "xbase/xhash.h"
#include "xdata/xnative_contract_address.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace xconsensus
    {
        const std::string  xcsobject_t::get_xclock_account_address() //find the account address of global clock-contract
        {
            return sys_contract_beacon_timer_addr;
        }
        
        //////////////////////////////////////////xcsobject_t////////////////////////////////////////////////
        xcsobject_t::xcsobject_t(xcsobject_t & parent_object,base::enum_xobject_type obj_type)
            :base::xionode_t(parent_object,(base::enum_xobject_type)obj_type)
        {
            m_vcertauth_plugin      = NULL;
            m_vblockstore_plugin    = NULL;
            m_workerpool_plugin     = NULL;
        }
        
        xcsobject_t::xcsobject_t(base::xcontext_t & _context,const int32_t target_thread_id,base::enum_xobject_type obj_type)
            :base::xionode_t(_context,target_thread_id,(base::enum_xobject_type)obj_type)
        {
            m_vcertauth_plugin      = NULL;
            m_vblockstore_plugin    = NULL;
            m_workerpool_plugin     = NULL;
        }

        xcsobject_t::~xcsobject_t()
        {
            if(m_vcertauth_plugin != NULL)
                m_vcertauth_plugin->release_ref();

            if(m_vblockstore_plugin != NULL)
                m_vblockstore_plugin->release_ref();

            if(m_workerpool_plugin != NULL)
                m_workerpool_plugin->release_ref();
        }

        void*   xcsobject_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(enum_xconsensus_object_type_base == _enum_xobject_type_)
                return this;

            return base::xionode_t::query_interface(_enum_xobject_type_);
        }

        base::xvcertauth_t*     xcsobject_t::get_vcertauth()
        {
            if(m_vcertauth_plugin != NULL)
                return m_vcertauth_plugin;

            //load base::xvcertauth_t  by searching from lower layer
            m_vcertauth_plugin = (base::xvcertauth_t*)query_plugin(std::string("*/") + base::xvcertauth_t::name());
            return m_vcertauth_plugin;
        }
        void  xcsobject_t::set_vcertauth(base::xvcertauth_t* new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            base::xvcertauth_t* old_ptr = base::xatomic_t::xexchange(m_vcertauth_plugin, new_ptr);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }

        base::xvblockstore_t*   xcsobject_t::get_vblockstore()
        {
            if(m_vblockstore_plugin != NULL)
                return m_vblockstore_plugin;

            //load xblockstore_t by searching from lower layer
            m_vblockstore_plugin = (base::xvblockstore_t*)query_plugin(std::string("*/") + base::xvblockstore_t::name());
            return m_vblockstore_plugin;
        }
        void  xcsobject_t::set_vblockstore(base::xvblockstore_t* new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            base::xvblockstore_t* old_ptr = base::xatomic_t::xexchange(m_vblockstore_plugin, new_ptr);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }

        base::xworkerpool_t*    xcsobject_t::get_workerpool()
        {
            if(NULL != m_workerpool_plugin)
                return m_workerpool_plugin;

            //load workerpool by searching from lower layer,note: workerpool is optional
            m_workerpool_plugin = (base::xworkerpool_t*)query_plugin(std::string("*/") + base::xworkerpool_t::name());
            return m_workerpool_plugin;
        }
        void  xcsobject_t::set_workerpool(base::xworkerpool_t* new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            base::xworkerpool_t* old_ptr = base::xatomic_t::xexchange(m_workerpool_plugin, new_ptr);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
        
        int   xcsobject_t::verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) //load and execute block at sanbox
        {
            xcsobject_t * parent_obj = (xcsobject_t*)get_parent_node();
            if(parent_obj != NULL)
                return parent_obj->verify_proposal(proposal_block,bind_clock_cert,this);

            return enum_xconsensus_error_not_handled;//not handled
        }

        bool  xcsobject_t::on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //triggered by push_event_up
        {
            switch(event.get_type())
            {
                case base::enum_xevent_core_type_pdu:
                    return on_pdu_event_up(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                    
                case base::enum_xevent_core_type_clock:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_up);//add route path indication
                    return on_clock_fire(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                }

                case enum_xcsevent_type_on_view_fire:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_up); //add route path indication
                    return on_view_fire(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                }

                case base::enum_xevent_core_type_create_block:
                {
                    xdbg("xcsobject_t::on_event_up enum_xevent_core_type_create_block(nathan)");
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_up); //add route path indication
                    return on_create_block_event(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                }

                case base::enum_xevent_core_type_tc:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_up); //add route path indication
                    return on_time_cert_event(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                }
            }
            return false; //return false to let event continuely throw up
        }

        bool  xcsobject_t::on_event_down(const base::xvevent_t & event,xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) //triggered by push_event_down
        {
            switch(event.get_type())
            {
                case base::enum_xevent_core_type_pdu:
                    return on_pdu_event_down(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);
                    
                case base::enum_xevent_core_type_clock:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_down);//add route path indication
                    return on_clock_fire(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);
                }

                case enum_xcsevent_type_on_view_fire:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_down);//add route path indication
                    return on_view_fire(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);
                }

                case base::enum_xevent_core_type_create_block:
                {
                    xdbg("xcsobject_t::on_event_down enum_xevent_core_type_create_block(nathan)");
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_down);//add route path indication
                    return on_create_block_event(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);
                }
            }
            return false; //return false to let event continuely push down
        }

        bool  xcsobject_t::on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely throw up
        }

        bool  xcsobject_t::on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely push down
        }

        //clock block always pass by higher layer to lower layer
        bool  xcsobject_t::on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely push down
        }

        bool  xcsobject_t::on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely push down
        }

        bool  xcsobject_t::on_time_cert_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely push down
        }

        //fire view-change event
        bool  xcsobject_t::on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false; //return false to let event continuely push up or down
        }

        //send clock event to child objects
        bool   xcsobject_t::fire_clock(base::xvblock_t & latest_clock_block,int32_t cur_thread_id,uint64_t timenow_ms)
        {
             if(get_child_node() != NULL)
             {
                 base::xauto_ptr<xcsclock_fire>_event_obj(new xcsclock_fire(latest_clock_block));
                 return get_child_node()->push_event_down(*_event_obj, this, cur_thread_id, timenow_ms);
             }
             return false;
        }

        //dispatch view-change event to both upper(parent objects) and lower layers(child objects)
        bool   xcsobject_t::fire_view(const std::string & target_account,const uint64_t new_view_id,const uint64_t global_clock,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            base::xauto_ptr<xcsview_fire>_event_obj(new xcsview_fire(target_account,new_view_id,global_clock));

            if(get_child_node() != NULL)//as default event path is going down
                get_child_node()->push_event_down(*_event_obj, this, cur_thread_id, timenow_ms);

            if(get_parent_node() != NULL)
            {
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                 get_parent_node()->push_event_up(*_event_obj, this, cur_thread_id, timenow_ms);
            }
            return false;
        }
        
        //send-out supposed that current layer/object fired this event so it dose not need know this again
        bool   xcsobject_t::send_out(const xvip2_t & from_addr, const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            if(get_parent_node() != NULL)
            {
                xdbg("csobject sendout src %" PRIx64 ".%" PRIx64 " dst %" PRIx64 ".%" PRIx64, from_addr.low_addr, from_addr.high_addr, to_addr.low_addr, to_addr.high_addr);
                base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire(packet));
                _event_obj->set_from_xip(from_addr);
                _event_obj->set_to_xip(to_addr);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, cur_thread_id, timenow_ms);
            }
            return false;
        }
        
        //receive-in should give chance of  this layer handle as well,so start from current object then go down
        bool   xcsobject_t::recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms)
        {
            xdbg("csobject recvin src %" PRIx64 ".%" PRIx64 " dst %" PRIx64 ".%" PRIx64, from_addr.low_addr, from_addr.high_addr, to_addr.low_addr, to_addr.high_addr);
            base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire(packet));
            _event_obj->set_from_xip(from_addr);
            _event_obj->set_to_xip(to_addr);
            push_event_down(*_event_obj, this, cur_thread_id, timenow_ms);
            return true;
        }

        //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
        bool    xcsobject_t::on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode)
        {
            //double check whether childnode is xcsobject_t family
            xassert(childnode != NULL);
            void* ptr_consobject = childnode->query_interface(enum_xconsensus_object_type_base);
            xassert(ptr_consobject != NULL);
            if(NULL == ptr_consobject)
                return false;

            //go default handle
            return base::xionode_t::on_child_node_join(error_code,cur_thread_id,timenow_ms,childnode);
        }

        //notify this node that is joined into parent-node
        bool    xcsobject_t::on_join_parent_node(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,const xvip2_t & alloc_address,const std::string & extra_data,xionode_t* from_parent)
        {
            //double check whether parent node is xcsobject_t family
            xassert(from_parent != NULL);
            void* ptr_consobject = from_parent->query_interface(enum_xconsensus_object_type_base);
            xassert(ptr_consobject != NULL);
            if(NULL == ptr_consobject)
                return false;

            //go default handle first
            base::xionode_t::on_join_parent_node(error_code,cur_thread_id,timenow_ms,alloc_address,extra_data,from_parent);
            
            //then to query plugins
            get_vcertauth();
            get_vblockstore();
            get_workerpool();

            return true;
        }

        bool  xcsobject_t::fire_asyn_job(base::xfunction_t job_at_woker_thread,base::xfunction_t callback_to_object_thread)
        {
            base::xworkerpool_t * _workers_pool = get_workerpool();
            auto _internal_asyn_function2 = [this](base::xcall_t & call, const int32_t cur_thread_id,const uint64_t timenow_ms)->bool{

                base::xfunction_t* _job_ = (base::xfunction_t *)call.get_param1().get_function();
                base::xfunction_t* _callback_ = (base::xfunction_t *)call.get_param2().get_function();
                
                (*_job_)(NULL);

                this->dispatch_call(*_callback_,(void*)0); //send back to object'host thread
                return true;
            };
            base::xcall_t asyn_call(_internal_asyn_function2,&job_at_woker_thread,&callback_to_object_thread,(base::xobject_t*)this);
            if(0 == _workers_pool) //at current thread
            {
                if(send_call(asyn_call) == enum_xcode_successful)
                    return true;
            }
            else
            {
                if(_workers_pool->send_call(asyn_call) == enum_xcode_successful)
                    return true;
            }
            xerror("xcsobject_t::fire_asyn_job2,capture exception to handle aysn_job, failover to process at this thread(%d)",get_current_thread_id());
            return false;
        }
        //////////////////////////////////////////xcsobject_t////////////////////////////////////////////////

        //////////////////////////////////////////xcscoreobj_t////////////////////////////////////////////////
        xcscoreobj_t::xcscoreobj_t(base::xcontext_t & _context,const int32_t target_thread_id,base::enum_xobject_type type,const std::string & account_addr)
            :xcsobject_t(_context,target_thread_id,type),
             base::xvaccount_t(account_addr)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xcscoreobj_t, 1);
        }

        xcscoreobj_t::xcscoreobj_t(xcscoreobj_t & parentobj,base::enum_xobject_type type)
            :xcsobject_t(parentobj,type),
             base::xvaccount_t(parentobj.get_account())
        {
            base::xionode_t::reset_xip_addr(parentobj.get_xip2_addr());//force update xip2 address by following parent
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xcscoreobj_t, 1);
        }

        xcscoreobj_t::~xcscoreobj_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xcscoreobj_t, -1);
        };

        void* xcscoreobj_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xconsensus_object_type_core)
                return this;

            return xcsobject_t::query_interface(_enum_xobject_type_);
        }

        bool  xcscoreobj_t::reset_xip_addr(const xvip2_t & new_addr)
        {
            if( (new_addr.high_addr != get_xip2_high_addr()) || (new_addr.low_addr != get_xip2_low_addr()))
            {
                base::xionode_t::reset_xip_addr(new_addr);
                if(get_child_node() != NULL)
                    ((xcscoreobj_t*)get_child_node())->reset_xip_addr(new_addr);
            }
            return true;
        }
        
        bool  xcscoreobj_t::on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //triggered by push_event_up
        {
            switch(event.get_type())
            {
                case enum_xcsevent_type_on_proposal_finish:
                    return on_proposal_finish(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);

                case enum_xcsevent_type_on_consensus_commit:
                    return on_consensus_commit(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);

                case enum_xcsevent_type_on_consensus_update:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_up);//add route path indication
                    return on_consensus_update(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);
                }
                    
                case enum_xcsevent_type_on_replicate_finish:
                    return on_replicate_finish(event, (xcsobject_t*)from_child, cur_thread_id, timenow_ms);
                    
                case enum_xcsevent_type_on_certificate_finish:
                    return on_certificate_finish(event,(xcsobject_t*)from_child,cur_thread_id,timenow_ms);

            }
            return xcsobject_t::on_event_up(event,from_child,cur_thread_id,timenow_ms);
        }

        bool  xcscoreobj_t::on_event_down(const base::xvevent_t & event,xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) //triggered by push_event_down
        {
            switch(event.get_type())
            {
                case enum_xcsevent_type_on_proposal_start:
                    return on_proposal_start(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);

                case enum_xcsevent_type_on_consensus_update:
                {
                    ((base::xvevent_t &)event).set_route_path(base::enum_xevent_route_path_down);//add route path indication
                    return on_consensus_update(event,(xcsobject_t*)from_parent,cur_thread_id,timenow_ms);
                }
            }
            return xcsobject_t::on_event_down(event,from_parent,cur_thread_id,timenow_ms);
        }

        bool  xcscoreobj_t::on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)

        {
            return false;//return false to let event continuely push down
        }

        bool  xcscoreobj_t::on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely throw up
        }
        
        bool  xcscoreobj_t::on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely throw up
        }

        bool  xcscoreobj_t::on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely throw up or down
        }

        //call from lower layer to higher layer(parent)
        bool  xcscoreobj_t::on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false; //return false to let event continuely throw up
        }
        
        //call from lower layer to higher layer(parent)
        bool  xcscoreobj_t::on_certificate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;//return false to let event continuely throw up
        }
        
        //proposal_start_event should give chance of  this layer handle as well,so start from current object then go down
        bool   xcscoreobj_t::fire_proposal_start_event(base::xvblock_t*proposal_block)
        {
            base::xauto_ptr<xproposal_start>_event_obj(new xproposal_start(proposal_block));
            return push_event_down(*_event_obj, this, get_current_thread_id(), get_time_now());
        }
        //proposal_start_event should give chance of  this layer handle as well,so start from current object then go down
        bool   xcscoreobj_t::fire_proposal_start_event(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block)
        {
            base::xauto_ptr<xproposal_start>_event_obj(new xproposal_start());
            _event_obj->set_latest_commit(latest_commit_block);
            _event_obj->set_latest_lock(latest_lock_block);
            _event_obj->set_latest_cert(latest_cert_block);
            return push_event_down(*_event_obj, this, get_current_thread_id(), get_time_now());
        }

        bool xcscoreobj_t::fire_proposal_finish_event(base::xvblock_t* target_proposal,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block,base::xvblock_t* latest_proposal_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xproposal_finish>_event_obj(new xproposal_finish(target_proposal));
                _event_obj->set_latest_commit(latest_commit_block);
                _event_obj->set_latest_lock(latest_lock_block);
                _event_obj->set_latest_cert(latest_cert_block);
                _event_obj->set_latest_proposal(latest_proposal_block);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }

        bool xcscoreobj_t::fire_proposal_finish_event(const int errcode,const std::string & err_detail,base::xvblock_t* target_proposal,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block,base::xvblock_t* latest_proposal_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xproposal_finish>_event_obj(new xproposal_finish(errcode,err_detail,target_proposal));
                _event_obj->set_latest_commit(latest_commit_block);
                _event_obj->set_latest_lock(latest_lock_block);
                _event_obj->set_latest_cert(latest_cert_block);
                _event_obj->set_latest_proposal(latest_proposal_block);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }

        //consensus_commit_event always go up from lower layer
        bool xcscoreobj_t::fire_consensus_commit_event(base::xvblock_t* target_commit,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xconsensus_commit>_event_obj(new xconsensus_commit(target_commit));
                _event_obj->set_latest_commit(latest_commit_block);
                _event_obj->set_latest_lock(latest_lock_block);
                _event_obj->set_latest_cert(latest_cert_block);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }

        bool xcscoreobj_t::fire_consensus_update_event_up(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xconsensus_update>_event_obj(new xconsensus_update());
                _event_obj->set_latest_commit(latest_commit_block);
                _event_obj->set_latest_lock(latest_lock_block);
                _event_obj->set_latest_cert(latest_cert_block);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }

        bool xcscoreobj_t::fire_consensus_update_event_down(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block)
        {
            if(get_child_node() != NULL)
            {
                base::xauto_ptr<xconsensus_update>_event_obj(new xconsensus_update());
                _event_obj->set_latest_commit(latest_commit_block);
                _event_obj->set_latest_lock(latest_lock_block);
                _event_obj->set_latest_cert(latest_cert_block);
 
                return get_child_node()->push_event_down(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }
        
        //certifcate_finish always go up from lower layer
        bool   xcscoreobj_t::fire_replicate_finish_event(base::xvblock_t * target_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xreplicate_finish>_event_obj(new xreplicate_finish(target_block));
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }
        
        //certifcate_finish always go up from lower layer
        bool    xcscoreobj_t::fire_certificate_finish_event(base::xvqcert_t* target_cert)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xcertificate_finish>_event_obj(new xcertificate_finish(target_cert));
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_current_thread_id(), get_time_now());
            }
            return false;
        }
        
        bool   xcscoreobj_t::fire_pdu_event_up(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire());

                _event_obj->set_from_xip(from_addr);
                _event_obj->set_to_xip(to_addr);
                _event_obj->_packet.set_block_chainid(for_block->get_chainid());
                _event_obj->_packet.set_block_account(for_block->get_account());
                _event_obj->_packet.set_block_height(for_block->get_height());
                _event_obj->_packet.set_block_clock(for_block->get_clock());

                _event_obj->_packet.set_block_viewid(for_block->get_viewid());
                _event_obj->_packet.set_block_viewtoken(for_block->get_viewtoken());
                
                _event_obj->_packet.reset_message(msg_type, get_default_msg_ttl(),msg_content,msg_nonce,from_addr.low_addr,to_addr.low_addr);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_thread_id(), get_time_now());
            }
            return false;
        }
        
        bool   xcscoreobj_t::fire_pdu_event_up(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block,const std::string & vblock_cert_bin,const std::string & vlatest_clock_cert)
        {
            if(get_parent_node() != NULL)
            {
                base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire());

                _event_obj->set_from_xip(from_addr);
                _event_obj->set_to_xip(to_addr);
                _event_obj->_packet.set_block_chainid(for_block->get_chainid());
                _event_obj->_packet.set_block_account(for_block->get_account());
                _event_obj->_packet.set_block_height(for_block->get_height());
                _event_obj->_packet.set_block_clock(for_block->get_clock());
                _event_obj->_packet.set_vblock_cert(vblock_cert_bin);
                _event_obj->_packet.set_xclock_cert(vlatest_clock_cert);
                
                _event_obj->_packet.set_block_viewid(for_block->get_viewid());
                _event_obj->_packet.set_block_viewtoken(for_block->get_viewtoken());
                
                _event_obj->_packet.reset_message(msg_type, get_default_msg_ttl(),msg_content,msg_nonce,from_addr.low_addr,to_addr.low_addr);
                _event_obj->set_route_path(base::enum_xevent_route_path_up);
                return get_parent_node()->push_event_up(*_event_obj, this, get_thread_id(), get_time_now());
            }
            return false;
        }
        
        bool   xcscoreobj_t::fire_pdu_event_down(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block)
        {
            if(get_child_node() != NULL)
            {
                base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire());

                _event_obj->set_from_xip(from_addr);
                _event_obj->set_to_xip(to_addr);
                _event_obj->_packet.set_block_chainid(for_block->get_chainid());
                _event_obj->_packet.set_block_account(for_block->get_account());
                _event_obj->_packet.set_block_height(for_block->get_height());
                _event_obj->_packet.set_block_clock(for_block->get_clock());
                
                _event_obj->_packet.set_block_viewid(for_block->get_viewid());
                _event_obj->_packet.set_block_viewtoken(for_block->get_viewtoken());

                _event_obj->_packet.reset_message(msg_type, get_default_msg_ttl(),msg_content,msg_nonce,from_addr.low_addr,to_addr.low_addr);

                return get_child_node()->push_event_down(*_event_obj, this, get_thread_id(), get_time_now());
            }
            return false;
        }
        //////////////////////////////////////////xcscoreobj_t////////////////////////////////////////////////

    };//end of namespace of xconsensus

};//end of namespace of top
