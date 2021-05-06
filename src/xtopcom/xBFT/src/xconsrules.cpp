// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>
#include "xconsdriver.h"

namespace top
{
    namespace xconsensus
    {
        //////////////////////////////////////////xBFTRules////////////////////////////////////////////////
        xBFTRules::xBFTRules(xcscoreobj_t & parent_object)
            :xcsdriver_t(parent_object)
        {
            m_latest_lock_block   = NULL;
            m_latest_voted_height = 0;
            m_latest_voted_viewid = 0;
            xinfo("xBFTRules::xBFTRules,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
        
        xBFTRules::~xBFTRules()
        {
            clean_proposals();
            clean_cert_blocks();
            
            if(m_latest_lock_block != NULL)
                m_latest_lock_block->release_ref();
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
                const base::xvblock_t * commit_block = m_latest_lock_block->get_prev_block();
                if(commit_block != NULL)
                    xprintf(local_param_buf,sizeof(local_param_buf),"{commited={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- locked={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hqc={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hviewid=%" PRIu64 "}",commit_block->get_height(),commit_block->get_viewid(),commit_block->get_viewtoken(),m_latest_lock_block->get_height(),m_latest_lock_block->get_viewid(),m_latest_lock_block->get_viewtoken(),_latest_cert->get_height(),_latest_cert->get_viewid(),_latest_cert->get_viewtoken(),get_latest_voted_viewid());
                else
                    xprintf(local_param_buf,sizeof(local_param_buf),"{commited={nil} <-- locked={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hqc={height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u} <-- hviewid=%" PRIu64 "}",m_latest_lock_block->get_height(),m_latest_lock_block->get_viewid(),m_latest_lock_block->get_viewtoken(),_latest_cert->get_height(),_latest_cert->get_viewid(),_latest_cert->get_viewtoken(),get_latest_voted_viewid());
            }
            return std::string(local_param_buf);
        }
        
        //////////////////////////////block managed fro proposal//////////////////////////////
        base::xvblock_t * xBFTRules::get_lock_block()
        {
            if(m_latest_lock_block != nullptr)
                return m_latest_lock_block;
            
            if(get_vblockstore() != nullptr)
            {
                base::xauto_ptr<base::xvblock_t>  latest_lock_block = get_vblockstore()->get_latest_locked_block(*this);
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
                    xdbg("xBFTRules::set_lock_block, old-lock=%s -> new-lock=%s,at node=0x%llx,this=%llx",m_latest_lock_block->dump().c_str(),latest_lock_block->dump().c_str(),get_xip2_addr().low_addr,(int64_t)this);
                    
                    m_latest_lock_block->release_ref();
                    m_latest_lock_block = NULL;
                }
                else
                {
                    xkinfo("xBFTRules::set_lock_block, nil-block ->new=%s,at node=0x%llx,this=%llx",latest_lock_block->dump().c_str(), get_xip2_addr().low_addr,(int64_t)this);
                }
                m_latest_lock_block = latest_lock_block;
                
                //m_latest_voted_height and m_latest_voted_viewid always increase
                m_latest_voted_height = std::max(m_latest_voted_height,latest_lock_block->get_height());
                m_latest_voted_viewid = std::max(m_latest_voted_viewid,latest_lock_block->get_viewid());
                
                recheck_block(m_certified_blocks);//any cert block has been notified upper layer
                
                //filter too old proposal and put into removed_list
                std::vector<xproposal_t*> removed_list;
                for(auto it = m_proposal_blocks.begin(); it != m_proposal_blocks.end();)
                {
                    xproposal_t * _proposal = it->second;
                    if(safe_check_for_block(_proposal->get_block()) == false)
                    {
                        removed_list.push_back(_proposal);
                        
                        auto old_it = it; //copy it first
                        ++it; //move forward
                        m_proposal_blocks.erase(old_it);//erase old one
                        continue;
                    }
                    ++it;
                }
                //notify each one to upper layer as enum_xconsensus_error_outofdate
                if(removed_list.empty() == false)
                {
                    std::sort(removed_list.begin(), removed_list.end(), sort_proposal());
                    for(auto it = removed_list.begin(); it != removed_list.end(); ++it)
                    {
                        xproposal_t * _to_remove = *it;
                        const std::string errdetail;
                        fire_proposal_finish_event(enum_xconsensus_error_outofdate,errdetail,_to_remove->get_block(), NULL, get_lock_block(), get_latest_cert_block(), get_latest_proposal_block());
                        
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
                if(safe_check_for_block(_lock_block) == false)
                {
                    xdbg("xBFTRules::safe_check_for_lock_block,false for safe_check_for_block,driver=%s,at node=0x%llx",dump().c_str(),get_xip2_low_addr());
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
        
        bool   xBFTRules::recheck_block(std::map<uint64_t,base::xvblock_t*> & stdmap)//recheck every block for safe_check_for_block ,and remove it if fail test
        {
            for(auto it = stdmap.begin(); it != stdmap.end();)
            {
                base::xvblock_t * _block = it->second;
                if(safe_check_for_block(_block) == false)
                {
                    auto old_it = it; //copy it first
                    ++it; //move forward
                    
                    old_it->second->release_ref();
                    stdmap.erase(old_it);//erase old one
                    continue;
                }
                ++it;
            }
            return true;
        }

        xproposal_t*  xBFTRules::add_proposal(base::xvblock_t * proposal,base::xvblock_t * parent_block,const uint32_t expired_ms)
        {
            if(NULL == proposal)
                return NULL;
            
            xproposal_t* new_block_ptr = new xproposal_t(*proposal,parent_block);
            new_block_ptr->set_expired_ms(get_time_now() + expired_ms);
            if(add_proposal(*new_block_ptr))
                return new_block_ptr;
            
            new_block_ptr->release_ref();
            return NULL;
        }
        
        bool  xBFTRules::add_proposal(xproposal_t & proposal_block)
        {
            //m_latest_voted_height and m_latest_voted_viewid always increase
            m_latest_voted_height = std::max(m_latest_voted_height,proposal_block.get_height());
            m_latest_voted_viewid = std::max(m_latest_voted_viewid,proposal_block.get_viewid());
            
            return add_block(&proposal_block,m_proposal_blocks);
        }
        
        bool  xBFTRules::remove_proposal(const uint64_t view_id)
        {
            return  remove_block(view_id,m_proposal_blocks);
        }
        
        bool  xBFTRules::clean_proposals()
        {
            return clean_blocks(m_proposal_blocks);
        }
        
        base::xvblock_t*  xBFTRules::get_latest_proposal_block() const
        {
            xproposal_t* _propoal_wrap = get_latest_block(m_proposal_blocks);
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
        bool  xBFTRules::add_cert_block(base::xvblock_t* _target_block)
        {
            if(nullptr == _target_block)
                return false;
            
            //filtered forked cert if have
            if(get_lock_block() != nullptr)
            {
                if(safe_check_follow_locked_branch(_target_block) < 0)//allow unknow case continue
                {
                    xwarn("xBFTRules::add_cert_block,fail-as safe_check_follow_locked_branch");
                    return false;
                }
            }
            auto insert_result = m_certified_blocks.emplace(_target_block->get_viewid(),_target_block);
            if(insert_result.second)//note:not allow overwrited existing cert with same view#
                _target_block->add_ref();//hold referene for map
 
            return insert_result.second;//return true means it inserted into map at brandnew
        }
        bool  xBFTRules::remove_cert_block(const uint64_t view_id)
        {
            return  remove_block(view_id,m_certified_blocks);
        }
        bool  xBFTRules::clean_cert_blocks()
        {
            return clean_blocks(m_certified_blocks);
        }
        base::xvblock_t*   xBFTRules::get_latest_cert_block() const //caller need care to release reference once no-longer need
        {
            return get_latest_block(m_certified_blocks);
        }
        base::xvblock_t*   xBFTRules::find_cert_block(const uint64_t view_id) const//caller need care to release reference once no-longer need
        {
            return find_block(view_id,m_certified_blocks);
        }
        
        ////////////////////////////////////minimal safe rules///////////////////////////////////////////////
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
            if(safe_check_for_packet(packet) == false)
                return false;
            
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
            
            //then ensure input & output are consisten
            if(false == _proposal_block->is_input_ready(false))  //input resources created after verify_proposal, here may has no input
            {
                xerror("xBFTRules::safe_check_for_proposal_block,input is not ready for  block=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
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
            if(_proposal_block->is_valid(true) == false)
            {
                xerror("xBFTRules::safe_check_for_proposal_block,it is not a valid block=%s at node=0x%llx",_proposal_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            return true;
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
                    xwarn("xBFTRules::safe_check_follow_locked_branch,fail-proposal try to fork at locked block of prev, proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                    return -1;
                }
                return 1;
            }
            else if(_test_for_block->get_height() == (locked_block_height + 2) )
            {
                if(_test_for_block->get_header()->get_block_level() == base::enum_xvblock_level_unit)
                {
                    if(_test_for_block->get_justify_cert_hash() != get_lock_block()->get_block_hash())
                    {
                        xwarn("xBFTRules::safe_check_follow_locked_branch,fail-proposal try to fork at locked block of prev->prev, proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;
                    }
                }
                else //any cases for non-unit block
                {
                    if(_test_for_block->get_justify_cert_hash() != get_lock_block()->get_cert()->get_output_root_hash())
                    {
                        xwarn("xBFTRules::safe_check_follow_locked_branch,fail-proposal try to fork at locked block of prev->prev, proposal=%s vs locked=%s at node=0x%llx",_test_for_block->dump().c_str(), get_lock_block()->dump().c_str(),get_xip2_addr().low_addr);
                        return -1;
                    }
                }
                return 1;
            }
            else
            {
                xdbg("xBFTRules::safe_check_follow_locked_branch,proposal is far away from locked block(height:%" PRIu64 ") vs proposal=%s,at node=0x%llx",get_lock_block()->get_height(),_test_for_block->dump().c_str(),get_xip2_addr().low_addr);
                
                return 0;  //unknow since too far than current locked
            }
        }
        
        /*safe rule for any voting block
         1. first it must be a valid proposal block  and pass the locked block,both done  at safe_check_for_proposal_block
         2. never voted at same view#,and never voted passed and voted-view#; and never voted passed height
         3. never voted confilct with existing proposal of same view# or lower height#
         4. never fork from locked block
         */
        //bool  xBFTRules::safe_check_for_vote_block(base::xvblock_t * _vote_block)//safe rule for voting block
        bool  xBFTRules::safe_precheck_for_voting(base::xvblock_t * _vote_block)//safe rule for voting block
        {
            //safe-rule#1: a valid proposal block  and pass the locked block
            if(safe_check_for_proposal_block(_vote_block) == false)
                return false;
            
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
                if(  (_vote_block->get_viewid() <= latest_cert_block->get_viewid())
                   ||(_vote_block->get_height() <= latest_cert_block->get_height()) )
                {
                    xwarn("xBFTRules::safe_check_for_vote_block,warn-conflict existing cert, proposal=%s < latest_cert_block=%s at node=0x%llx",_vote_block->dump().c_str(), latest_cert_block->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
            }
            
            //safe-rule#3: never voted confilct with existing proposal of same view# or lower height#
            base::xvblock_t *  latest_proposal = get_latest_proposal_block();
            if(latest_proposal != NULL) //never vote behind proposal block
            {
                if(  (_vote_block->get_viewid() <= latest_proposal->get_viewid())
                   ||(_vote_block->get_height() <  latest_proposal->get_height()) )
                {
                    xwarn("xBFTRules::safe_check_for_vote_block,warn-conflict existing proposal, proposal=%s <= latest_proposal=%s at node=0x%llx",_vote_block->dump().c_str(), latest_proposal->dump().c_str(),get_xip2_addr().low_addr);
                    return false;
                }
            }
            
            //safe-rule#4: never fork from locked block
            if(safe_check_follow_locked_branch(_vote_block) < 0 ) //allow unknow case continue when pre-check
                return false;
            
            return true;
        }
        
        //check again before send voting msg and after verified signature
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
                if(  (_vote_block->get_viewid() <= latest_cert_block->get_viewid())
                   ||(_vote_block->get_height() <= latest_cert_block->get_height()) )
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
            if(safe_check_follow_locked_branch(_commit_block) < 0)//allow unknow case continue
            {
                xwarn("xBFTRules::safe_check_for_sync_block,fail-as safe_check_follow_locked_branch");
                return false;
            }
            
            //step#3: then test block 'valid or deliver. note:ensure input & output are consisten
            if(false == _commit_block->is_input_ready(true))
            {
                xerror("xBFTRules::safe_check_for_sync_block,input not ready for block=%s at node=0x%llx",_commit_block->dump().c_str(),get_xip2_addr().low_addr);
                return false;
            }
            if(false == _commit_block->is_output_ready(true))
            {
                xerror("xBFTRules::safe_check_for_sync_block,output not ready for block=%s at node=0x%llx",_commit_block->dump().c_str(),get_xip2_addr().low_addr);
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
