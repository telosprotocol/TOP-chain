// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>
#include "xconsdriver.h"
#include "xmetrics/xmetrics.h"

#define __FAST_CLEAN_PROPOSAL__

namespace top
{
    namespace xconsensus
    {
        //////////////////////////////////////////xBFTRules////////////////////////////////////////////////
        xBFTRules::xBFTRules(xcscoreobj_t & parent_object)
            :xcsdriver_t(parent_object)
        {
            m_latest_commit_block = NULL;
            m_latest_lock_block   = NULL;
            m_latest_voted_height = 0;
            m_latest_voted_viewid = 0;
            m_latest_viewid = 0;
            m_latest_clock = 0;
            xinfo("xBFTRules::xBFTRules,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
        
        xBFTRules::~xBFTRules()
        {
            clean_proposals();
            clean_cert_blocks();
            
            if(m_latest_lock_block != NULL)
                m_latest_lock_block->release_ref();
            
            if(m_latest_commit_block != NULL)
                m_latest_commit_block->release_ref();
            
            xinfo("xBFTRules::~xBFTRules,destroy,this=%p",this);
        }
        
        std::string  xBFTRules::dump() const  //dump driver ' main information
        {
            char local_param_buf[256];
            base::xvblock_t* _latest_cert = get_latest_cert_block();
            if(NULL == m_latest_lock_block)
            {
                xprintf(local_param_buf,sizeof(local_param_buf),"{nil locked-block,hviewid=%" PRIu64 "}",get_latest_voted_viewid());
            }
            else if(NULL == _latest_cert)
            {
                xprintf(local_param_buf,sizeof(local_param_buf),"{locked={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 "} <--nil hqc <-- hviewid=%" PRIu64 "}",m_latest_lock_block->get_height(),m_latest_lock_block->get_viewid(),m_latest_lock_block->get_viewtoken(),m_latest_lock_block->get_clock(),get_latest_voted_viewid());
            }
            else
            {
                const base::xvblock_t * commit_block = m_latest_commit_block;
                if(commit_block != NULL)
                    xprintf(local_param_buf,sizeof(local_param_buf),"{commited={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- locked={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hqc={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hviewid=%" PRIu64 "}",commit_block->get_height(),commit_block->get_viewid(),commit_block->get_viewtoken(),m_latest_lock_block->get_height(),m_latest_lock_block->get_viewid(),m_latest_lock_block->get_viewtoken(),_latest_cert->get_height(),_latest_cert->get_viewid(),_latest_cert->get_viewtoken(),get_latest_voted_viewid());
                else
                    xprintf(local_param_buf,sizeof(local_param_buf),"{commited={nil} <-- locked={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hqc={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hviewid=%" PRIu64 "}",m_latest_lock_block->get_height(),m_latest_lock_block->get_viewid(),m_latest_lock_block->get_viewtoken(),_latest_cert->get_height(),_latest_cert->get_viewid(),_latest_cert->get_viewtoken(),get_latest_voted_viewid());
            }
            return std::string(local_param_buf);
        }
        
        //////////////////////////////block managed fro proposal//////////////////////////////
        base::xvblock_t *  xBFTRules::get_commit_block()
        {
            if(m_latest_commit_block != nullptr)
                return m_latest_commit_block;
            
            base::xvblock_t * locked_block = get_lock_block();
            if(locked_block != NULL)
            {
                if(locked_block->get_height() > 0)//
                {
                    base::xauto_ptr<base::xvblock_t> commit_block(get_vblockstore()->load_block_object(*this, locked_block->get_height() - 1,locked_block->get_last_block_hash(),false, metrics::blockstore_access_from_bft_get_commit_blk));
                    if(commit_block)
                    {
                        m_latest_commit_block = commit_block();
                        m_latest_commit_block->add_ref();
                    }
                }
            }
            
            if(NULL != m_latest_commit_block )
                return m_latest_commit_block;
            else
                return locked_block; //use lock as commit when it is not avaiable
        }
    
        bool        xBFTRules::set_commit_block(base::xvblock_t * new_commit_block)//update block of  commited one
        {
            if(NULL == new_commit_block)
                return false;
            
            if(new_commit_block == m_latest_commit_block)
                return true; //same object
            
            if(m_latest_commit_block != NULL)
            {
                if(   (new_commit_block->get_height() <= m_latest_commit_block->get_height())
                   || (new_commit_block->get_viewid() <= m_latest_commit_block->get_viewid()) )
                {
                    return false;
                }
            }
            
            base::xvblock_t * locked_block = get_lock_block();
            if(locked_block != NULL)
            {
                if(locked_block->get_height() != 0)
                {
                    if(  (locked_block->get_height() != new_commit_block->get_height() + 1)
                       ||(locked_block->get_last_block_hash() != new_commit_block->get_block_hash()) )
                    {
                        xwarn("xBFTRules::set_commit_block, new-commit=%s agains lock=%s,at node=0x%llx,this=%llx",new_commit_block->dump().c_str(),locked_block->dump().c_str(),get_xip2_addr().low_addr,(int64_t)this);
                        return false;
                    }
                }
                
                if(m_latest_commit_block != NULL)
                {
                    xkinfo("xBFTRules::set_commit_block, old-commit=%s -> new-commit=%s,at node=0x%llx,this=%llx",m_latest_commit_block->dump().c_str(),new_commit_block->dump().c_str(),get_xip2_addr().low_addr,(int64_t)this);
                    m_latest_commit_block->release_ref();
                }
                else
                {
                    xkinfo("xBFTRules::set_commit_block, nil-commit -> new-commit=%s,at node=0x%llx,this=%llx",new_commit_block->dump().c_str(),get_xip2_addr().low_addr,(int64_t)this);
                }
                
                m_latest_commit_block = new_commit_block;
                m_latest_commit_block->add_ref();
                return true;
            }
            //locked block must be valid first
            return false;
        }
    
        base::xvblock_t * xBFTRules::get_lock_block()
        {
            if(m_latest_lock_block != nullptr)
                return m_latest_lock_block;
            
            if(get_vblockstore() != nullptr)
            {
                base::xauto_ptr<base::xvblock_t>  latest_lock_block = get_vblockstore()->get_latest_locked_block(*this, metrics::blockstore_access_from_bft_get_lock_blk);
                if(latest_lock_block != nullptr)
                    set_lock_block(latest_lock_block.get());
            }
            return m_latest_lock_block;
        }
         
        bool xBFTRules::set_lock_block(base::xvblock_t* latest_lock_block)
        {
            if(m_latest_lock_block != NULL)
            {
                if(m_latest_lock_block == latest_lock_block) //same ptr
                    return true;
                
                if(   (latest_lock_block != NULL)
                   && (m_latest_lock_block->get_height() == latest_lock_block->get_height())
                   && (m_latest_lock_block->get_viewid() == latest_lock_block->get_viewid())
                   && (latest_lock_block->check_block_flag(base::enum_xvblock_flag_locked))  )
                {
                    //same height & view but diffierent ptr,using new object ptr to replace old ptr
                    latest_lock_block->add_ref();
                    base::xvblock_t* old_ptr = base::xatomic_t::xexchange(m_latest_lock_block, latest_lock_block);
                    if(old_ptr != NULL)
                        old_ptr->release_ref();
                    return true;
                }
            }
            
            if(safe_check_for_lock_block(latest_lock_block) == false)
                return false;

            //update local lock information
            if(m_latest_lock_block != latest_lock_block) //check equal or not before update
            {
                latest_lock_block->add_ref();
                if(m_latest_lock_block != NULL)
                {
                    xkinfo("xBFTRules::set_lock_block, old-lock=%s -> new-lock=%s,at node=0x%llx,this=%llx",m_latest_lock_block->dump().c_str(),latest_lock_block->dump().c_str(),get_xip2_addr().low_addr,(int64_t)this);
                    
                    m_latest_lock_block->release_ref();
                    m_latest_lock_block = NULL;
                }
                else
                {
                    xkinfo("xBFTRules::set_lock_block, nil-block ->new=%s,at node=0x%llx,this=%llx",latest_lock_block->dump().c_str(), get_xip2_addr().low_addr,(int64_t)this);
                }
                m_latest_lock_block = latest_lock_block;
                if(m_latest_commit_block != NULL) //reset commit to keep coordinated with lock
                {
                    m_latest_commit_block->release_ref();
                    m_latest_commit_block = NULL;
                }
                
                //m_latest_voted_height and m_latest_voted_viewid always increase
                m_latest_voted_height = std::max(m_latest_voted_height,latest_lock_block->get_height());
                m_latest_voted_viewid = std::max(m_latest_voted_viewid,latest_lock_block->get_viewid());
                
                //clean cert that not follow lock branch(decided by blockstore now)
                for(auto it = m_certified_blocks.begin(); it != m_certified_blocks.end();)
                {
                    auto cur_it = it;
                    ++it;
                    
                    if(   (cur_it->second->get_height()  <= m_latest_lock_block->get_height())
                       || (cur_it->second->get_viewid()  <= m_latest_lock_block->get_viewid())
                       || (cur_it->second->get_chainid() != m_latest_lock_block->get_chainid())
                       || (cur_it->second->get_account() != m_latest_lock_block->get_account())
                       ) //not follow locked block
                    {
                        base::xvblock_t* _to_remove = cur_it->second;
                        m_certified_blocks.erase(cur_it);
                        
                        xinfo("xBFTRules::set_lock_block,remove un-satified cert block(%s)",_to_remove->dump().c_str());
                        _to_remove->release_ref();
                    }
                    else if(cur_it->second->get_height() == (m_latest_lock_block->get_height() + 1) ) //found next height
                    {
                        if(cur_it->second->get_last_block_hash() != m_latest_lock_block->get_block_hash())//not connect to lock
                        {
                            base::xvblock_t* _to_remove = cur_it->second;
                            m_certified_blocks.erase(cur_it);
                            
                            xinfo("xBFTRules::set_lock_block,remove un-connected cert block(%s)",_to_remove->dump().c_str());
                            _to_remove->release_ref();
                        }
                    }
                }
                
                //filter too old proposal and put into removed_list
                std::vector<xproposal_t*> removed_list;
                for(auto it = m_proposal_blocks.begin(); it != m_proposal_blocks.end();)
                {
                    auto cur_it = it;//copy first
                    ++it; //navigate to next
                    
                    if(   (cur_it->second->get_height()  <= m_latest_lock_block->get_height())
                       || (cur_it->second->get_viewid()  <= m_latest_lock_block->get_viewid())
                       || (cur_it->second->get_chainid() != m_latest_lock_block->get_chainid())
                       || (cur_it->second->get_account() != m_latest_lock_block->get_account())
                       ) //not follow locked block
                    {
                       xproposal_t * _to_remove = cur_it->second;
                       removed_list.push_back(_to_remove);//transfer owner to list
                       m_proposal_blocks.erase(cur_it);//erase old one
                        
                       xinfo("xBFTRules::set_lock_block,remove un-satified cert block(%s)",_to_remove->dump().c_str());
                    }
                    else if(cur_it->second->get_height() == (m_latest_lock_block->get_height() + 1) ) //found next height
                    {
                        if(cur_it->second->get_last_block_hash() != m_latest_lock_block->get_block_hash())//not connect to lock
                        {
                            xproposal_t * _to_remove = cur_it->second;
                            removed_list.push_back(_to_remove);//transfer owner to list
                            m_proposal_blocks.erase(cur_it);//erase old one
                            
                            xinfo("xBFTRules::set_lock_block,remove un-satified cert block(%s)",_to_remove->dump().c_str());
                        }
                    }
                }
                
                //notify each one to upper layer as enum_xconsensus_error_outofdate
                if(removed_list.empty() == false)
                {
                    std::sort(removed_list.begin(), removed_list.end(), sort_proposal());
                    for(auto it = removed_list.begin(); it != removed_list.end(); ++it)
                    {
                        xproposal_t * _to_remove = *it;
                        const std::string errdetail;
                        fire_proposal_finish_event(enum_xconsensus_error_cancel,errdetail,_to_remove->get_block(), NULL, NULL, NULL, get_latest_proposal_block());
                        
                        _to_remove->release_ref();
                    }
                    removed_list.clear();
                }
                return true;
            }
            return false;
        }
        
        //safe rule to lock block
        bool xBFTRules::safe_check_for_lock_block(base::xvblock_t * _lock_block)
        {
            if(NULL == _lock_block)
                return false;
            
            if(m_latest_lock_block != NULL)
            {
                //sanity test first, check view#id of proposal,that must over > last voted one
                if(  (_lock_block->get_height()  <=  m_latest_lock_block->get_height())
                   ||(_lock_block->get_viewid()  <=  m_latest_lock_block->get_viewid()) )
                {
                    xwarn("xBFTRules::safe_check_for_lock_block,lower _lock_block(%s) < driver=%s,at node=0x%llx",_lock_block->dump().c_str(),dump().c_str(),get_xip2_low_addr());
                    return false;
                }
            }
            
            if(_lock_block->is_deliver(false) == false)
            {
                xerror("xBFTRules::safe_check_for_lock_block,not a delivered block=%s at node=0x%llx",_lock_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            return true;
        }

        xproposal_t*  xBFTRules::add_proposal(base::xvblock_t * proposal,base::xvblock_t * parent_block,const uint32_t expired_ms,base::xvqcert_t * clock_cert)
        {
            if(NULL == proposal)
                return NULL;
            
            xproposal_t* new_block_ptr = new xproposal_t(*proposal,parent_block);
            new_block_ptr->set_bind_clock_cert(clock_cert);
            new_block_ptr->set_expired_ms(get_time_now() + expired_ms);
            if(add_proposal(*new_block_ptr))
                return new_block_ptr;
            
            new_block_ptr->release_ref();
            return NULL;
        }
        
        //consistency gurantee: only have one valid proposal or cert at specified view and height,aka : view-lock and height-lock
        //liveness gurantee :  any cert or proposal of height, might be expired until lock/proposal move to next height
        //note:add_proposal need ensure it is finished at transaction(either by lock or single-thread)
        bool  xBFTRules::add_proposal(xproposal_t & proposal_block)
        {
            if(    (proposal_block.get_height() <= get_lock_block()->get_height())
               ||  (proposal_block.get_viewid() <= get_lock_block()->get_viewid()) )
            {
                xinfo("xBFTRules::add_proposal,fail-proposal(%s) vs local(%s),at node=0x%llx",proposal_block.dump().c_str(),dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //rule#1: not conflict with existing cert block
            for(auto it = m_certified_blocks.rbegin(); it != m_certified_blocks.rend(); ++it)
            {
                if(  (proposal_block.get_height() <  it->second->get_height())
                   ||(proposal_block.get_viewid() <= it->second->get_viewid()) )//consistency protect
                {
                    xinfo("xBFTRules::add_proposal,fail-lc proposal(%s) vs cert(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
                else if(proposal_block.get_height() == it->second->get_height())
                {
                    xinfo("xBFTRules::add_proposal,fail-ec proposal(%s) vs non-expired cert(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                    return false; //have un-expired cert at same height
                }
                else if(proposal_block.get_height() == (it->second->get_height() + 1) ) //next height'proposal
                {
                    if(proposal_block.get_last_block_cert()->get_viewid() < it->second->get_viewid())
                    {
                        xinfo("xBFTRules::add_proposal,fail-hc proposal(%s) vs prev cert(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                        return false; //have un-expired cert at same height
                    }
                }
            }
                        
            //rule#2: not conflict with existing proposal
            for(auto it = m_proposal_blocks.rbegin(); it != m_proposal_blocks.rend(); ++it)
            {
                if(  (proposal_block.get_height() <  it->second->get_height())
                   ||(proposal_block.get_viewid() <= it->second->get_viewid()) )//consistency protect
                {
                    xinfo("xBFTRules::add_proposal,fail-lp proposal(%s) vs exsit proposal(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
                else if(proposal_block.get_height() == it->second->get_height()) //liveness entry
                {
                    if(proposal_block.get_bind_clock_cert()->get_clock() < (it->second->get_bind_clock_cert()->get_clock() + 3) )
                    {
                        xinfo("xBFTRules::add_proposal,fail-ep proposal(%s) vs non-expired proposal(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                        return false; //have un-expired proposal at same height
                    }
                    //lock max cert of prev height
                    if(proposal_block.get_last_block_cert()->get_viewid() < it->second->get_last_block_cert()->get_viewid())
                    {
                        xinfo("xBFTRules::add_proposal,fail-epv proposal(%s) vs exist proposal(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                        return false;
                    }
                }
                else if(proposal_block.get_height() == (it->second->get_height() + 1) )//next height
                {
                    //go quick path if proposal point to latest proposal at prev height,otherwise still need wait timeout
                    if(proposal_block.get_last_block_cert()->get_viewid() != it->second->get_viewid())
                    {
                        if(proposal_block.get_bind_clock_cert()->get_clock() < (it->second->get_bind_clock_cert()->get_clock() + 3) )
                        {
                            xinfo("xBFTRules::add_proposal,fail-hp proposal(%s) vs non-expired proposal(%s),at node=0x%llx",proposal_block.dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                            return false; //have un-expired proposal at same height
                        }
                    }
                }
            }

            //rule#3: need follow branch of commit
            if(safe_check_follow_commit_branch(proposal_block.get_block()) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::add_proposal,fail-as safe_check_follow_commit_branch for block(%s) vs local(%s),at node=0x%llx",proposal_block.dump().c_str(),dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //rule#4: need follow branch of lock
            if(safe_check_follow_locked_branch(proposal_block.get_block()) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::add_proposal,fail-as safe_check_follow_locked_branch for block(%s) vs local(%s),at node=0x%llx",proposal_block.dump().c_str(),dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //rule5: clean any proposal_block
            std::vector<xproposal_t*> removed_list;
            for(auto it = m_proposal_blocks.begin(); it != m_proposal_blocks.end();)
            {
                auto cur_it = it;//copy first
                ++it; //navigate next
                
                //keep lower proposal since the related commist-msg may arrive later
                if(cur_it->second->get_height() == proposal_block.get_height())
                {
                    xproposal_t * _to_remove = cur_it->second;
                    _to_remove->disable_vote();
                    _to_remove->mark_expired();
                    removed_list.push_back(_to_remove);//transfer owner to list
                    m_proposal_blocks.erase(cur_it);
                }
                else //proposal_block is latest one
                {
                    cur_it->second->disable_vote(); //not allow vote anymore
                    cur_it->second->mark_pending(); //still keep and waiting for new messaging
                }
            }
            
            //add new proposal
            m_proposal_blocks[proposal_block.get_viewid()] = &proposal_block;
            proposal_block.add_ref();
            
            //rule6: clean any lower viewid of height than proposal_block
            for(auto it = m_certified_blocks.begin(); it != m_certified_blocks.end();)
            {
                auto cur_it = it;//copy first
                ++it; //navigate next
                
                if( cur_it->second->get_height() == proposal_block.get_height())
                {
                    if(cur_it->second->get_viewid() < proposal_block.get_viewid())
                    {
                        xinfo("xBFTRules::add_proposal,clean cert(%s) by new proposal(%s),at node=0x%llx",cur_it->second->dump().c_str(),proposal_block.dump().c_str(),get_xip2_addr().low_addr);
                        
                        cur_it->second->release_ref();
                        m_certified_blocks.erase(cur_it);
                    }
                }
            }
            
            //notify each one to upper layer
            if(removed_list.empty() == false)
            {
                std::sort(removed_list.begin(), removed_list.end(), sort_proposal());
                for(auto it = removed_list.begin(); it != removed_list.end(); ++it)
                {
                    xproposal_t * _to_remove = *it;
                    
                    xdbg_info("xBFTRules::add_proposal,clean existing proposal(%s) by new proposal(%s),at node=0x%llx",_to_remove->dump().c_str(),proposal_block.dump().c_str(),get_xip2_addr().low_addr);
                    
                    {
                        const std::string errdetail;
                        fire_proposal_finish_event(enum_xconsensus_error_cancel,errdetail,_to_remove->get_block(), NULL, NULL, NULL, get_latest_proposal_block());
                    }

                    _to_remove->release_ref();
                }
                removed_list.clear();
            }
            
            xinfo("xBFTRules::add_proposal,successful for new proposal(%s),at node=0x%llx",proposal_block.dump().c_str(),get_xip2_addr().low_addr);
            return true;
        }
    
        void  xBFTRules::update_voted_metric(base::xvblock_t * _block)
        {
            if(_block != NULL)
            {
                //m_latest_voted_height and m_latest_voted_viewid always increase
                m_latest_voted_height = std::max(m_latest_voted_height,_block->get_height());
                m_latest_voted_viewid = std::max(m_latest_voted_viewid,_block->get_viewid());
            }
        }
         
        bool  xBFTRules::clean_proposals()
        {
            return clean_blocks(m_proposal_blocks);
        }
    
        xproposal_t* xBFTRules::get_latest_proposal() const
        {
            return get_latest_block(m_proposal_blocks);
        }
    
        base::xvblock_t*  xBFTRules::get_latest_proposal_block() const
        {
            xproposal_t* _propoal_wrap = get_latest_proposal();
            if(NULL != _propoal_wrap)
                return _propoal_wrap->get_block();
            
            return NULL;
        }
        xproposal_t*  xBFTRules::find_proposal(const uint64_t view_id) const
        {
            return find_block(view_id,m_proposal_blocks);
        }
        
        //////////////////////////////block managed for synchoronization//////////////////////////////
        
        //////////////////////////////block manage for certified block//////////////////////////////
        bool  xBFTRules::on_cert_verified(base::xvqcert_t * new_cert)
        {
            if(new_cert != NULL)
            {
                for(auto it = m_proposal_blocks.begin(); it != m_proposal_blocks.end();++it)
                {
                    if(it->second->get_last_block_cert()->get_viewid() < new_cert->get_viewid())
                    {
                        if(it->second->is_vote_disable() == false)
                        {
                            it->second->disable_vote();//outdated proposal < new cert
                            xinfo("xBFTRules::on_cert_verified,outdated proposal(%s) vs cert(%s),at node=0x%llx",it->second->dump().c_str(),new_cert->dump().c_str(),get_xip2_addr().low_addr);
                        }
                    }
                }
                return fire_certificate_finish_event(new_cert);
            }
            return false;
        }
    
        bool  xBFTRules::add_cert_block(base::xvblock_t* _target_block,bool & found_matched_proposal)
        {
            found_matched_proposal = false;
            if(nullptr == _target_block)
                return false;
            
            //rule#1: need follow branch of commit
            if(safe_check_follow_commit_branch(_target_block) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::add_cert_block,fail-as safe_check_follow_commit_branch for block(%s) vs local(%s),at node=0x%llx",_target_block->dump().c_str(),dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //rule#2: need follow branch of lock
            if(safe_check_follow_locked_branch(_target_block) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::add_cert_block,fail-as safe_check_follow_locked_branch for block(%s) vs local(%s),at node=0x%llx",_target_block->dump().c_str(),dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            //rule#3: mutex with existing proposal
            {
                //the voted proposal may locked current height and prev height as well
                for(auto it = m_proposal_blocks.rbegin(); it != m_proposal_blocks.rend(); ++it)
                {
                    if(  (it->second->is_pending() == false) //not pending == still at fade stage
                       ||(it->second->is_vote_disable() == false) )//still working proposal
                    {
                        if(  (it->second->get_height() == (_target_block->get_height() + 1) ) //a block of prev height
                           &&(it->second->get_last_block_hash() != _target_block->get_block_hash())  )//but a unpointed cer
                        {
                            //consistency gurantee
                            xinfo("xBFTRules::add_cert_block,fail-prev_cert(%s) vs exist proposal(%s),at node=0x%llx",_target_block->dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                            return false;
                        }
                        
                        if(  (it->second->get_height() == (_target_block->get_height() + 2) ) //a block of prev_prev height
                           &&(it->second->get_justify_cert_hash() != _target_block->get_input_root_hash())  )//unpaired
                        {
                            //consistency gurantee
                            xwarn("xBFTRules::add_cert_block,fail-prev_prev cert(%s) vs exist proposal(%s),at node=0x%llx",_target_block->dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                            return false;
                        }
                        
                        if(_target_block->get_height() == it->second->get_height()) //found same heights
                        {
                            if(_target_block->get_viewid() < it->second->get_viewid())//only keep higher view of proposal and cert
                            {
                                //consistency gurantee
                                xinfo("xBFTRules::add_cert_block,fail-cert(%s) vs exist proposal(%s),at node=0x%llx",_target_block->dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                                return false; //only allow have one proposal or cert at same height,keep higher viewid
                            }
                        }
                    }
                }
            }
         
            #ifdef __JUST_ALLOW_HIGHEST_VIEW_OF_CERT__
            //rule#4: only allow one valid cert with higher viewid at same height
            for(auto it = m_certified_blocks.begin(); it != m_certified_blocks.end(); ++it)
            {
                if(_target_block->get_height() == it->second->get_height())
                {
                    if(_target_block->get_viewid() <= it->second->get_viewid())//just keep highest view
                    {
                        //consistency gurantee
                        if(_target_block->get_viewid() != it->second->get_viewid())
                            xinfo("xBFTRules::add_cert_block,a outdated block(%s) vs cert(%s),at node=0x%llx",_target_block->dump().c_str(),it->second->dump().c_str(),get_xip2_addr().low_addr);
                        return false;
                    }
                }
            }
            
            //rule5: clean any lower viewid of height than _target_block
            for(auto it = m_certified_blocks.begin(); it != m_certified_blocks.end();)
            {
                auto cur_it = it;//copy first
                ++it; //navigate next
                
                if( cur_it->second->get_height() == _target_block->get_height())
                {
                    if(cur_it->second->get_viewid() < _target_block->get_viewid())
                    {
                        cur_it->second->release_ref();
                        m_certified_blocks.erase(cur_it);
                    }
                }
            }
            #endif //end of __JUST_ALLOW_HIGHEST_VIEW_OF_CERT__
            
            //then insert it
            auto insert_result = m_certified_blocks.emplace(_target_block->get_viewid(),_target_block);
            if(insert_result.second)//note:not allow overwrited existing cert with same view#
                _target_block->add_ref();//hold referene for map
            
            //rule#6: clean any proposal with lower viewid/height by comparing cert block
            std::vector<xproposal_t*> removed_list;
            for(auto it = m_proposal_blocks.begin(); it != m_proposal_blocks.end();)
            {
                auto cur_it = it;//copy first
                ++it; //navigate next
                    
                if(  (cur_it->second->get_height() == _target_block->get_height())
                   &&(cur_it->second->get_viewid() == _target_block->get_viewid())
                   &&(cur_it->second->get_viewtoken() == _target_block->get_viewtoken()) )
                {
                    cur_it->second->mark_certed(); //found matched cert with proposasl
                    found_matched_proposal = true;
                }
                
                //consistency gurantee
                if(cur_it->second->get_height() == _target_block->get_height())
                {
                    if(cur_it->second->get_viewid() <= _target_block->get_viewid())
                    {
                        xproposal_t * _to_remove = cur_it->second;
                        _to_remove->disable_vote();
                        removed_list.push_back(_to_remove);//transfer owner to list
                        
                        m_proposal_blocks.erase(cur_it);//erase old one
                    }
                }
                else if(cur_it->second->get_height() < _target_block->get_height())
                {
                    cur_it->second->disable_vote(); //not allow vote anymore
                    cur_it->second->mark_pending(); //still keep and waiting for new messaging
                }
            }
            //notify each one to upper layer as enum_xconsensus_error_outofdate
            if(removed_list.empty() == false)
            {
                std::sort(removed_list.begin(), removed_list.end(), sort_proposal());
                for(auto it = removed_list.begin(); it != removed_list.end(); ++it)
                {
                    xproposal_t * _to_remove = *it;
                    if(false == _to_remove->is_certed())
                    {
                        xinfo("xBFTRules::add_cert_block,clean existing proposal(%s) by new cert(%s),at node=0x%llx",_to_remove->dump().c_str(),_target_block->dump().c_str(),get_xip2_addr().low_addr);
                        
                        const std::string errdetail;
                        fire_proposal_finish_event(enum_xconsensus_error_cancel,errdetail,_to_remove->get_block(), NULL, NULL, NULL, get_latest_proposal_block());
                    }
                    else //notify U.S the proposal has been successful
                    {
                        xinfo("xBFTRules::add_cert_block,proposal certified now cert(%s),at node=0x%llx",_target_block->dump().c_str(),get_xip2_addr().low_addr);
                        
                        if(  (_target_block->get_input()->get_resources_hash().empty() == false) //link resoure data
                           &&(_target_block->get_input()->has_resource_data() == false) ) //but dont have resource _target_block now
                        {
                            //_local_block need reload input resource
                            get_vblockstore()->load_block_input(*this, _target_block);
                            xassert(_target_block->get_input()->has_resource_data());
                        }
                        
                        if(  (_target_block->get_output()->get_resources_hash().empty() == false) //link resoure data
                           &&(_target_block->get_output()->has_resource_data() == false) ) //but dont have resource avaiable now
                        {
                            //_local_block need reload output resource
                            get_vblockstore()->load_block_output(*this, _target_block);
                            xassert(_target_block->get_output()->has_resource_data());
                        }

                        get_vblockstore()->load_block_output_offdata(*this, _target_block);
       
                        fire_proposal_finish_event(_target_block, NULL, NULL, NULL, NULL);//call on_consensus_finish(block) to driver context layer
                    }
                    _to_remove->release_ref();
                }
                removed_list.clear();
            }
            
            xinfo("xBFTRules::add_cert_block,added new cert block(%s),at node=0x%llx",_target_block->dump().c_str(),get_xip2_addr().low_addr);
            return true;//return true means it inserted into map at brandnew
        }
        bool  xBFTRules::remove_cert_block(const uint64_t view_id)
        {
            return  remove_block(view_id,m_certified_blocks);
        }
        bool  xBFTRules::clean_cert_blocks()
        {
            return clean_blocks(m_certified_blocks);
        }
        base::xvblock_t*   xBFTRules::get_latest_cert_block() const
        {
            return get_latest_block(m_certified_blocks);
        }
    
        base::xvblock_t*   xBFTRules::find_first_cert_block(const uint64_t block_height) const
        {
            for(auto it = m_certified_blocks.begin(); it != m_certified_blocks.end(); ++it)
            {
                if(block_height == it->second->get_height())
                {
                    return it->second;
                }
            }
            return NULL;
        }
    
        base::xvblock_t*   xBFTRules::find_cert_block(const uint64_t view_id) const//caller need care to release reference once no-longer need
        {
            for(auto it = m_certified_blocks.rbegin(); it != m_certified_blocks.rend(); ++it)
            {
                if(view_id == it->second->get_viewid()) //found parent qc cert
                {
                    return it->second;
                }
            }
      
            return NULL;
        }
    
        base::xvblock_t*   xBFTRules::find_cert_block(const uint64_t block_height,const std::string & block_hash)
        {
            for(auto it = m_certified_blocks.rbegin(); it != m_certified_blocks.rend(); ++it)
            {
                if(   (block_height == it->second->get_height()) //found parent qc cert
                   && (block_hash   == it->second->get_block_hash()) )
                {
                    return it->second;
                }
            }
        
            return NULL;
        }
    
        base::xauto_ptr<base::xvbindex_t> xBFTRules::load_block_index(const uint64_t block_height,const std::string & block_hash)
        {
            base::xvblock_t* cached_block = find_cert_block(block_height,block_hash);
            if(cached_block != NULL)
                return new base::xvbindex_t(*cached_block);
            
            return get_vblockstore()->load_block_index(*this, block_height,block_hash);
        }
        
        ////////////////////////////////////minimal safe rules///////////////////////////////////////////////
        bool xBFTRules::is_proposal_expire(xproposal_t * _proposal)
        {
            base::xvblock_t * commit_block = get_commit_block();
            base::xvblock_t * lock_block   = get_lock_block();
            if( (NULL == _proposal) || (NULL == commit_block) || (NULL == lock_block) )
                return false;
            
            //now using commit as lower bound to clean ones
            if(   (_proposal->get_height() <= commit_block->get_height())
               || (_proposal->get_viewid() <= commit_block->get_viewid()) )
            {
                return true;//expired
            }
            
            if(   (_proposal->get_height() <= lock_block->get_height())
               || (_proposal->get_viewid() <= lock_block->get_viewid()) )
            {
                return true;//expired
            }
            
            base::xvblock_t *  latest_cert_block = get_latest_cert_block();
            if(latest_cert_block != NULL)
            {
                if(  (_proposal->get_viewid() <= latest_cert_block->get_viewid())
                   ||(_proposal->get_height() <  latest_cert_block->get_height()) )
                {
                    return true;//expired
                }
            }
            
            if(get_latest_viewid() > _proposal->get_viewid())
                return true; //once viewid update, the proposal is logically expired
    
            //using clock as upper bound to clean ones, 3 * 10 = 30s
            if(get_lastest_clock() > (_proposal->get_block()->get_clock() + 2) )
                return true;
            
            return false;//still valid
        }
    
        bool xBFTRules::safe_check_for_block(base::xvblock_t * _block)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == _block) || (NULL == lock_block) )
                return false;
            
            if(   (_block->get_viewid()  < _block->get_height())
               || (_block->get_height()  <= lock_block->get_height())
               || (_block->get_viewid()  <= lock_block->get_viewid())
               || (_block->get_chainid() != lock_block->get_chainid())
               || (_block->get_account() != lock_block->get_account())
               )
            {
                return false;
            }
            return true;
        }
        
        bool xBFTRules::safe_check_for_packet(base::xcspdu_t & _test_packet)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if(NULL == lock_block)
                return false;
            
            //sanity test first, check view#id of proposal,that must over > last voted one
            if( (NULL == lock_block)
               || (_test_packet.get_block_viewid()  < _test_packet.get_block_height())   //view#id must >= block height
               || (_test_packet.get_block_viewid()  <= lock_block->get_viewid()) //new view#id must > last commit ' one
               || (_test_packet.get_block_height()  <= lock_block->get_height()) //new block#height must > last commit' one
               || (_test_packet.get_block_chainid() != lock_block->get_chainid())
               || (_test_packet.get_block_account() != lock_block->get_account())
               )
            {
                return false;
            }
            return true;
        }
        
        ////////////////////////////////////safe rules for packets////////////////////////////////////////////
        bool xBFTRules::safe_check_for_proposal_packet(base::xcspdu_t & packet,xproposal_msg_t & out_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account()) )
            {
                return false;
            }
                        
            if(out_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;
            
            if(out_msg.get_block_object().empty()) //alow empty body of block
                return false;
            
            return true;
        }

        bool xBFTRules::safe_check_for_proposal_packet(base::xcspdu_t & packet,xproposal_msg_v2_t & out_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account()) )
            {
                return false;
            }
                        
            if(out_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;
            
            if(out_msg.get_block_object().empty()) //alow empty body of block
                return false;
            
            return true;
        }
        
        bool xBFTRules::safe_check_for_vote_packet(base::xcspdu_t & in_packet,xvote_msg_t & out_msg)
        {
            if(safe_check_for_packet(in_packet) == false)
                return false;
            
            if(out_msg.serialize_from_string(in_packet.get_msg_body()) <= 0) //invalid packet
                return false;
            
            if(out_msg.get_justify_source().empty())
                return false;
            
            return true;
        }
        
        bool xBFTRules::safe_check_for_commit_packet(base::xcspdu_t & packet,xcommit_msg_t & out_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_viewid()  < packet.get_block_height())   //view#id must >= block height
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account())
               )
            {
                return false;
            }
 
            if(out_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;

            return true;
        }
        
        bool xBFTRules::safe_check_for_sync_request_packet(base::xcspdu_t & packet,xsync_request_t & _syncrequest_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account()) )
            {
                return false;
            }
            
            if(_syncrequest_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;
        
            if(_syncrequest_msg.get_block_hash().empty()) //input & output'root hash might be nil
                return false;
            
            return true;
        }
        
        bool xBFTRules::safe_check_for_sync_respond_packet(base::xcspdu_t & packet,xsync_respond_t & _sync_respond_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_viewid() < packet.get_block_height())   //view#id must >= block height
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account())
               )
            {
                return false;
            }
            
            if(_sync_respond_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;
            
            if(_sync_respond_msg.get_block_object().empty()) //block'header & cert must be ready
                return false;
            
            return true;
        }

        bool xBFTRules::safe_check_for_sync_respond_v2_packet(base::xcspdu_t & packet,xsync_respond_v2_t & _sync_respond_msg)
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == lock_block)
               || (packet.get_block_viewid() < packet.get_block_height())   //view#id must >= block height
               || (packet.get_block_chainid() != lock_block->get_chainid())
               || (packet.get_block_account() != lock_block->get_account())
               )
            {
                return false;
            }
            
            if(_sync_respond_msg.serialize_from_string(packet.get_msg_body()) <= 0) //invalid packet
                return false;
            
            if(_sync_respond_msg.get_block_object().empty()) //block'header & cert must be ready
                return false;
            
            return true;
        }
        
        ////////////////////////////////////safe rules for blocks////////////////////////////////////////////
        
        /*safe rule for any proposal block or any one of m_proposal_blocks
         1.pass the minimal rule for any block by safe_check_for_block();
         2.has input and output for non-nil block
         3.pass test for is_valid(true)
         
         it only allow signature of certification is not ready
         */
        bool xBFTRules::safe_check_for_proposal_block(base::xvblock_t * _proposal_block)//safe rule for proposal block
        {
            //sanity test first, check view#id of proposal,that must over > last locked one
            if( safe_check_for_block(_proposal_block) == false)
                return false;
            
            // //then ensure input & output are consisten  -- XTODO backup vote has no input
            // if(false == _proposal_block->is_input_ready(false))  //input resources created after verify_proposal, here may has no input
            // {
            //     xerror("xBFTRules::safe_check_for_proposal_block,input is not ready for  block=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
            //     return false;
            // }
            //block 'flags must clean before proposal
            if(_proposal_block->get_block_flags() != 0)
            {
                xerror("xBFTRules::safe_check_for_proposal_block,invalid block flags, proposal=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            #ifndef DEBUG //enable exception check at release mode first
            if(false == _proposal_block->get_block_hash().empty())
            {
                xerror("xBFTRules::safe_check_for_proposal_block,invalid block that build block hash before authenticated , proposal=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            #endif
            
            //now ready to do deep verification for hash based on header and input 'binary data
            // if(_proposal_block->is_valid(true) == false)
            // {
            //     xerror("xBFTRules::safe_check_for_proposal_block,it is not a valid block=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
            //     return false;
            // }
            return true;
        }
            
        //return  1     when true
        //return  -1    when false
        //return  0     when unknow
        int xBFTRules::safe_check_follow_commit_branch(base::xvblock_t * _test_for_block)
        {
            if(NULL == _test_for_block)
                return -1;//failed
            
            //need follow branch of commit
            base::xvblock_t *  latest_commit_block = get_commit_block();
            if(latest_commit_block != NULL)
            {
                if(_test_for_block->get_height() == (uint64_t)(latest_commit_block->get_height() - 1) )
                {
                    if(_test_for_block->get_block_hash() != latest_commit_block->get_last_block_hash())
                    {
                        xwarn("xBFTRules::safe_check_follow_commit_branch,fail-cert not follow the commited branch, cert(%s) vs commited(%s) at node=0x%llx",_test_for_block->dump().c_str(), latest_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;//failed
                    }
                    return 1;//good
                }
                else if(_test_for_block->get_height() == latest_commit_block->get_height())
                {
                    if(_test_for_block->get_block_hash() != latest_commit_block->get_block_hash())
                    {
                        xwarn("xBFTRules::safe_check_follow_commit_branch,fail-cert try fork from the commited branch, cert(%s) vs commited(%s) at node=0x%llx",_test_for_block->dump().c_str(), latest_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;//failed
                    }
                    return 1;//good
                }
                else if(_test_for_block->get_height() == (latest_commit_block->get_height() + 1))
                {
                    if(_test_for_block->get_last_block_hash() != latest_commit_block->get_block_hash())
                    {
                        xerror("xBFTRules::safe_check_follow_commit_branch,fail-cert not follow the commited branch, cert(%s) vs commited(%s) at node=0x%llx",_test_for_block->dump().c_str(), latest_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;//failed
                    }
                    return 1;//good
                }
                else if(_test_for_block->get_height() == (latest_commit_block->get_height() + 2))
                {
                    if(_test_for_block->get_justify_cert_hash() != latest_commit_block->get_input_root_hash())
                    {
                        xerror("xBFTRules::safe_check_follow_commit_branch,fail-justify cert hash unmatch,cert(%s) vs commited(%s) at node=0x%llx",_test_for_block->dump().c_str(), latest_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;//failed
                    }
                    return 1; //good
                }
            }
            return 0; //un-determined yet
        }
        
        //return  1     when true
        //return  -1    when false
        //return  0     when unknow
        int xBFTRules::safe_check_follow_locked_branch(base::xvblock_t * _test_for_block)
        {
            if(NULL == _test_for_block)
                return -1;
            
            const uint64_t locked_block_height = get_lock_block()->get_height();
            if(_test_for_block->get_height() == (locked_block_height -1) )
            {
                if(_test_for_block->get_block_hash() != get_lock_block()->get_last_block_hash())
                {
                    xwarn("xBFTRules::safe_check_follow_locked_branch,fail-block is not linked by locked block(height:%" PRIu64 ") vs proposal=%s,at node=0x%llx",get_lock_block()->get_height(),_test_for_block->dump().c_str(),get_xip2_addr().low_addr);
                    return -1;
                }
                return 1;
            }
            else if(_test_for_block->get_height() == locked_block_height)
            {
                if(_test_for_block->get_block_hash() != get_lock_block()->get_block_hash())
                {
                    xwarn("xBFTRules::safe_check_follow_locked_branch,fail-block with same height of locked,but different hash of proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                    return -1;
                }
                return 1;
            }
            else if(_test_for_block->get_height() == (locked_block_height + 1) )
            {
                if(_test_for_block->get_last_block_hash() != get_lock_block()->get_block_hash())
                {
                    xerror("xBFTRules::safe_check_follow_locked_branch,fail-proposal try to fork at locked block of prev, proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                    return -1;
                }
                return 1;
            }
            else if(_test_for_block->get_height() == (locked_block_height + 2) )
            {
                if(_test_for_block->get_justify_cert_hash() != get_lock_block()->get_input_root_hash())
                {
#if defined(ENABLE_METRICS)
                    auto fork_tag = "cons_lock_fork_" + get_account();
                    XMETRICS_COUNTER_INCREMENT( fork_tag , 1);
#endif
                    xerror("xBFTRules::safe_check_follow_locked_branch,fail-proposal justify cert hash unmatch of prev->prev, proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                    return -1;
                }
                return 1;
            }
            else
            {
                xdbg("xBFTRules::safe_check_follow_locked_branch,proposal is far away from locked block(height:%" PRIu64 ") vs proposal=%s,at node=0x%llx",get_lock_block()->get_height(),_test_for_block->dump().c_str(),get_xip2_addr().low_addr);
                
                return 0;  //unknow since too far than current locked
            }
        }
        
        bool  xBFTRules::safe_align_with_blockstore(xproposal_t* new_proposal)
        {
            base::xvblock_t * _peer_block = new_proposal->get_block();
            base::xvqcert_t * _peer_prev_block_cert = new_proposal->get_last_block_cert();
            //add more specific rule:proposal must point the lowest viewid of prev_height
            if(_peer_block->get_height() > 1)
            {
                base::xauto_ptr<base::xvbindex_t> local_prev_block_cert(get_vblockstore()->load_block_index(*this, _peer_block->get_height() - 1, base::enum_xvblock_flag_authenticated));//find most early cert block
                if(!local_prev_block_cert)
                {
                    xwarn("xBFTRules::safe_align_with_blockstore,fail-dont found prev block for proposal(%s) vs dump=%s at node=0x%llx", _peer_block->dump().c_str(),dump().c_str(),get_xip2_low_addr());
                    return false; //ask leader sync cert/hq block from this backup
                }
                else if(local_prev_block_cert->get_viewid() != _peer_prev_block_cert->get_viewid())//must alignment with blockstore
                {
                    xwarn("xBFTRules::safe_align_with_blockstore,fail-unmatch prev-block(%s) for proposal(%s) vs dump=%s at node=0x%llx",local_prev_block_cert->dump().c_str(), _peer_block->dump().c_str(),dump().c_str(),get_xip2_low_addr());
                    return false; //ask leader sync cert/hq block from this backup
                }
            }
            return true;
        }
    
        /*safe rule for any voting block
         1. first it must be a valid proposal block  and pass the locked block,both done  at safe_check_for_proposal_block
         2. never voted at same view#,and never voted passed and voted-view#; and never voted passed height
         3. never voted confilct with existing proposal of same view# or lower height#
         4. never fork from locked block
         */
        //bool  xBFTRules::safe_check_for_vote_block(base::xvblock_t * _vote_block)//safe rule for voting block
        bool  xBFTRules::safe_precheck_for_voting(xproposal_t* new_proposal)
        {
            if(new_proposal->is_vote_disable())
                return false;
            
            base::xvblock_t * _peer_block = new_proposal->get_block();
            const bool result = safe_precheck_for_voting(_peer_block);
            return result;
        }
    
        bool  xBFTRules::safe_precheck_for_voting(base::xvblock_t * _vote_block)//safe rule for voting block
        {
            //safe-rule#1: a valid proposal block  and pass the locked block
            if(safe_check_for_proposal_block(_vote_block) == false)
            {
                xinfo("xBFTRules::safe_precheck_for_voting,fail-invalid proposal(%s) vs local(%s),at node=0x%llx",_vote_block->dump().c_str(),dump().c_str(),get_xip2_low_addr());
                return false;
            }

            //safe-rule#2: never voted at same view#,and never voted passed and voted-view#; and never voted passed height
            //possibly might have mutiple proposals with different view# at same height
            if(   (_vote_block->get_viewid() <= get_latest_voted_viewid())  //only vote once for one view#
               || (_vote_block->get_height() <  get_latest_voted_height()) )//never vote for last block again
            {
                xwarn("xBFTRules::safe_check_for_vote_block,warn-has voted at higher view(%" PRIu64 ") than proposal=%s at node=0x%llx",m_latest_voted_viewid,_vote_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            base::xvblock_t *  latest_cert_block = get_latest_cert_block();
            if(latest_cert_block != NULL)
            {
                //note:cert now have concert about timeout,so it might still have cert at this heigth but at timeout status
                if(  (_vote_block->get_viewid() <= latest_cert_block->get_viewid())
                   ||(_vote_block->get_height() <  latest_cert_block->get_height()) )
                {
                    xwarn("xBFTRules::safe_check_for_vote_block,warn-conflict existing cert, proposal=%s < latest_cert_block=%s at node=0x%llx",_vote_block->dump().c_str(), latest_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
            }
            
            //safe-rule#3: never voted confilct with existing proposal of same view# or lower height#
            base::xvblock_t *  latest_proposal = get_latest_proposal_block();
            if(latest_proposal != NULL) //never vote behind proposal block
            {
                //note:proposal might be at timeout,so it might still have propoal at this heigth but at timeout status
                if(  (_vote_block->get_viewid() <=  latest_proposal->get_viewid())
                   ||(_vote_block->get_height() <   latest_proposal->get_height()) )
                {
                    xwarn("xBFTRules::safe_check_for_vote_block,warn-conflict existing proposal, proposal=%s <= latest_proposal=%s at node=0x%llx",_vote_block->dump().c_str(), latest_proposal->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
            }
            
            return true;
        }
        
        //check again before send voting msg and after verified signature
        bool  xBFTRules::safe_finalcheck_for_voting(xproposal_t* new_proposal)
        {
            if(new_proposal->is_vote_enable() == false)
            {
                xwarn("xBFTRules::safe_finalcheck_for_voting,warn-disabled proposal=%s at node=0x%llx",new_proposal->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            if(new_proposal->is_voted()) //not allow double voted
            {
                xwarn("xBFTRules::safe_finalcheck_for_voting,warn-voted proposal=%s at node=0x%llx",new_proposal->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            
            base::xvblock_t * _peer_block = new_proposal->get_block();
            const bool result = safe_finalcheck_for_voting(_peer_block);
            return result;
        }
    
        bool  xBFTRules::safe_finalcheck_for_voting(base::xvblock_t * _vote_block)//safe rule for voting block
        {
            base::xvblock_t * lock_block = get_lock_block();
            if( (NULL == _vote_block) || (NULL == lock_block) )
                return false;

            if( (_vote_block->get_height() <= lock_block->get_height()) || (_vote_block->get_viewid()  <= lock_block->get_viewid()))
            {
                xwarn("xBFTRules::safe_finalcheck_for_voting,warn-behind than locked, proposal=%s < locked_block=%s at node=0x%llx",_vote_block->dump().c_str(), lock_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;//never vote for behind one than locked block
            }
            
            base::xvblock_t *  latest_cert_block = get_latest_cert_block();
            if(latest_cert_block != NULL)//never vote for behind one than latest cert(hqc)
            {
                //note:cert now have concert about timeout,so it might still have cert at this heigth but at timeout status
                if(  (_vote_block->get_viewid() <= latest_cert_block->get_viewid())
                   ||(_vote_block->get_height() <  latest_cert_block->get_height()) )
                {
                    xwarn("xBFTRules::safe_finalcheck_for_voting,warn-conflict existing cert, proposal=%s < latest_cert_block=%s at node=0x%llx",_vote_block->dump().c_str(), latest_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
            }
            //safe-rule#4: never fork from locked block
            if(safe_check_follow_locked_branch(_vote_block) > 0) //only pass when restrictly connected locked
            {
                if(_vote_block->get_height() == (get_lock_block()->get_height() + 2) ) //one more check
                {
                    //safe-rule#5: any proposal must found the full data of hqc and locked block
                    for(auto it = m_certified_blocks.rbegin(); it != m_certified_blocks.rend(); ++it)
                    {
                        if(   (_vote_block->get_height() == (it->second->get_height() + 1)) //proposal connect to hqc
                           && (_vote_block->get_last_block_hash() == it->second->get_block_hash()) )
                        {
                            if(get_lock_block()->get_block_hash() == it->second->get_last_block_hash())//hqc connect to locked block
                                return true;
                            
                            xerror("xBFTRules::safe_finalcheck_for_voting,fail-prev-qc is not connect to locked block, proposal=%s and  prev-qc=%s at node=0x%llx",_vote_block->dump().c_str(), it->second->dump().c_str(),get_xip2_addr().low_addr);
                        }
                    }
                    xwarn("xBFTRules::safe_finalcheck_for_voting,warn-proposal not found prev-hqc for proposal=%s,at node=0x%llx",_vote_block->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
                return true;
            }
            xwarn("xBFTRules::safe_finalcheck_for_voting,fail-as safe_check_follow_locked_branch");
            return false;
        }
        
        bool  xBFTRules::safe_check_for_sync_block(base::xvblock_t * _commit_block)//safe rule for commit block
        {
            //step#2: never fork from locked block
            if(safe_check_follow_commit_branch(_commit_block) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::safe_check_for_sync_block,fail-as safe_check_follow_locked_branch");
                return false;
            }
            
            //step#3: then test block 'valid or deliver. note:ensure input & output are consisten
            if(false == _commit_block->is_body_and_offdata_ready(false))  // XTODO already check resource hash when set resource
            {
                xerror("xBFTRules::safe_check_for_sync_block,input not ready for block=%s at node=0x%llx",_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            //step#4:now ready to do deep verification for hash based on header and input 'binary data
            if(false == _commit_block->is_deliver(true))
            {
                xerror("xBFTRules::safe_check_for_sync_block,it is not a valid block=%s at node=0x%llx",_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            return true;
        }

    };//end of namespace of xconsensus
    
};//end of namespace of top
