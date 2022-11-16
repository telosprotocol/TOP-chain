// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconsaccount.h"
#include "xconscontext.h"
#include "xmetrics/xmetrics.h"

#include <cinttypes>

namespace top
{
    namespace xconsensus
    {
        xcsaccount_t::xcsaccount_t(xcsobject_t & parent_object,const std::string & account_addr)
            :xcscoreobj_t(*parent_object.get_context(),parent_object.get_thread_id(),(base::enum_xobject_type)enum_xconsensus_object_type_account,account_addr)
        {
            xinfo("xcsaccount_t::xcsaccount_t,create,this=%p,account=%s",this,account_addr.c_str());
        }

        xcsaccount_t::xcsaccount_t(base::xcontext_t & _context,const int32_t target_thread_id,const std::string & account_addr)
            :xcscoreobj_t(_context,target_thread_id,(base::enum_xobject_type)enum_xconsensus_object_type_account,account_addr)
        {
            xinfo("xcsaccount_t::xcsaccount_t,create,this=%p,account=%s",this,account_addr.c_str());
        }

        xcsaccount_t::~xcsaccount_t()
        {
            xinfo("xcsaccount_t::~xcsaccount_t,destroy,account=%s",get_address().c_str());
        };

        void*    xcsaccount_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(enum_xconsensus_object_type_account == _enum_xobject_type_)
                return this;

            return xcscoreobj_t::query_interface(_enum_xobject_type_);
        }

        //subclass need override this default behavior that create context of enum_xconsensus_context_type_xthbft
        xcscoreobj_t*  xcsaccount_t::create_engine_object()
        {
            return xcscoreobj_t::create_engine(*this,enum_xconsensus_pacemaker_type_clock_cert);
        }
        
