// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xconsdriver.h"

#include "xmetrics/xmetrics.h"

namespace top
{
    namespace xconsensus
    {
        xBFTSyncdrv::xBFTSyncdrv(xcscoreobj_t & parent_object)
            :xBFTRules(parent_object)
        {
            xinfo("xBFTSyncdrv::xBFTSyncdrv,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }

        xBFTSyncdrv::~xBFTSyncdrv()
        {
            m_syncing_requests.clear();
            xinfo("xBFTSyncdrv::~xBFTSyncdrv,destroy,this=%p",this);
        }

        bool xBFTSyncdrv::set_lock_block(base::xvblock_t* latest_lock_block)
        {
            if(xBFTRules::set_lock_block(latest_lock_block))
            {
                //recheck syncing request
                for(auto it = m_syncing_requests.begin(); it != m_syncing_requests.end();)
                {
                    auto old_it = it; //copy it first
                    ++it; //move forward

                    if(latest_lock_block->get_height() > (old_it->second.target_height + 32) )//only allow sync around blocks
                        m_syncing_requests.erase(old_it);//erase old one
                }
                return true;
            }
            return false;
        }

        //fire view-change event
        bool  xBFTSyncdrv::on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xcsview_fire * _ev_obj = (xcsview_fire*)&event;
            m_latest_viewid = std::max(m_latest_viewid,_ev_obj->get_viewid());//record latest viewid
            
            for(auto it = m_syncing_requests.begin(); it != m_syncing_requests.end();)
            {
                auto old_it = it; //copy it first
                ++it; //move forward

                if(old_it->second.expired_viewid != 0) //0 == not set
                {
                    if(_ev_obj->get_viewid() >= old_it->second.expired_viewid)//check expired viewid
                        m_syncing_requests.erase(old_it);//erase old one
                }
                else if(old_it->second.expired_clock != 0)  //0 == not set
                {
                    if(_ev_obj->get_clock() >= old_it->second.expired_clock)//check expired clock
                        m_syncing_requests.erase(old_it);//erase old one
                }
            }
            return true;
        }

        //clock block always pass by higher layer to lower layer
        bool  xBFTSyncdrv::on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xcsclock_fire* _ev_obj = (xcsclock_fire*)&event;
            m_latest_clock = std::max(m_latest_clock,_ev_obj->get_clock_block()->get_height());
            
            for(auto it = m_syncing_requests.begin(); it != m_syncing_requests.end();)
            {
                auto old_it = it; //copy it first
                ++it; //move forward

                if(old_it->second.expired_clock != 0)  //0 == not set
                {
                    if(_ev_obj->get_clock_block()->get_height() >= old_it->second.expired_clock)//check expired clock
                        m_syncing_requests.erase(old_it);//erase old one
                }
            }
            return true;
        }

        bool xBFTSyncdrv::send_sync_request(const xvip2_t & from_addr,const xvip2_t & to_addr,const uint64_t target_block_height,const std::string & target_block_hash,base::xvqcert_t* proof_cert,const uint64_t proof_cert_height,const uint64_t expired_at_clock,const uint64_t chainid)
        {
            return send_sync_request(from_addr,to_addr,target_block_height,target_block_hash,proof_cert->get_viewid(),proof_cert->get_viewtoken(),proof_cert_height,expired_at_clock,chainid);
        }

