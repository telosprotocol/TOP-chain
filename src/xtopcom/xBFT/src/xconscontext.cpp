// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconscontext.h"

namespace top
{
    namespace xconsensus
    {
        //////////////////////////////////////////xcscontext_t////////////////////////////////////////////////
        //wrap function to create xcscontext_t and driver, and attach them
        xcscontext_t *  xcscontext_t::create_context_object(xcspacemaker_t&  parent_object)
        {
            xcscontext_t * new_context_object = new xBFTcontext_t(parent_object);
            
            xvip2_t alloc_address;
            alloc_address.high_addr = 0;
            alloc_address.low_addr = 0;
            parent_object.attach_child_node(new_context_object,alloc_address,std::string());//attach_child_node may include additonal reference
            return new_context_object;
        }
        
        //////////////////////////////////////////xthcontext_t////////////////////////////////////////////////
        
        xBFTcontext_t::xBFTcontext_t(xcscoreobj_t& parent_object)
            :xcscontext_t(parent_object)
        {
            m_latest_commit_block  = NULL;
            m_latest_lock_block    = NULL;
            xinfo("xBFTcontext_t::xBFTcontext_t,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
                
        xBFTcontext_t::~xBFTcontext_t()
        {
            xinfo("xBFTcontext_t::~xBFTcontext_t,destroy,this=%p",this);
            
            for(auto it = m_hash_pool.begin(); it != m_hash_pool.end();it++)
            {
                it->second->close(false);    //close this node only
                it->second->release_ref();   //release reference of pool holding
            }
            m_hash_pool.clear();
            m_view_pool.clear();
            
            if(m_latest_lock_block != NULL)
                m_latest_lock_block->release_ref();
            
            if(m_latest_commit_block != NULL)
                m_latest_commit_block->release_ref();
        }
        
        bool xBFTcontext_t::update_lock_block(base::xvblock_t * _new_lock)
        {
            if(NULL == _new_lock)
                return false;
            
            if(   (false == _new_lock->check_block_flag(base::enum_xvblock_flag_locked))
               && (false == _new_lock->check_block_flag(base::enum_xvblock_flag_committed))
               )
            {
                xerror("xBFTcontext_t::update_lock_block,fail-try replace by un-locked block=%s,at node=0x%llx",_new_lock->dump().c_str(),get_xip2_low_addr());
                return false;
            }
            
            if(m_latest_lock_block == _new_lock)
                return false;
            
            if(NULL == m_latest_lock_block)
            {
                _new_lock->add_ref();
                m_latest_lock_block = _new_lock;
                
                xkinfo("xBFTcontext_t::update_lock_block at node=0x%llx,nil-lock -> new_lock=%s,this=%llx",get_xip2_addr().low_addr,_new_lock->dump().c_str(),(int64_t)this);
                return true;
            }
            else
            {
                bool allow_update = false;
                if(   (m_latest_lock_block->get_viewid() < _new_lock->get_viewid())
                   && (m_latest_lock_block->get_height() < _new_lock->get_height()) )
                {
                    allow_update = true;
                }
 
                if(allow_update)
                {
                    if(   (_new_lock->get_chainid() == m_latest_lock_block->get_chainid())
                       && (_new_lock->get_account() == m_latest_lock_block->get_account()))
                    {
                        xdbg("xBFTcontext_t::update_lock_block at node=0x%llx,old_lock=%s --> new_lock=%s,this=%llx",get_xip2_addr().low_addr,m_latest_lock_block->dump().c_str(),_new_lock->dump().c_str(),(int64_t)this);
                        
                        _new_lock->add_ref();
                        base::xvblock_t* old_ptr = base::xatomic_t::xexchange(m_latest_lock_block, _new_lock);
                        old_ptr->release_ref();
                        
                        return true;
                    }
                    xerror("xBFTcontext_t::update_lock_block,fail-unmatched account of block=%s vs this=%s, at node=0x%llx",_new_lock->dump().c_str(),m_latest_lock_block->get_account().c_str(),get_xip2_low_addr());
                }

            }
            return false;
        }
        
        bool xBFTcontext_t::update_commit_block(base::xvblock_t * _new_commit)
        {
            if(NULL == _new_commit)
                return false;
            
            if(  (false == _new_commit->check_block_flag(base::enum_xvblock_flag_committed))
              || (false == _new_commit->check_block_flag(base::enum_xvblock_flag_authenticated)) )
            {
                xerror("xBFTcontext_t::update_commit_block,fail-try replace by un-commited block=%s,at node=0x%llx",_new_commit->dump().c_str(),get_xip2_low_addr());
                return false;
            }
            
            if(m_latest_commit_block == _new_commit)
                return false;
            
            if(NULL == m_latest_commit_block)
            {
                _new_commit->add_ref();
                m_latest_commit_block = _new_commit;
                
                xkinfo("xBFTcontext_t::update_commit_block at node=0x%llx,nil-commit -> new_commit=%s,this=%llx",get_xip2_addr().low_addr,_new_commit->dump().c_str(),(int64_t)this);
                
                update_lock_block(m_latest_commit_block);//sync to lock block as initialize
                return true;
            }
            else if( (m_latest_commit_block->get_viewid() < _new_commit->get_viewid()) && (m_latest_commit_block->get_height() < _new_commit->get_height()) )
            {
                if( (_new_commit->get_chainid() == m_latest_commit_block->get_chainid())
                 && (_new_commit->get_account() == m_latest_commit_block->get_account()))
                {
                    xdbg("xBFTcontext_t::update_commit_block at node=0x%llx,old_commit=%s --> new_commit=%s,this=%llx",get_xip2_addr().low_addr,m_latest_commit_block->dump().c_str(),_new_commit->dump().c_str(),(int64_t)this);
                    
                    _new_commit->add_ref();
                    base::xvblock_t* old_ptr = base::xatomic_t::xexchange(m_latest_commit_block, _new_commit);
                    old_ptr->release_ref();
                    
                    return true;
                }
                xerror("xBFTcontext_t::update_commit_block,fail-unmatched account of block=%s vs this=%s, at node=0x%llx",_new_commit->dump().c_str(),m_latest_commit_block->get_account().c_str(),get_xip2_low_addr());
            }
            return false;
        }
        
        //any block must be a valid certified block at context layer
        bool  xBFTcontext_t::safe_check_for_block(base::xvblock_t * _block)
        {
            if(NULL == _block)
                return false;
            
            if(false == _block->is_deliver(false))
            {
                xerror("xBFTcontext_t::safe_check_for_block,undeliver block=%s",_block->dump().c_str());
                return false;
            }
            return true;
        }
        
        //more restrict condition that must newer than m_latest_commit_block
        bool xBFTcontext_t::safe_check_for_cert_block(base::xvblock_t* _block)
        {
            if(  (NULL == _block)
               ||(_block->check_block_flag(base::enum_xvblock_flag_committed)) )//block has been committed
            {
                return false;
            }
            if(false == _block->is_input_ready(true)) //put warning to identify it,may remove warning at release
            {
                xwarn("xBFTcontext_t::safe_check_for_cert_block,input is not ready for block=%s",_block->dump().c_str());
            }
            
            if(NULL != m_latest_lock_block)
            {
                if(   (_block->get_height() <= m_latest_lock_block->get_height())
                   || (_block->get_viewid() <= m_latest_lock_block->get_viewid())
                   || (_block->get_chainid() != m_latest_lock_block->get_chainid())
                   || (_block->get_account() != m_latest_lock_block->get_account())
                   )
                {
                    return false;
                }
            }
            if(false == safe_check_for_block(_block))
                return false;
            
            return true;
        }
        
        //execute 3-chain process
        bool  xBFTcontext_t::do_execute(base::xvbnode_t * new_node)
        {
            if(NULL == new_node) //new_node must has been tested by is_deliver() before call this
                return false;
            
            if(new_node->get_obj_flags() == 0)//it has been deleted from pool logically
            {
                xerror("xBFTcontext_t::do_execute,try execute a deleted block at node=0x%llx,high-qc=%s",get_xip2_addr().low_addr,new_node->get_block()->dump().c_str());
                return false; //should not happen,here just add protection
            }
            else
            {
                //step#1: update highest qc-cert block if need
                xdbg("xBFTcontext_t::do_execute at node=0x%llx,high-qc=%s",get_xip2_addr().low_addr,new_node->get_block()->dump().c_str());
            }
        
            //step#2: check and set latest lock block
            base::xvbnode_t* last_cert_node = new_node->get_parent();
            if(   (NULL == last_cert_node)
               || (last_cert_node->get_obj_flags() == 0) //it has been deleted from pool logically,and will completely delete later
               )
            {
                return false;
            }
            last_cert_node->get_block()->set_block_flag(base::enum_xvblock_flag_locked);//changed to lock status
            last_cert_node->set_obj_flag(base::enum_xvblock_flag_locked); //mark as lock node
            last_cert_node->reset_child(new_node);//do forward connection
            
            update_lock_block(last_cert_node->get_block());
            xinfo("xBFTcontext_t::do_execute at node=0x%llx,lock-qc=%s",get_xip2_addr().low_addr,last_cert_node->get_block()->dump().c_str());
            
            //step#3: check and set latest commit block
            base::xvbnode_t* last_last_cert_node = last_cert_node->get_parent();
            if(   (NULL == last_last_cert_node)
               || (last_last_cert_node->get_obj_flags() == 0)//it has been deleted from pool logically,and will completely delete later
               )
            {
                return true; //indicated move to locked node
            }
            last_last_cert_node->get_block()->set_block_flag(base::enum_xvblock_flag_locked);//ensure been locked status
            last_last_cert_node->get_block()->set_block_flag(base::enum_xvblock_flag_committed);//change to commit status
            last_last_cert_node->set_obj_flag(base::enum_xvblock_flag_committed);//mark as committed node
            last_last_cert_node->get_block()->set_next_next_cert(new_node->get_block()->get_cert()); //record who push it as commited
            last_last_cert_node->reset_child(last_cert_node);//do forward connection
            
            update_commit_block(last_last_cert_node->get_block());
            
            xinfo("xBFTcontext_t::do_execute at node=0x%llx,commit-qc=%s",get_xip2_addr().low_addr,last_last_cert_node->get_block()->dump().c_str());
            
            base::xvbnode_t * last_commit_node = last_last_cert_node;
            for(base::xvbnode_t * node_it = last_commit_node->get_parent(); node_it != NULL; node_it = node_it->get_parent())
            {
                if(node_it->get_obj_flags() != 0) //0 means has been deleted from pool logically,and will completely delete later
                {
                    node_it->get_block()->set_block_flag(base::enum_xvblock_flag_committed);//change to commit status
                    node_it->set_obj_flag(base::enum_xvblock_flag_committed); //mark as commit node
                    node_it->reset_child(last_commit_node); //do forward connection
                    node_it->get_block()->set_next_next_cert(last_commit_node->get_child()->get_cert());
                    last_commit_node = node_it; //update last_commit_node
                }
                else
                {
                    break;
                }
            }
            return true;
        }

        //cache and reorganize blocks
        bool  xBFTcontext_t::do_update(base::xvblock_t * new_block)
        {
            if(false == safe_check_for_cert_block(new_block))
                return false;
            
            if(m_hash_pool.find(new_block->get_block_hash()) != m_hash_pool.end()) //duplicated one
            {
                xwarn("xBFTcontext_t::do_update,found duplicated block:%s at node=0x%llx",new_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            auto set_test_res = m_view_pool.emplace(new_block->get_viewid());
            if(false == set_test_res.second)
            {
                xwarn("xBFTcontext_t::do_update,found same view# for block:%s at node=0x%llx",new_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //step#1: find any parent node that link to this new node
            base::xvbnode_t *  new_node = NULL;
            auto parent_it = m_hash_pool.find(new_block->get_last_block_hash());
            if(parent_it != m_hash_pool.end()) //found parent node
            {
                new_node = new base::xvbnode_t(parent_it->second,*new_block);//link each other
                new_node->set_obj_flag(base::enum_xvblock_flag_authenticated); //mark as authenticated node
                m_hash_pool[new_node->get_hash()] = new_node;
                do_execute(new_node); //update status and mark as committed/locked as well
            }
            else
            {
                new_node = new base::xvbnode_t(NULL,*new_block);
                new_node->set_obj_flag(base::enum_xvblock_flag_authenticated); //mark as authenticated node
                m_hash_pool[new_block->get_block_hash()] = new_node; //insert to pool first
            }
            
            //step#2: try to find any child nodes that link to this new node
            bool  is_found_linked_block = false;
            
            const uint64_t new_block_height = new_block->get_height();
            for(auto it = m_hash_pool.begin(); it != m_hash_pool.end(); ++it)
            {
                if(it->second->get_parent() == NULL)//test unlinked block
                {
                    if(it->second->get_height() == (new_block_height + 1)) //found child node
                    {
                        if(it->second->reset_parent(new_node))//link each other(note: reset_parent may verify hash as well)
                            is_found_linked_block = true;
                    }
                }
            }
            if(is_found_linked_block) //recheck every node to see whether has ready one
            {
                for(auto it = m_hash_pool.begin(); it != m_hash_pool.end(); ++it)
                {
                    do_execute(it->second); //try this cert-only block to update status
                }
            }
            //step#3: fire do_execute as above
            return true;
        }
                    
        //submit commited block to upper layer
        bool  xBFTcontext_t::do_submit(base::xvblock_t* highest_cert_block,base::xvblock_t* highest_proposal)
        {
            //step#1: find out all commited block and remove them from pool as well
            std::deque<base::xvbnode_t*> to_commit_blocks;
            for(auto it = m_hash_pool.begin(); it != m_hash_pool.end();)
            {
                auto old_it = it;
                ++it;
                
                base::xvbnode_t * _node_ptr = old_it->second;
                if(_node_ptr->get_block()->check_block_flag(base::enum_xvblock_flag_committed))
                {
                    _node_ptr->reset_obj_flags(); //clean flag to tell it is has been deleted from pool,and will completely delete later
                    to_commit_blocks.push_back(_node_ptr);//copy raw block ptr into queue
                    
                    m_view_pool.erase(_node_ptr->get_viewid());//remove view# from pool
                    m_hash_pool.erase(old_it);  //remove from pool
                }
            }
            //sort by lower viewid/height to higher viewid/height
            std::sort(to_commit_blocks.begin(), to_commit_blocks.end(), base::less_by_vbnode_viewid());
            
            //step#2: update layer of driver first
            fire_consensus_update_event_down(get_latest_commit_block(), get_latest_lock_block(), NULL);
            
            //step#3: notify commited block then
            if(to_commit_blocks.empty() == false)
            {
                for(auto it : to_commit_blocks)
                {
                    xkinfo("xBFTcontext_t::do_submit at node=0x%llx,submit for block=%s",get_xip2_addr().low_addr,it->dump().c_str());
                    
                }
                for(auto it : to_commit_blocks)
                {
                    it->close(false);    //close this node only
                    it->release_ref();   //release reference of pool holding
                }
            }
            return true;
        }
        
        bool xBFTcontext_t::do_clean(base::xvblock_t* highest_cert_block,base::xvblock_t* highest_proposal)
        {
            base::xvblock_t* latest_commit_block = get_latest_commit_block();
            base::xvblock_t* latest_lock_block   = get_latest_lock_block();
            
            //step#1:clean all nodes that old than commit/lock blocks
            std::deque<base::xvblock_t*> to_clean_blocks;
            for(auto it = m_hash_pool.begin(); it != m_hash_pool.end();)
            {
                auto old_it = it;
                ++it;
                
                base::xvbnode_t * _node_ptr = old_it->second;
                base::xvbnode_t * _parent_node_ptr = _node_ptr->get_parent();
                if(_parent_node_ptr != NULL) //clean parent node
                {
                    if(_parent_node_ptr->get_obj_flags() == 0) //parent node has been deleted from pool
                        _node_ptr->reset_parent(NULL); //disconnect from deleted block
                    else if(_parent_node_ptr->get_block()->check_block_flag(base::enum_xvblock_flag_committed))
                        _node_ptr->reset_parent(NULL); //disconnect from committed block
                }
                
                if(   (NULL == latest_commit_block)
                   || (_node_ptr->get_height() <= latest_commit_block->get_height())//any block <= committed block
                   || (_node_ptr->get_viewid() <= latest_commit_block->get_viewid())//any block <= committed block
                   || (_node_ptr->get_height() <  latest_lock_block->get_height())  //any block <  lock block
                   || ( (_node_ptr->get_height() == latest_lock_block->get_height()) && (_node_ptr->get_viewid() != latest_lock_block->get_viewid()) )//clean all same height but different viewid than locked block
                   )
                {
                    _node_ptr->reset_obj_flags(); //clean flag to tell it is has been deleted from pool
                    
                    _node_ptr->get_block()->add_ref();
                    to_clean_blocks.push_back(_node_ptr->get_block());//copy raw block into clean queue
                    
                    m_view_pool.erase(_node_ptr->get_viewid()); //remove view# from pool
                    _node_ptr->close(false);    //close this node only
                    _node_ptr->release_ref();   //release reference of pool holding
                    m_hash_pool.erase(old_it);  //remove from pool
                }
            }

            //sort by lower viewid/height to higher viewid/height
            std::sort(to_clean_blocks.begin(), to_clean_blocks.end(), base::less_by_block_viewid());
            
            //step#2: notify canceled block
            for(auto it : to_clean_blocks)
            {
                xinfo("xBFTcontext_t::do_clean,cancel the block:%s at node=0x%llx",it->dump().c_str(),get_xip2_addr().low_addr);
                
                it->release_ref(); //releasae reference holding by queue
            }
            return true;
        }
        
        //call from higher layer to lower layer(child)
        bool  xBFTcontext_t::on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xproposal_start * _evt_obj = (xproposal_start*)&event;
            if(_evt_obj->get_proposal() != NULL)
            {
                xdbg("xBFTcontext_t::on_proposal_start at node=0x%llx of leader",get_xip2_addr().low_addr);
            }
            else
            {
                xdbg("xBFTcontext_t::on_proposal_start at node=0x%llx of replica",get_xip2_addr().low_addr);
            }
            
            if(_evt_obj->get_latest_cert() != NULL)
            {
                if(m_view_pool.find(_evt_obj->get_latest_cert()->get_viewid()) == m_view_pool.end())//not found duplicate
                   do_update(_evt_obj->get_latest_cert()); //sync as blockstore
            }
            if(_evt_obj->get_latest_lock() != NULL)
            {
                if(m_view_pool.find(_evt_obj->get_latest_lock()->get_viewid()) == m_view_pool.end())//not found duplicate
                    do_update(_evt_obj->get_latest_lock()); //sync with blockstore
            }
            
            bool any_lock_commit_change = false;
            if(safe_check_for_block(_evt_obj->get_latest_lock()))
                any_lock_commit_change = update_lock_block(_evt_obj->get_latest_lock());
            if(safe_check_for_block(_evt_obj->get_latest_commit()))
            {
                if(update_commit_block(_evt_obj->get_latest_commit()))
                    any_lock_commit_change = true;
            }
            if(any_lock_commit_change)
                do_clean(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
            
            if(_evt_obj->get_latest_commit() == NULL) //follow blockstore
                _evt_obj->set_latest_commit(get_latest_commit_block());//update with latest commit block
            if(_evt_obj->get_latest_lock() == NULL) //follow blockstore
                _evt_obj->set_latest_lock(get_latest_lock_block()); //update with latest lock block
            return false; //let driver continue handle it
        }
        
        //0: a QC(Quorum Certification) = prove the 'block' is verified by majority but not knew yet by majority nodes(except leader).
        //However it also prove : parent-QC(justify_qc) is knew by majority nodes.
        //1. leader:  call back from driver when collect enough vote from relica
        //2. replica: call back from driver when leader'QC arrive
        //3. driver guanrentee the deilviered block has full data with QC(signature) and block data
        
        //call from lower layer to higher layer(parent)
        bool  xBFTcontext_t::on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xdbg("xBFTcontext_t::on_proposal_finish---> at node=0x%llx",get_xip2_addr().low_addr);
            
            xproposal_finish * _evt_obj = (xproposal_finish*)&event;
            base::xvblock_t * new_cert_block = _evt_obj->get_target_proposal();
            if(enum_xconsensus_code_successful == _evt_obj->get_error_code()) //driver have proven that it is a valid QC
            {
                if(safe_check_for_block(new_cert_block))  //sanity check again
                {
                    if(do_update(new_cert_block)) //update into chain successful
                    {
                        xdbg_info("xBFTcontext_t::on_proposal_finish,do_update for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                        
                        do_submit(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
                        do_clean(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
                    }
                    else //new_cert_block might be duplicated or behind commit block as well
                    {
                        xwarn("xBFTcontext_t::on_proposal_finish,drop proposal as false as do_update for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                    }
                }
                else
                {
                    xwarn("xBFTcontext_t::on_proposal_finish,fail-safe_check_for_block for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                }
            }
    
            //go default handle ,bring latest information of commit and lock
            _evt_obj->set_latest_commit(get_latest_commit_block());//update with latest commit block
            _evt_obj->set_latest_lock(get_latest_lock_block()); //update with latest lock block
            if(enum_xconsensus_code_successful != _evt_obj->get_error_code())
            {
                if(new_cert_block != NULL)
                    xwarn("xBFTcontext_t::on_proposal_finish,fail-at node=0x%llx with error_code=%d,for proposal=%s",get_xip2_addr().low_addr,_evt_obj->get_error_code(),new_cert_block->dump().c_str());
                else
                    xwarn("xBFTcontext_t::on_proposal_finish,fail-at node=0x%llx with error_code=%d for empty proposal,at node=0x%llx",get_xip2_addr().low_addr,_evt_obj->get_error_code());
            }
            return false; //let upper layer to continue handle it
        }
        
        //call from lower layer to higher layer(parent) ,or parent->childs
        bool  xBFTcontext_t::on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus_update * _evt_obj = (xconsensus_update*)&event;
            if(event.get_route_path() == base::enum_xevent_route_path_down)
            {
                xdbg("xBFTcontext_t::on_consensus_update_down at node=0x%llx",get_xip2_addr().low_addr);
                
                if(_evt_obj->get_latest_cert() != NULL)
                {
                    if(m_view_pool.find(_evt_obj->get_latest_cert()->get_viewid()) == m_view_pool.end())//not found duplicate
                        do_update(_evt_obj->get_latest_cert()); //sync as blockstore
                }
                if(_evt_obj->get_latest_lock() != NULL)
                {
                    if(m_view_pool.find(_evt_obj->get_latest_lock()->get_viewid()) == m_view_pool.end())//not found duplicate
                        do_update(_evt_obj->get_latest_lock()); //sync with blockstore
                }
                
                bool any_lock_commit_change = false;
                if(safe_check_for_block(_evt_obj->get_latest_lock()))
                    any_lock_commit_change = update_lock_block(_evt_obj->get_latest_lock());
                if(safe_check_for_block(_evt_obj->get_latest_commit()))
                {
                    if(update_commit_block(_evt_obj->get_latest_commit()))
                        any_lock_commit_change = true;
                }
                if(any_lock_commit_change)
                    do_clean(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
            }
            else
            {
                xdbg("xBFTcontext_t::on_consensus_update_up at node=0x%llx",get_xip2_addr().low_addr);
            }
            _evt_obj->set_latest_commit(get_latest_commit_block());//update with latest commit block
            _evt_obj->set_latest_lock(get_latest_lock_block());    //update with latest lock block
            
            return false; //let driver or pacemaker continue handle it
        }
        
        //call from lower layer to higher layer(parent)
        bool  xBFTcontext_t::on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xdbg("xBFTcontext_t::on_replicate_finish---> at node=0x%llx",get_xip2_addr().low_addr);
            
            xreplicate_finish * _evt_obj = (xreplicate_finish*)&event;
            base::xvblock_t * new_cert_block = _evt_obj->get_target_block();
            if(NULL == new_cert_block)
                return true;
            
            if(enum_xconsensus_code_successful == _evt_obj->get_error_code()) //driver have proven that it is a valid QC
            {
                if(safe_check_for_block(new_cert_block))  //sanity check again
                {
                    if(do_update(new_cert_block)) //update into chain successful
                    {
                        xdbg_info("xBFTcontext_t::on_replicate_finish,do_update for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                        
                        do_submit(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
                        do_clean(_evt_obj->get_latest_cert(),_evt_obj->get_latest_proposal());
                    }
                    else //new_cert_block might be duplicated or behind commit block as well
                    {
                        xwarn("xBFTcontext_t::on_replicate_finish,drop cert as false as do_update for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                    }
                }
                else
                {
                    xwarn("xBFTcontext_t::on_replicate_finish,fail-safe_check_for_block for block:%s at node=0x%llx",new_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                }
                
                //go default handle ,bring latest information of commit and lock
                _evt_obj->set_latest_commit(get_latest_commit_block());//update with latest commit block
                _evt_obj->set_latest_lock(get_latest_lock_block()); //update with latest lock block
                return false; //return false to let event continuely throw up
            }
            return true; //stop for any error
        }
  
        //////////////////////////////////////////xthcontext_t////////////////////////////////////////////////
        
    };//end of namespace of xconsensus
    
};//end of namespace of top