        //return specific error code(enum_xconsensus_result_code) to let caller know reason
        int    xcsaccount_t::verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) //load and execute block at sanbox
        {
            if(NULL == proposal_block)
                return enum_xconsensus_error_bad_proposal;
            
            //xbft has enhanced block check and U.S will do addtional check,so here just pass to improve performance
            return enum_xconsensus_code_successful;
        }

        bool    xcsaccount_t::verify_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, std::string & result) {
            return true;
        }

        void    xcsaccount_t::add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result) {
        }

        bool    xcsaccount_t::proc_vote_complate(base::xvblock_t * proposal_block) {
            return true;
        }

        bool    xcsaccount_t::verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data) {
            return true;
        }

        //clock block always pass by higher layer to lower layer
        bool  xcsaccount_t::on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            // xcsobject_t * child_obj = (xcsobject_t*)get_child_node();
            // if(child_obj == NULL) //create context assocated with as default
            // {
            //     base::xauto_ptr<xcsobject_t> ptr_engine_obj(create_engine_object());
            // }
            // base::xauto_ptr<base::xvblock_t>  highest_block(get_vblockstore()->get_latest_cert_block(*this, metrics::blockstore_access_from_bft_on_clock_fire));
            // if(highest_block)
            // {
            //     xinfo("xcsaccount_t::on_clock_fire block=%s", highest_block->dump().c_str());
            //     xcsclock_fire * _clock_event = (xcsclock_fire*)&event;
            //     _clock_event->reset_latest_block(highest_block()); //carry latest cert block from blockstore ,so that it may sync to xbft
            // } else {
            //     xerror("xcsaccount_t::on_clock_fire null account=%s", get_account().c_str());
            // }
            return false; //contiuse let event go down
        }

        bool   xcsaccount_t::fire_clock(base::xvblock_t & latest_clock_block,int32_t cur_thread_id,uint64_t timenow_ms)
        {
             if(get_child_node() != NULL)
             {
                base::xauto_ptr<xcsclock_fire>_event_obj(new xcsclock_fire(latest_clock_block));
                base::xauto_ptr<base::xvblock_t>  highest_block(get_vblockstore()->get_latest_cert_block(*this, metrics::blockstore_access_from_bft_on_clock_fire));
                if(highest_block)
                {
                    xdbg("xcsaccount_t::fire_clock clock=%s,cert=%s", latest_clock_block.dump().c_str(), highest_block->dump().c_str());
                    _event_obj->reset_latest_block(highest_block.get()); //carry latest cert block from blockstore ,so that it may sync to xbft
                }
                return get_child_node()->push_event_down(*_event_obj, this, cur_thread_id, timenow_ms);
             } else {
                xerror("xcsaccount_t::fire_clock null child_node");
             }
             return false;            
        }

        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xcsaccount_t::on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus::xcspdu_fire * _evt_obj = (xconsensus::xcspdu_fire*)&event;
            if(   /*(_evt_obj->_packet.get_block_chainid() != (uint32_t)get_chainid())
               || */(_evt_obj->_packet.get_block_account() != get_account()) ) //ensure only handle the packet for this account
            {
                xwarn("xcsaccount_t::on_pdu_event_down,receive pdu from other account=%s vs this=%s,at node=%llu",_evt_obj->_packet.get_block_account().c_str(),get_address().c_str(),get_xip2_low_addr());
                return true;
            }
            if(is_xip2_equal(get_xip2_addr(), _evt_obj->get_to_xip()) == false)//address verification
            {
                xwarn("xcsaccount_t::on_pdu_event_down,wrong dest address(%llu) at node=%llu",get_xip2_addr().low_addr,get_xip2_low_addr());
                _evt_obj->set_to_xip(get_xip2_addr());//correct target address to make sure it always be this address
            }
            if(   (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal) //replica get proposal from leader
               || (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_commit) ) //try sync first for commit msg
            {
                if(get_child_node() != NULL)
                {

                    base::xblock_mptrs latest_list = get_vblockstore()->get_latest_blocks(*this, metrics::blockstore_access_from_bft_pdu_event_down);
                    base::xauto_ptr<base::xvblock_t> latest_clock = get_vblockstore()->get_latest_cert_block(get_xclock_account_address(), metrics::blockstore_access_from_bft_pdu_event_down);

                    base::xauto_ptr<xproposal_start>_event_proposal_start(new xproposal_start());
                    _event_proposal_start->set_latest_commit(latest_list.get_latest_committed_block());
                    _event_proposal_start->set_latest_lock(latest_list.get_latest_locked_block());
                    _event_proposal_start->set_latest_cert(latest_list.get_latest_cert_block());
                    _event_proposal_start->set_latest_clock(latest_clock.get());
                    get_child_node()->push_event_down(*_event_proposal_start, this, get_thread_id(), get_time_now());
                     //try best to let context/driver know what is latest commit block
                }
            }
            return false;
        }

        //call from higher layer to lower layer(child)
        bool  xcsaccount_t::on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xproposal_start * _evt_obj = (xproposal_start*)&event;
            if(_evt_obj->get_proposal()->get_prev_block() == nullptr)
            {
                xerror("xcsaccount_t::on_proposal_start,carry empty parent blocks of proposal for leader=%" PRIx64 ",at node=%" PRIx64,_evt_obj->get_proposal()->get_cert()->get_validator().low_addr,get_xip2_low_addr());
                return true;
            }
            //xdbgassert(check_proposal(_evt_obj->get_proposal()));
            base::xblock_mptrs latest_list = get_vblockstore()->get_latest_blocks(*this, metrics::blockstore_access_from_bft_consaccnt_on_proposal_start);
            if(NULL == _evt_obj->get_latest_commit())
                _evt_obj->set_latest_commit(latest_list.get_latest_committed_block());
            if(NULL == _evt_obj->get_latest_lock())
                _evt_obj->set_latest_lock(latest_list.get_latest_locked_block());
            if(NULL == _evt_obj->get_latest_cert())
                _evt_obj->set_latest_cert(latest_list.get_latest_cert_block());

            if(NULL == _evt_obj->get_latest_clock())//auto fill latest clock
            {
                base::xauto_ptr<base::xvblock_t> latest_clock = get_vblockstore()->get_latest_cert_block(get_xclock_account_address(), metrics::blockstore_access_from_bft_consaccnt_on_proposal_start);
                _evt_obj->set_latest_clock(latest_clock.get());
            }

            // get leader xip
            auto leader_xip = _evt_obj->get_proposal()->get_cert()->get_validator();
            if (get_node_id_from_xip2(leader_xip) == 0x3FF) {
                leader_xip = _evt_obj->get_proposal()->get_cert()->get_auditor();
            }
            if(false == is_xip2_equal(leader_xip, get_xip2_addr()))
            {
                 xwarn_err("xcsaccount_t::on_proposal_start,wrong cert'validator address(%" PRIx64 ") node=%" PRIx64,_evt_obj->get_proposal()->get_cert()->get_validator().low_addr,get_xip2_addr());

                //do protection,force to reset it at release build
                _evt_obj->get_proposal()->get_cert()->set_validator(get_xip2_addr());
            }
            return false; //contiuse let event go down
        }

        //call from lower layer to higher layer(parent)
        bool  xcsaccount_t::on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xproposal_finish * _evt_obj = (xproposal_finish*)&event;
            if(_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful)
            {
                base::xvblock_t* _latest_lock_block   = _evt_obj->get_latest_lock();
                base::xvblock_t* _target_cert_block   = _evt_obj->get_target_proposal();

                std::vector<base::xvblock_t*> _batch_blocks;
                _batch_blocks.resize(2);
                _batch_blocks[0] = _target_cert_block;
                _batch_blocks[1] = _latest_lock_block;
                //stored larger height block first for commit prove
                //get_vblockstore()->store_blocks(*this,_batch_blocks);//save to blockstore
                XMETRICS_TIME_RECORD("cons_store_block_cost");
                get_vblockstore()->store_block(*this,_target_cert_block); //just store cert only
                xdbg("xcsaccount_t::on_proposal_finish _target_cert_block:%s", _target_cert_block->dump().c_str());
            }
            return false;//throw event up again to let txs-pool or other object start new consensus
        }

        //call from lower layer to higher layer(parent)
        bool  xcsaccount_t::on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus_commit * _evt_obj = (xconsensus_commit*)&event;

            base::xvblock_t* _latest_lock_block  = _evt_obj->get_latest_lock();
            base::xvblock_t* _latest_cert_block  = _evt_obj->get_latest_cert();
            
            std::vector<base::xvblock_t*> _batch_blocks;
            _batch_blocks.resize(3);
            _batch_blocks[0] = _latest_cert_block;
            _batch_blocks[1] = _latest_lock_block;
            _batch_blocks[2] = _evt_obj->get_target_commit();

            //stored larger height block first for commit prove
            //get_vblockstore()->store_blocks(*this,_batch_blocks);//save to blockstore
            xdbg("xcsaccount_t::on_consensus_commit commit block:%s", _evt_obj->get_target_commit()->dump().c_str());
            get_vblockstore()->store_block(*this,_evt_obj->get_target_commit()); //sore commit only
            return false;//throw event up again to let txs-pool or other object start new consensus
        }

        bool  xcsaccount_t::on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus_update * _evt_obj = (xconsensus_update*)&event;

            base::xvblock_t* _latest_commit_block = _evt_obj->get_latest_commit();
            base::xvblock_t* _latest_lock_block   = _evt_obj->get_latest_lock();
            base::xvblock_t* _latest_cert_block   = _evt_obj->get_latest_cert();

            std::vector<base::xvblock_t*> _batch_blocks;
            _batch_blocks.resize(3);
            _batch_blocks[0] = _latest_cert_block;
            _batch_blocks[1] = _latest_lock_block;
            _batch_blocks[2] = _latest_commit_block;
            //stored larger height block first for commit prove
            xdbg("xcsaccount_t::on_consensus_update cert:%s,lock:%s,commit:%s", _latest_cert_block->dump().c_str(), _latest_lock_block->dump().c_str(), _latest_commit_block->dump().c_str());
            get_vblockstore()->store_blocks(*this,_batch_blocks);//save to blockstore
            return true;//stop handle anymore
        }

        bool  xcsaccount_t::on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //call from lower layer to higher layer(parent)
        {
            xreplicate_finish * _evt_obj = (xreplicate_finish*)&event;
            if(_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful)
            {
                base::xvblock_t* _latest_lock_block   = _evt_obj->get_latest_lock();
                base::xvblock_t* _target_cert_block   = _evt_obj->get_target_block();

                std::vector<base::xvblock_t*> _batch_blocks;
                _batch_blocks.resize(2);
                _batch_blocks[0] = _target_cert_block;
                _batch_blocks[1] = _latest_lock_block;
                //stored larger height block first for commit prove
                //get_vblockstore()->store_blocks(*this,_batch_blocks);//save to blockstore
                xdbg("xcsaccount_t::on_replicate_finish _target_cert_block:%s", _target_cert_block->dump().c_str());
                get_vblockstore()->store_block(*this,_target_cert_block);
            }
            return true; //stop handle anymore
        }

        bool    xcsaccount_t::store_commit_block(base::xvblock_t* _commit_block)
        {
            if(NULL == _commit_block)
                return false;

            #ifdef DEBUG
            if(   (_commit_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               && (_commit_block->check_block_flag(base::enum_xvblock_flag_committed))
               && (_commit_block->is_deliver(false))//double check to has pass siganture verification
               )
            #else //release version
            if(   (_commit_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               && (_commit_block->check_block_flag(base::enum_xvblock_flag_committed))
              )
            #endif
            {
                get_vblockstore()->store_block(*this,_commit_block);//save to blockstore
                return true;
            }
            xassert(0);
            return false;
        }

        bool    xcsaccount_t::store_lock_block(base::xvblock_t* _lock_block)
        {
            if(NULL == _lock_block)
                return false;

            #ifdef DEBUG
            if(   (_lock_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               && (_lock_block->is_deliver(false))//double check to has pass siganture verification
               )
            #else //release version
            if(_lock_block->check_block_flag(base::enum_xvblock_flag_authenticated))
            #endif
            {
                if(_lock_block->check_block_flag(base::enum_xvblock_flag_locked))
                    get_vblockstore()->store_block(*this,_lock_block); //save to blockstore

                return true;
            }
            xassert(0);
            return false;
        }

        bool    xcsaccount_t::store_cert_block(base::xvblock_t* _cert_block)
        {
            if(NULL == _cert_block)
                return false;

            #ifdef DEBUG
            if(   (_cert_block->check_block_flag(base::enum_xvblock_flag_authenticated))
               && (_cert_block->is_deliver(false))//double check to has pass siganture verification
               )
            #else //release version
            if(_cert_block->check_block_flag(base::enum_xvblock_flag_authenticated))
            #endif
            {
                get_vblockstore()->store_block(*this,_cert_block); //save to blockstore
                return true;
            }
            xassert(0);
            return false;
        }
        
        bool xcsaccount_t::is_mailbox_over_limit(const int32_t max_mailbox_num) {
            int64_t in, out;
            int32_t queue_size = count_calls(in, out);
            bool discard = queue_size >= max_mailbox_num;
            if (discard) {
                xwarn("xcsaccount_t::is_mailbox_over_limit in=%lld,out=%lld,queue:%d,max_mailbox_num:%d", in, out, queue_size, max_mailbox_num);
                return true;
            }
            return false;
        }

    };//end of namespace of xconsensus

};//end of namespace of top