        bool xBFTSyncdrv::send_sync_request(const xvip2_t & from_addr,const xvip2_t & to_addr,const uint64_t target_block_height,const std::string & target_block_hash,const uint64_t proof_block_viewid,const uint32_t proof_block_viewtoken,const uint64_t proof_block_height,const uint64_t expired_at_clock,const uint64_t chainid)
        {
            if(get_parent_node() != NULL && target_block_height > 0)
            {
                const uint32_t  sync_cookie = base::xtime_utl::get_fast_randomu();
                const std::string sync_key  = target_block_hash + base::xstring_utl::tostring(target_block_height);
                auto insert_result = m_syncing_requests.emplace(sync_key,xsyn_request(target_block_height,expired_at_clock,1));
                if(false == insert_result.second)//duplicated sync request
                {
                    insert_result.first->second.sync_trycount += 1;
                    if(insert_result.first->second.sync_trycount > 1) //allow retry 1 times
                    {
                        xdbg("xBFTSyncdrv::send_sync_request,duplicated request for block={height=%llu with proof of viewid=%llu,viewtoken=%u to node=0x%llx,at node=0x%llx",target_block_height,proof_block_viewid,proof_block_viewtoken,to_addr.low_addr,from_addr.low_addr);
                        return true;
                    }
                }

                std::string msg_stream;
                xsync_request_t _sync_request(enum_xsync_target_block_object | enum_xsync_target_block_input | enum_xsync_target_block_output, sync_cookie,target_block_height,target_block_hash);
                _sync_request.serialize_to_string(msg_stream);

                //construct request msg here
                base::xauto_ptr<xcspdu_fire>_event_obj(new xcspdu_fire(get_target_pdu_class()));
                _event_obj->set_from_xip(from_addr);
                _event_obj->set_to_xip(to_addr);
                _event_obj->_packet.set_block_chainid((uint32_t)chainid);
                _event_obj->_packet.set_block_account(get_account());
                _event_obj->_packet.set_block_height(proof_block_height);
                _event_obj->_packet.set_block_viewid(proof_block_viewid);
                _event_obj->_packet.set_block_viewtoken(proof_block_viewtoken);
                _event_obj->_packet.set_block_clock(0);

                _event_obj->_packet.reset_message(xsync_request_t::get_msg_type(), get_default_msg_ttl(),msg_stream,0,from_addr.low_addr,to_addr.low_addr);

                xinfo("xBFTSyncdrv::send_sync_request,send request for target block={height=%llu} with proof of height=%llu,viewid=%llu,viewtoken=%u to node=0x%llx,at node=0x%llx",target_block_height,proof_block_height,proof_block_viewid,proof_block_viewtoken,to_addr.low_addr,from_addr.low_addr);
                get_parent_node()->push_event_up(*_event_obj, this, get_thread_id(), get_time_now());

                return true;
            }
            return  false;
        }
    
        //return true if fired sync request
        bool  xBFTSyncdrv::resync_local_and_peer(base::xvblock_t* peer_block,const xvip2_t & peer_addr,const xvip2_t & my_addr,const uint64_t cur_clock)
        {
            if( (NULL == peer_block) || (get_lock_block() == NULL) )
                return false;
            
            //allow sync addtional blocks around the locked block
            if( (peer_block->get_height() > 1) && (peer_block->get_height() >= get_lock_block()->get_height()) )
            {
                base::xauto_ptr<base::xvbindex_t> prev_index(load_block_index(peer_block->get_height() - 1, peer_block->get_last_block_hash()));
                if(!prev_index) //fire sync request for prev block of _sync_block
                {
                    xinfo("xBFTSyncdrv::resync_local_and_peer,request prev one <- block(%s) from peer(0x%llx) at  node(0x%llx)",peer_block->dump().c_str(),peer_addr.low_addr,my_addr.low_addr);
                    
                    send_sync_request(my_addr,peer_addr, (peer_block->get_height() - 1),peer_block->get_last_block_hash(),peer_block->get_cert(),peer_block->get_height(),cur_clock + 1,peer_block->get_chainid());
                    
                    return true;
                }
                else
                {
                    base::xauto_ptr<base::xvbindex_t> prev_prev_index(load_block_index(prev_index->get_height() - 1, prev_index->get_last_block_hash()));
                    if(!prev_prev_index)  //fire sync request for prev_prev block of _sync_block
                    {
                        xinfo("xBFTSyncdrv::resync_local_and_peer,request prev one <- block(%s) from peer(0x%llx) at  node(0x%llx)",prev_index->dump().c_str(),peer_addr.low_addr,my_addr.low_addr);
                        
                        send_sync_request(my_addr,peer_addr, (prev_index->get_height() - 1),prev_index->get_last_block_hash(),peer_block->get_cert(),peer_block->get_height(),cur_clock + 1,peer_block->get_chainid());
                        
                        return true;
                    }
                }
            }
            return false;
        }

        int   xBFTSyncdrv::handle_sync_request_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent)
        {
            //step#0: verified that replica and leader are valid by from_addr and to_addr at top layer like xconsnetwork or xconsnode_t. here just consider pass.
            base::xcspdu_t & packet = event_obj->_packet;
            //step#1: do sanity check and verify proposal packet first, also do check whether behind too much
            xsync_request_t _syncrequest_msg;
            if(safe_check_for_sync_request_packet(packet,_syncrequest_msg) == false)
            {
                xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-safe_check_for_sync_request_packet for packet=%s vs driver:%s,at node=0x%llx",packet.dump().c_str(),dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_packet;
            }

            //step#2: do basic safe check
            //note: packet carry the proof information to allow download some blocks
            bool is_proof_check_pass = false;
            {
                if(packet.get_block_height() > get_lock_block()->get_height())
                {
                    auto  cert_blocks = get_cert_blocks();
                    for(auto it = cert_blocks.rbegin(); it != cert_blocks.rend(); ++it)
                    {
                        if(  (it->second->get_height()     == packet.get_block_height())
                           &&(it->second->get_viewid()     == packet.get_block_viewid())
                           &&(it->second->get_viewtoken()  == packet.get_block_viewtoken()) )
                        {
                            is_proof_check_pass = true;
                            break;
                        }
                    }
                }
                else if(packet.get_block_height() == get_lock_block()->get_height())
                {
                    if(  (get_lock_block()->get_viewid()    == packet.get_block_viewid())
                       &&(get_lock_block()->get_viewtoken() == packet.get_block_viewtoken()) )
                    {
                        is_proof_check_pass = true;
                    }
                }
                
                if(false == is_proof_check_pass)
                {
                    //search from blockstore at nearby locked block
                    {
                        base::xauto_ptr<base::xvbindex_t> proof_block = get_vblockstore()->load_block_index(*this, packet.get_block_height(),packet.get_block_viewid());//packet carry proof 'info
                        if(proof_block != nullptr)
                        {
                            if(  (proof_block->get_viewid()    == packet.get_block_viewid())
                               &&(proof_block->get_viewtoken() == packet.get_block_viewtoken()) )
                            {
                                is_proof_check_pass = true;
                            }
                        }
                    }
                }
 
            }
            if(false == is_proof_check_pass)
            {
                xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-is_proof_check_pass for packet=%s vs driver:%s,at node=0x%llx",packet.dump().c_str(),dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_not_authorized;
            }

            //step#3: now search local to find target block to send peer
            base::xvblock_t *   _local_block = NULL;
            const uint64_t      target_block_height = _syncrequest_msg.get_block_height();
            const std::string&  target_block_hash   = _syncrequest_msg.get_block_hash();
            const uint32_t      sync_targets        = _syncrequest_msg.get_sync_targets();
            if(target_block_height > get_lock_block()->get_height())//search under cert blocks
            {
                auto  cert_blocks = get_cert_blocks();
                for(auto it = cert_blocks.rbegin(); it != cert_blocks.rend(); ++it)
                {
                    if(  (it->second->get_height()     == target_block_height)
                       &&(it->second->get_block_hash() == target_block_hash) )
                    {
                        _local_block = it->second;
                        _local_block->add_ref(); //hold reference
                        break;
                    }
                }
                if(NULL == _local_block) //search from blockstore
                {
                    base::xauto_ptr<base::xvblock_t> target_block = get_vblockstore()->load_block_object(*this, target_block_height, target_block_hash,true, metrics::blockstore_access_from_bft_sync);
                    if(target_block == nullptr)
                    {
                        xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-found target cert of height:%llu,at node=0x%llx",target_block_height,get_xip2_low_addr());
                        return enum_xconsensus_error_not_found;
                    }
                    
                    #ifdef DEBUG
                    xassert(target_block->is_input_ready(true));
                    xassert(target_block->is_output_ready(true));
                    #else
                    xassert(target_block->is_input_ready(false));
                    xassert(target_block->is_output_ready(false));
                    #endif
                    
                    bool found_matched_proposal = false;
                    add_cert_block(target_block.get(),found_matched_proposal);//fill into cache list

                    _local_block = target_block.get();
                    _local_block->add_ref(); //hold reference
                }
            }
            else if(target_block_height == get_lock_block()->get_height())
            {
                _local_block = get_lock_block();
                _local_block->add_ref(); //hold reference
            }
            
            if(NULL == _local_block)
            {
                if( (get_lock_block()->get_height() - target_block_height) < 128 )//search from blockstore at nearby locked block
                {
                    xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-sync too old block(heigh:%llu) from packet=%s,at node=0x%llx",target_block_height,packet.dump().c_str(),get_xip2_low_addr());
                    //return enum_xconsensus_error_outofdate; //note:open for download any block now
                }
                
                bool full_load = (sync_targets & enum_xsync_target_block_input) || (sync_targets & enum_xsync_target_block_output);
                base::xauto_ptr<base::xvblock_t> target_block = get_vblockstore()->load_block_object(*this, target_block_height,target_block_hash,full_load, metrics::blockstore_access_from_bft_sync);//specific load target block
                if(target_block == nullptr)
                {
                    xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-found target block of height:%llu,at node=0x%llx",target_block_height,get_xip2_low_addr());
                    return enum_xconsensus_error_not_found;
                }

                if(target_block_hash != target_block->get_block_hash())
                {
                    xwarn("xBFTSyncdrv::handle_sync_request_msg,fail-unmatched local-block for packet=%s vs local-block=%s,at node=0x%llx",packet.dump().c_str(),target_block->dump().c_str(),get_xip2_low_addr());
                    return enum_xconsensus_error_bad_packet;
                }

                _local_block = target_block.get();
                _local_block->add_ref(); //hold reference
            }

            //step#2: verify view_id & viewtoken etc to protect from DDOS attack by local block
            base::xauto_ptr<base::xvblock_t> safe_release(_local_block); //release reference when quit automatically
            if(NULL != _local_block)
            {
                //TODO,add another protection for DDOS ,attacker may broadcast request frequently to eat bandwidth here
                //add the requester'xip to set ,and filter the duplicated one
                xsync_respond_t respond_msg(sync_targets,_syncrequest_msg.get_sync_cookie());

                std::string block_object_bin;
                _local_block->serialize_to_string(block_object_bin);
                respond_msg.set_block_object(block_object_bin);

                if(sync_targets & enum_xsync_target_block_input)
                {
                    if(  (_local_block->get_input()->get_resources_hash().empty() == false) //link resoure data
                       &&(_local_block->get_input()->has_resource_data() == false) ) //but dont have resource avaiable now
                    {
                        //_local_block need reload input resource
                        get_vblockstore()->load_block_input(*this, _local_block);
                        xassert(_local_block->get_input()->has_resource_data());
                    }
                    respond_msg.set_input_resource(_local_block->get_input()->get_resources_data());
                }
                
                if(sync_targets & enum_xsync_target_block_output)
                {
                    if(  (_local_block->get_output()->get_resources_hash().empty() == false) //link resoure data
                       &&(_local_block->get_output()->has_resource_data() == false) ) //but dont have resource avaiable now
                    {
                        //_local_block need reload output resource
                        get_vblockstore()->load_block_output(*this, _local_block);
                        xassert(_local_block->get_output()->has_resource_data());
                    }
                    respond_msg.set_output_resource(_local_block->get_output()->get_resources_data());
                }

                std::string msg_stream;
                respond_msg.serialize_to_string(msg_stream);

                xinfo("xBFTSyncdrv::handle_sync_request_msg,deliver a block for packet=%s,cert-block=%s,at node=0x%llx",packet.dump().c_str(),_local_block->dump().c_str(),get_xip2_low_addr());
                fire_pdu_event_up(xsync_respond_t::get_msg_type(), msg_stream, packet.get_msg_nonce() + 1, to_addr, from_addr, _local_block);
            }
            else
            {
                xwarn("xBFTSyncdrv::handle_sync_request_msg,local cert block has been removed for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());
            }
            return enum_xconsensus_code_successful;
        }

        int   xBFTSyncdrv::handle_sync_respond_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent)
        {
            //step#0: verified that replica and leader are valid by from_addr and to_addr at top layer like xconsnetwork or xconsnode_t. here just consider pass.
            base::xcspdu_t & packet = event_obj->_packet;
            //step#1: /do sanity check and verify proposal packet first, also do check whether behind too much
            xsync_respond_t _sync_respond_msg;
            if(safe_check_for_sync_respond_packet(packet,_sync_respond_msg) == false)
            {
                xwarn("xBFTSyncdrv::handle_sync_respond_msg,fail-safe_check_for_sync_respond_packet for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_packet;
            }
            //one view# just only have one cert
            {
                base::xvblock_t* _local_cert_block = find_cert_block(packet.get_block_viewid());
                if(_local_cert_block)
                {
                    if(   (_local_cert_block->get_height()     != packet.get_block_height())
                       || (_local_cert_block->get_chainid()    != packet.get_block_chainid())
                       || (_local_cert_block->get_viewid()     != packet.get_block_viewid())
                       || (_local_cert_block->get_account()    != packet.get_block_account())
                       )
                    {
                        xwarn("xBFTSyncdrv::handle_sync_respond_msg,fail-unmatched packet=%s vs local certified block=%s,at node=0x%llx",packet.dump().c_str(),_local_cert_block->dump().c_str(),get_xip2_low_addr());
                        return enum_xconsensus_error_bad_packet;
                    }
                    xinfo("xBFTSyncdrv::handle_sync_respond_msg,target block has finished and deliver to certified _local_cert_block=%s, at node=0x%llx",_local_cert_block->dump().c_str(),get_xip2_low_addr());
                    return enum_xconsensus_code_successful;//local proposal block has verified and ready,so it is duplicated commit msg
                }
            }

            //step#2: veriry header/cert, input and output are consist with each others
            base::xauto_ptr<base::xvblock_t> _sync_block(base::xvblock_t::create_block_object(_sync_respond_msg.get_block_object()));
            if( (!_sync_block) || (false == _sync_block->is_valid(false)) )
            {
                xerror("xBFTSyncdrv::handle_sync_respond_msg,fail-invalid block from packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_block;
            }
            //then try to merge to local syncing request
            if(_sync_block->set_input_resources(_sync_respond_msg.get_input_resource()) == false)//not match input hash  in header
            {
                xerror("xBFTSyncdrv::handle_sync_respond_msg,fail-block of bad input from packet=%s vs sync_block=%s,at node=0x%llx",packet.dump().c_str(),_sync_block->dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_block;
            }
            if(_sync_block->set_output_resources(_sync_respond_msg.get_output_resource()) == false)//not match output hash in cert
            {
                xerror("xBFTSyncdrv::handle_sync_respond_msg,fail-block of bad output from packet=%s vs sync_block=%s,at node=0x%llx",packet.dump().c_str(),_sync_block->dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_block;
            }

            //step#3: verify request etc to protect from DDOS attack
            const std::string sync_key  = _sync_block->get_block_hash() + base::xstring_utl::tostring(_sync_block->get_height());
            auto sync_request_it = m_syncing_requests.find(sync_key);
            if(sync_request_it == m_syncing_requests.end())
            {
                xinfo("xBFTSyncdrv::handle_sync_respond_msg,warn-NOT find request for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());
                
                //XTODO, need restore it with better control later
                //here simply just pass any responsed block
            }

            //step#4: do deep check finally
            if(safe_check_for_sync_block(_sync_block.get()))
            {
                //note:#1 safe rule, always cleans up flags carried by peer
                _sync_block->reset_block_flags();  //now force to clean all flags for both block and cert
                if(sync_request_it != m_syncing_requests.end())
                    m_syncing_requests.erase(sync_request_it); //safe to remove local request now
                //fire asyn job to verify signature & cert then
                {
                    xinfo("xBFTSyncdrv::handle_sync_respond_msg,pulled un-verified commit-block:%s at node=0x%llx from peer:0x%llx,local(%s)",_sync_block->dump().c_str(),get_xip2_addr().low_addr,from_addr.low_addr,dump().c_str());
                    fire_verify_syncblock_job(_sync_block.get(),NULL);
                }
            }
            else
            {
                xwarn("xBFTSyncdrv::handle_sync_respond_msg,failed pass safe-check for block:%s at node=0x%llx from peer:0x%llx",_sync_block->dump().c_str(),get_xip2_addr().low_addr,from_addr.low_addr);
            }
            return enum_xconsensus_code_successful;
        }

        //note:for commit msg we need merger local proposal and received certifcate from leader
        //but  for sync msg "target_block" already carry full certifcate ,it no-need merge again
        bool xBFTSyncdrv::fire_verify_syncblock_job(base::xvblock_t * target_block,base::xvqcert_t * paired_cert)
        {
            if(NULL == target_block)
                return false;

            if(false == target_block->is_valid(true))//add addtional simple check,optional
                return false;

            //now safe to do heavy job to verify quorum_ceritification completely
            std::function<void(void*)> _after_verify_commit_job = [this](void* _block)->void{
                base::xvblock_t* _full_block_ = (base::xvblock_t*)_block;

                bool found_matched_proposal = false;
                if(add_cert_block(_full_block_,found_matched_proposal))//set certified block(QC block)
                {
                    //now recheck any existing proposal and could push to voting again
                    on_new_block_fire(_full_block_);
                    
                    if(found_matched_proposal)//on_proposal_finish event has been fired by add_cert_block
                    {
                        xinfo("xBFTSyncdrv::fire_verify_syncblock,deliver an proposal-and-authed block:%s at node=0x%llx",_full_block_->dump().c_str(),get_xip2_addr().low_addr);
                    }
                    else
                    {
                        xinfo("xBFTSyncdrv::fire_verify_syncblock,deliver an replicated-and-authed block:%s at node=0x%llx",_full_block_->dump().c_str(),get_xip2_addr().low_addr);
                        fire_replicate_finish_event(_full_block_);//call on_replicate_finish(block) to driver context layer
                    }
                }
                _full_block_->release_ref(); //release reference hold by _verify_function
            };

            if(paired_cert != nullptr) //manually add reference for _verify_function call
                paired_cert->add_ref();
            auto _verify_function = [this,paired_cert](base::xcall_t & call, const int32_t cur_thread_id,const uint64_t timenow_ms)->bool{
                base::xauto_ptr<base::xvqcert_t> _merge_cert(paired_cert);//auto release the added addtional once quit
                if(is_close() == false)
                {
                    base::xvblock_t* _for_check_block_ = (base::xvblock_t *)call.get_param1().get_object();
                    if (_for_check_block_->check_block_flag(base::enum_xvblock_flag_authenticated))
                    {
                        xinfo("xBFTSyncdrv::fire_verify_syncblock,successful already is cert block. block:%s at node=0x%llx",_for_check_block_->dump().c_str(),get_xip2_addr().low_addr);
                        return true;
                    }
                    
                    if( (_merge_cert != nullptr) && (false == _for_check_block_->merge_cert(*_merge_cert)) ) //here is thread-safe to merge cert into block
                    {
                        xwarn_err("xBFTSyncdrv::fire_verify_syncblock,fail-unmatched commit_cert=%s vs proposal=%s,at node=0x%llx",_merge_cert->dump().c_str(),_for_check_block_->dump().c_str(),get_xip2_low_addr());
                        return true;
                    }
                    XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_xbft, 1);
                    if(   _for_check_block_->check_block_flag(base::enum_xvblock_flag_authenticated)
                       || (get_vcertauth()->verify_muti_sign(_for_check_block_) == base::enum_vcert_auth_result::enum_successful) )
                    {
                        _for_check_block_->get_cert()->set_unit_flag(base::enum_xvblock_flag_authenticated);
                        _for_check_block_->set_block_flag(base::enum_xvblock_flag_authenticated);

                        xinfo("xBFTSyncdrv::fire_verify_syncblock,successful finish verify for commit block:%s at node=0x%llx",_for_check_block_->dump().c_str(),get_xip2_addr().low_addr);

                        base::xfunction_t* _callback_ = (base::xfunction_t *)call.get_param2().get_function();
                        if(_callback_ != NULL)
                        {
                            _for_check_block_->add_ref(); //hold reference for async
                            dispatch_call(*_callback_,(void*)_for_check_block_);
                        }
                    }
                    else
                        xwarn("xBFTSyncdrv::fire_verify_syncblock,fail-verify_muti_sign for block=%s,at node=0x%llx",_for_check_block_->dump().c_str(),get_xip2_low_addr());
                }
                return true;
            };
            base::xcall_t asyn_verify_call(_verify_function,(base::xobject_t*)target_block,&_after_verify_commit_job,(base::xobject_t*)this);
            asyn_verify_call.bind_taskid(get_account_index());
            base::xworkerpool_t * _workers_pool = get_workerpool();
            if(_workers_pool != NULL)
                return (_workers_pool->send_call(asyn_verify_call) == enum_xcode_successful);
            else
                return (dispatch_call(asyn_verify_call) == enum_xcode_successful);
        }

    };//end of namespace of xconsensus

};//end of namespace of top
