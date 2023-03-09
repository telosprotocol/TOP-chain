// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>

#include "xclockcertview.h"
#include "xtimercertview.h"
#include <cstdlib>
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace xconsensus
    {
        //////////////////////////////////////////xcspacemaker_t////////////////////////////////////////////////
        //wrap function to create xcspacemaker_t, and attach into parent_object
        xcspacemaker_t *  xcspacemaker_t::create_pacemaker_object(xcscoreobj_t&  parent_object,enum_xconsensus_pacemaker_type type)
        {
            xcspacemaker_t * new_pacemaker_ptr = NULL;
            if(type == enum_xconsensus_pacemaker_type_clock_cert)
                new_pacemaker_ptr = new xclockcert_view(parent_object);
            else if(type == enum_xconsensus_pacemaker_type_timeout_cert)
                new_pacemaker_ptr = new xconspacemaker_t(parent_object);

            if(new_pacemaker_ptr != NULL)
            {
                xvip2_t alloc_address;
                alloc_address.high_addr = 0;
                alloc_address.low_addr = 0;
                parent_object.attach_child_node(new_pacemaker_ptr,alloc_address,std::string());//attach_child_node may include additonal reference

                return new_pacemaker_ptr;
            }
            xerror("xcspacemaker_t::create_pacemaker_object,fail-critical bug: not found match type:%d",type); //nothing catch
            return new_pacemaker_ptr;
        }

        //////////////////////////////////////////xclockcert_view////////////////////////////////////////////////
        xclockcert_view::xclockcert_view(xcscoreobj_t&  parent_object)
            :xcspacemaker_t(parent_object)
        {
            xinfo("xclockcert_view::xclockcert_view,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
            m_latest_view_id       = 0;
            m_latest_clock_cert    = NULL;
            m_latest_vblock_cert   = NULL;
        }

        xclockcert_view::~xclockcert_view()
        {
            xinfo("xclockcert_view::~xclockcert_view,,destroy,this=%p",this);
            if(m_latest_clock_cert != NULL)
                m_latest_clock_cert->release_ref();

            if(m_latest_vblock_cert != NULL)
                m_latest_vblock_cert->release_ref();
        }

        bool  xclockcert_view::init_vblock(const std::string &account)
        {
            if( (NULL == m_latest_clock_cert) || (NULL == m_latest_vblock_cert) )
            {
                if(get_vblockstore() != NULL)
                {
                    #ifdef __ENABLE_PRE_INIT_CLOCK_CERT__
                    if((NULL == m_latest_clock_cert))
                    {
                        base::xauto_ptr<base::xvblock_t> latest_clock = get_vblockstore()->get_latest_cert_block(get_xclock_account_address(), metrics::blockstore_access_from_bft_init_blk);
                        if( (latest_clock) && (latest_clock->is_deliver(false)))
                        {
                            m_latest_clock_cert = latest_clock->get_cert();
                            m_latest_clock_cert->add_ref();
                        }
                    }
                    #endif
                    if(NULL == m_latest_vblock_cert)
                    {
                        base::xauto_ptr<base::xvblock_t> latest_vblock(get_vblockstore()->get_latest_cert_block(account, metrics::blockstore_access_from_bft_init_blk));
                        if( (latest_vblock) && (latest_vblock->is_deliver(false)))
                        {
                            xinfo("xclockcert_view::init_vblock %s",latest_vblock->dump().c_str());
                            m_latest_vblock_cert = latest_vblock->get_cert();
                            m_latest_vblock_cert->add_ref();
                        }
                    }
                    update_view();
                }
            }
            return true;
        }

        const uint64_t xclockcert_view::get_latest_viewid(const std::string &account)
        {
            if(0 == m_latest_view_id) //not initialized yet
            {
                init_vblock(account);
            }
            return m_latest_view_id;
        }

        const uint64_t xclockcert_view::get_latest_xclock_height()
        {
            if(NULL == m_latest_clock_cert)
                return 0;

            return m_latest_clock_cert->get_clock();
        }

        bool  xclockcert_view::safe_check_clock_cert(base::xvqcert_t * clock_cert)
        {
            if( (NULL  == clock_cert) || (clock_cert == m_latest_clock_cert) )
                return false;

            if(m_latest_clock_cert != NULL)
            {
                if(   (m_latest_clock_cert->get_clock()  >= clock_cert->get_clock())
                   || (m_latest_clock_cert->get_viewid() >= clock_cert->get_viewid())
                   )
                {
                    return false;
                }
            }
            if(false == clock_cert->is_deliver()) //deliver for clock
            {
                xdbg("xclockcert_view::safe_check_clock_cert is_deliver false TC:%llu", clock_cert->get_clock());
                return false;
            }
            return true;
        }

        bool  xclockcert_view::update_clock_cert(base::xvblock_t * clock_block)
        {
            if(NULL == clock_block)
                return false;

            //quickly filter
            if(m_latest_clock_cert != NULL)
            {
                if(   (m_latest_clock_cert->get_clock()  >= clock_block->get_clock())
                   || (m_latest_clock_cert->get_viewid() >= clock_block->get_viewid())
                   )
                {
                    return false;
                }
            }
            #ifdef DEBUG//then do more check, actually safe_check_clock_cert may do similar check again
            if(false == clock_block->is_deliver(false))
                return false;
            #else
            if(clock_block->get_account() != get_xclock_account_address()) //ensure it is from xclock account
            {
                xerror("xclockcert_view::update_clock_cert,receive bad xclock block=%s",clock_block->dump().c_str());
                return false;
            }
            if(false == clock_block->check_block_flag(base::enum_xvblock_flag_authenticated)) //at least has verificated
                return false;
            #endif

            return update_clock_cert(clock_block->get_cert());
        }

        bool  xclockcert_view::update_clock_cert(base::xvqcert_t * clock_cert)
        {
            if(safe_check_clock_cert(clock_cert))
            {
                if(NULL == m_latest_clock_cert)
                    xdbg("xclockcert_view::update_clock_cert,nil-clock -> new_clock=%s,at node=0x%llx",clock_cert->dump().c_str(),get_xip2_addr().low_addr);
                //else
                //    xdbg("xclockcert_view::update_clock_cert,old_clock=%s --> new_clock=%s,at node=0x%llx",m_latest_clock_cert->dump().c_str(),clock_cert->dump().c_str(),get_xip2_addr().low_addr);

                clock_cert->add_ref();
                base::xvqcert_t * old_ptr = base::xatomic_t::xexchange(m_latest_clock_cert, clock_cert);
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                //now recalculate viewid
                update_view();
                return true;
            }
            return false;
        }

        const uint64_t  xclockcert_view::get_latest_vblock_viewid()
        {
            if(NULL == m_latest_vblock_cert)
                return 0;

            return m_latest_vblock_cert->get_viewid();
        }

        bool  xclockcert_view::safe_check_vblock_cert(base::xvqcert_t * hqc_cert)
        {
            if( (NULL  == hqc_cert) || (hqc_cert == m_latest_vblock_cert) )
                return false;

            if(m_latest_vblock_cert != NULL)
            {
                if(m_latest_vblock_cert->get_viewid() >= hqc_cert->get_viewid())
                {
                    return false;
                }
            }
            if(false == hqc_cert->is_deliver()) //deliver for block
            {
                return false;
            }
            return true;
        }

        bool    xclockcert_view::update_vblock_cert(base::xvblock_t * hqc_block)
        {
            if(NULL == hqc_block)
                return false;

            init_vblock(hqc_block->get_account());//check whether need initialize first

            //quickly filter
            if( (m_latest_vblock_cert != NULL) && (m_latest_vblock_cert->get_viewid() >= hqc_block->get_viewid()))
                return false;

            //then do more check
            if(false == hqc_block->is_deliver(false))
                return false;

            xinfo("xclockcert_view::update_vblock_cert %s",hqc_block->dump().c_str());
            return update_vblock_cert(hqc_block->get_cert());
        }

        bool    xclockcert_view::update_vblock_cert(base::xvqcert_t * hqc_cert)
        {
            if(safe_check_vblock_cert(hqc_cert))
            {
                if(NULL == m_latest_vblock_cert)
                    xdbg("xclockcert_view::update_vblock_cert,nil-block-cert -> new_block_cert=%s,at node=0x%llx",hqc_cert->dump().c_str(),get_xip2_addr().low_addr);
                else
                    xdbg("xclockcert_view::update_vblock_cert,old_block_cert=%s --> new_block_cert=%s,at node=0x%llx",m_latest_vblock_cert->dump().c_str(),hqc_cert->dump().c_str(),get_xip2_addr().low_addr);

                hqc_cert->add_ref();
                base::xvqcert_t * old_ptr = base::xatomic_t::xexchange(m_latest_vblock_cert, hqc_cert);
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                //now recalculate viewid
                update_view();
                return true;
            }
            return false;
        }

        bool    xclockcert_view::update_view()
        {
            if( (NULL == m_latest_vblock_cert) || (NULL == m_latest_clock_cert) )
                return false;

            const uint64_t clock_height_from_latest_commit = m_latest_vblock_cert->get_clock();
            const uint64_t clock_height_from_latest_clock  = m_latest_clock_cert->get_clock();

            uint64_t new_view_id = 0;
            if(clock_height_from_latest_clock > clock_height_from_latest_commit)
            {
                new_view_id = m_latest_vblock_cert->get_viewid() + 1 + (clock_height_from_latest_clock - clock_height_from_latest_commit) / 3; //align to 3
            }
            else//clock is behind the commit block, clock-block shoul have been on the way ,or just sync from other peers
            {
                new_view_id = m_latest_vblock_cert->get_viewid() + 1;
            }
            if(new_view_id > m_latest_view_id)
            {
                xkinfo("xclockcert_view::update_view,tps_key at node=0x%llx, account %s old viewid=%llu --> new_view_id=%llu =  cert'view=%llu + 1 + {clock:%llu - %llu}",
                       get_xip2_addr().low_addr,
                       get_address().c_str(),
                       m_latest_view_id,
                       new_view_id,
                       m_latest_vblock_cert->get_viewid(),
                       clock_height_from_latest_clock,
                       clock_height_from_latest_commit);
                base::xatomic_t::xexchange(m_latest_view_id, new_view_id);
                //fire view chagne event

                //send_call may automatically hold this reference whiling executing,so here is safe to using "this"
                std::function<void(void*)> _aysn_update_view = [this,new_view_id,clock_height_from_latest_clock](void*)->void{
                    fire_view(get_account(), new_view_id, clock_height_from_latest_clock, get_thread_id(), get_time_now());
                };
                // send_call(_aysn_update_view,(void*)NULL);
                dispatch_call(_aysn_update_view,(void*)NULL);
                return true;
            }
            return false;
        }

        //clock block always pass by higher layer to lower layer
        bool  xclockcert_view::on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xcsclock_fire * _clock_event = (xcsclock_fire*)&event;

            xdbg("xclockcert_view::on_clock_fire,recv new clock=%s at node=0x%llx",_clock_event->get_clock_block()->dump().c_str(),get_xip2_addr().low_addr);

            update_clock_cert(_clock_event->get_clock_block());
            if(_clock_event->get_latest_block() != NULL) {
                update_vblock_cert(_clock_event->get_latest_block());
            } else {
                xerror("xclockcert_view::on_clock_fire null");
            }
            
            return false;//let lower layer continue get notified
        }

        //note: to return false may call parent'push_event_up,or stop further routing when return true
        bool  xclockcert_view::on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            //m_latest_clock_block must be valid befor start
            xassert(m_latest_clock_cert != NULL);
            if(NULL == m_latest_clock_cert)
                return true; //stop

            xcspdu_fire * _evt_obj = (xcspdu_fire*)&event;
            if ((_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal) || (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal_v2) ||
                (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_commit)) {
                if(_evt_obj->_packet.get_xclock_cert().empty())
                {
                    std::string latest_block_cert;
                    m_latest_clock_cert->serialize_to_string(latest_block_cert);
                    _evt_obj->_packet.set_xclock_cert(latest_block_cert);//attach block certification for proposal
                }
            }
            if(_evt_obj->_packet.get_block_clock() == 0)
            {
                _evt_obj->_packet.set_block_clock(m_latest_clock_cert->get_clock());
            }
            return false;//let upper layer continue get notified
        }

        bool xclockcert_view::fire_verify_pduevent_job(xcspdu_fire * _pdu_event)
        {
            if(NULL == _pdu_event)
                return false;

            //now safe to do heavy job to verify quorum_ceritification completely
            std::function<void(void*)> _after_verify_cert_job = [this](void* _event)->void{
                xcspdu_fire * _evt_obj  = (xcspdu_fire*)_event;

                base::xvqcert_t*   _packet_xclock_cert = _evt_obj->get_xclock_cert();
                if( (_packet_xclock_cert != NULL) && (_packet_xclock_cert->check_unit_flag(base::enum_xvblock_flag_authenticated)) )
                    update_clock_cert(_packet_xclock_cert);

                base::xvqcert_t*   _packet_vblock_cert = _evt_obj->get_vblock_cert();
                if( (_packet_vblock_cert != NULL) && (_packet_vblock_cert->check_unit_flag(base::enum_xvblock_flag_authenticated)) )
                    update_vblock_cert(_packet_vblock_cert);

                if(get_child_node() != NULL) //next layer still there
                {
                    _evt_obj->set_cookie(get_latest_viewid());//carry latest viewid
                    if(_evt_obj->_packet.get_block_viewid() == get_latest_viewid()) //check again
                    {
                        xdbg("xclockcert_view::_after_verify_cert_job,final view# aligned by clock or hqc cert for packet=%s,at node=0x%llx",_evt_obj->_packet.dump().c_str(),get_xip2_low_addr());
                        get_child_node()->push_event_down(*_evt_obj,this,0,0);//alignment of clock and viewid now
                    }
                    else
                    {
                        //note:to enahance sync function, we need let proposal pass first and then check view alignment at driver layer again
                        if(_evt_obj->_packet.get_msg_type() != enum_consensus_msg_type_vote)
                        {
                            //xinfo("xclockcert_view::_after_verify_cert_job,allow un-alignment-packet=%s,even expect viewid:%llu,at node=0x%llx",_evt_obj->_packet.dump().c_str(),get_latest_viewid(),get_xip2_low_addr());
                            get_child_node()->push_event_down(*_evt_obj,this,0,0);////view just need control proposal and vote messages
                        }
                        xwarn("xclockcert_view::_after_verify_cert_job,NOT-alignment-packet=%s,since expect viewid:%llu,at node=0x%llx",_evt_obj->_packet.dump().c_str(),get_latest_viewid(),get_xip2_low_addr());
                    }
                }
                //TODO,send newer clock cert  to peer for optimization
                _evt_obj->release_ref(); //release refernce added by _verify_function;
            };

            auto _verify_function = [this](base::xcall_t & call, const int32_t cur_thread_id,const uint64_t timenow_ms)->bool{
                if(is_close() == false)
                {
                    xcspdu_fire * _evt_obj  = (xcspdu_fire*)call.get_param1().get_object();
                    const xvip2_t & to_addr = _evt_obj->get_to_xip();

                    base::xvqcert_t*   _packet_xclock_cert = _evt_obj->get_xclock_cert();
                    if(NULL != _packet_xclock_cert)
                    {
                        const std::string xclock_account_addrs = xcsobject_t::get_xclock_account_address();
                        _packet_xclock_cert->reset_unit_flag(base::enum_xvblock_flag_authenticated);//remove first
                        XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_xbft, 1);
                        if(get_vcertauth()->verify_muti_sign(_packet_xclock_cert,xclock_account_addrs) == base::enum_vcert_auth_result::enum_successful)
                            _packet_xclock_cert->set_unit_flag(base::enum_xvblock_flag_authenticated);
                        else
                            xwarn("xclockcert_view::fire_verify_pduevent_job,bad xclock cert=%s,at node=0x%llx",_packet_xclock_cert->dump().c_str(),to_addr.low_addr);
                    }
                    base::xvqcert_t*   _packet_vblock_cert = _evt_obj->get_vblock_cert();
                    if(NULL != _packet_vblock_cert)
                    {
                        _packet_vblock_cert->reset_unit_flag(base::enum_xvblock_flag_authenticated);//remove first
                        XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_xbft, 1);
                        if(get_vcertauth()->verify_muti_sign(_packet_vblock_cert,get_account()) == base::enum_vcert_auth_result::enum_successful)
                            _packet_vblock_cert->set_unit_flag(base::enum_xvblock_flag_authenticated);
                        else
                            xwarn("xclockcert_view::fire_verify_pduevent_job,bad block-qc cert=%s,at node=0x%llx",_packet_vblock_cert->dump().c_str(),to_addr.low_addr);
                    }
                    base::xfunction_t* _callback_ = (base::xfunction_t *)call.get_param2().get_function();
                    if(_callback_ != NULL)
                    {
                        _evt_obj->add_ref(); //hold addtional reference for next aync call
                        dispatch_call(*_callback_,(void*)_evt_obj);
                    }
                }
                return true;
            };
            base::xcall_t asyn_verify_call(_verify_function,(base::xobject_t*)_pdu_event,&_after_verify_cert_job,(base::xobject_t*)this);
            asyn_verify_call.bind_taskid(get_account_index());
            base::xworkerpool_t * _workers_pool = get_workerpool();
            if(_workers_pool != NULL)
                return (_workers_pool->send_call(asyn_verify_call) == enum_xcode_successful);
            else
                return (dispatch_call(asyn_verify_call) == enum_xcode_successful);
        }

        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xclockcert_view::on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            const xvip2_t & from_addr = event.get_from_xip();
            const xvip2_t & to_addr   = event.get_to_xip();
            xcspdu_fire * _evt_obj  = (xcspdu_fire*)&event;
            base::xcspdu_t & packet = _evt_obj->_packet;

            if(is_xip2_equal(from_addr, to_addr))
            {
                xwarn("xclockcert_view::on_pdu_event_down,a loopback' packet=%s at node=0x%llx",_evt_obj->_packet.dump().c_str(),get_xip2_low_addr());
                return true;
            }
            _evt_obj->set_clock(get_latest_xclock_height());
            _evt_obj->set_cookie(get_latest_viewid(packet.get_block_account()));//carry latest viewid

            const std::string & latest_xclock_cert_bin = _evt_obj->_packet.get_xclock_cert();
            if(latest_xclock_cert_bin.empty() == false) //try to update clock cert
            {
                base::xauto_ptr<base::xvqcert_t> _clock_cert_obj(base::xvblock_t::create_qcert_object(latest_xclock_cert_bin));
                if(_clock_cert_obj)
                {
                    //note:#1 safe rule, always cleans up flags carried by peer
                    _clock_cert_obj->reset_block_flags(); //force remove
                    if(_clock_cert_obj->is_deliver())
                        _evt_obj->set_xclock_cert(_clock_cert_obj.get());
                    else
                        xwarn_err("xclockcert_view::on_pdu_event_down,fail-carry a bad clock cert for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());
                }
                else
                {
                    xwarn_err("xclockcert_view::on_pdu_event_down,fail-carry a unrecognized clock cert for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr()); //possible attack
                }
            }
            if(_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal || _evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal_v2)
            {
                if(_evt_obj->get_xclock_cert() == NULL) //proposal now force to carry the bind clock cert
                {
                    xerror("xclockcert_view::on_pdu_event_down,fail-carry a unrecognized clock cert for proposal msg from packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr()); //possible attack
                    return true;
                }
            }

            if(packet.get_block_viewid() != get_latest_viewid(packet.get_block_account()))
            {
                //only do cert verification for view and proposal at pacemaker layer, since this node will not be leader after updated view by this cert
                if ((_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_view) || (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal) ||
                    (_evt_obj->_packet.get_msg_type() == enum_consensus_msg_type_proposal_v2)) {
                    const std::string & latest_vblock_cert_bin = _evt_obj->_packet.get_vblock_cert();
                    if(latest_vblock_cert_bin.empty() == false) //try to update hqc cert
                    {
                        base::xauto_ptr<base::xvqcert_t>_vblock_cert_obj(base::xvblock_t::create_qcert_object(latest_vblock_cert_bin));
                        if(_vblock_cert_obj)
                        {
                            //note:#1 safe rule, always cleans up flags carried by peer
                            _vblock_cert_obj->reset_block_flags(); //force remove
                            if(_vblock_cert_obj->get_viewid() > get_latest_vblock_viewid())
                            {
                                if(_vblock_cert_obj->is_deliver())
                                    _evt_obj->set_vblock_cert(_vblock_cert_obj.get());
                                else
                                    xwarn_err("xclockcert_view::on_pdu_event_down,fail-carry a bad hqc cert for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());//possible attack
                            }
                        }
                        else
                        {
                            xwarn_err("xclockcert_view::on_pdu_event_down,fail-carry a unrecognized hqc cert for packet=%s,at node=0x%llx",packet.dump().c_str(),get_xip2_low_addr());//possible attack
                        }
                    }
                }

                if( (_evt_obj->get_xclock_cert() != NULL) || (_evt_obj->get_vblock_cert() != NULL) )
                {
                    xdbg("xclockcert_view::on_pdu_event_down,luanch fire_verify_pduevent_job,at node=0x%llx",get_xip2_low_addr());
                    fire_verify_pduevent_job(_evt_obj); //go another worker'thread to verify signaturen,and callback to engine'thread
                    return true; //stop here since fire_verify_pduevent_job may push event again
                }
                else //dont found any valid cert to update view
                {
                    //note:to enahance sync function, we need let proposal pass first and then check view alignment at driver layer again
                    if(_evt_obj->_packet.get_msg_type() != enum_consensus_msg_type_vote)//stop vote at pacemaker level for unlignment view
                    {
                        //xdbg("xclockcert_view::on_pdu_event_down,allow un-alignment-packet=%s,even expect viewid:%llu,at node=0x%llx",packet.dump().c_str(),get_latest_viewid(),get_xip2_low_addr());
                        return false;//view just need control proposal and vote messages
                    }
                    xwarn("xclockcert_view::on_pdu_event_down,NOT-alignment-packet=%s,but expect viewid:%llu,at node=0x%llx",packet.dump().c_str(),get_latest_viewid(),get_xip2_low_addr());
                    return true;//restore to restrict mode for vote that must alignwith viewid
                }
            }
            return false;//let lower layer continue get notified
        }

        //return specific error code(enum_xconsensus_result_code) to let caller know reason
        int   xclockcert_view::verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) //load and execute block at sanbox
        {
            //verify verifiable_nonce first
            const uint64_t verifyable_nonce = base::xhash64_t::digest(proposal_block->get_last_block_hash()) + base::xhash64_t::digest(bind_clock_cert->build_block_hash());
            if(verifyable_nonce != proposal_block->get_cert()->get_nonce())
            {
                xerror("xclockcert_view::verify_proposal,fail-verify nonce for proposal=%s with clock-cert=%s,at node=0x%llx",proposal_block->dump().c_str(),bind_clock_cert->dump().c_str(),get_xip2_low_addr());
                return enum_xconsensus_error_bad_clock_cert;
            }
            return xcspacemaker_t::verify_proposal(proposal_block,bind_clock_cert,_from_child);
        }

        //call from higher layer to lower layer(child)
        bool  xclockcert_view::on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xproposal_start* _evt_obj = (xproposal_start*)&event;
            update_clock_cert(_evt_obj->get_latest_clock());

            update_vblock_cert(_evt_obj->get_latest_cert());
            update_vblock_cert(_evt_obj->get_latest_commit());

            //update proposal with latest viewid
            if(_evt_obj->get_proposal() != NULL) //proposal from leader
            {
                if(_evt_obj->get_proposal()->get_prev_block() != nullptr)//one more try to update cert
                    update_vblock_cert(_evt_obj->get_proposal()->get_prev_block());

                uint64_t latest_viewid = get_latest_viewid(_evt_obj->get_proposal()->get_account());
                if (_evt_obj->get_proposal()->get_cert()->get_viewid() != latest_viewid) {
                    xwarn("xclockcert_view::on_proposal_start,fail-viewid changed,proposal=%s,latest_viewid=%" PRIu64 ", at node=0x%llx", _evt_obj->get_proposal()->dump().c_str(), latest_viewid, get_xip2_low_addr());
                    _evt_obj->get_proposal()->get_cert()->set_viewid(latest_viewid);  // set viewid to notify higher layer
                    return true; //not ready to start proposal,since viewid changed after on_view_fire
                }

                if(_evt_obj->get_proposal()->get_cert()->get_clock() == 0) //application not set,let make it up
                    _evt_obj->get_proposal()->get_cert()->set_clock(get_latest_xclock_height());

                //for leader,it must let clock ready first before start consensus
                if(0 == get_latest_xclock_height())//this pacemarker rely on clock block
                {
                    xwarn("xclockcert_view::on_proposal_start,fail-clock-block not received yet,just drop at node=0x%llx",get_xip2_low_addr());
                    return true; //not ready to start proposal,since there is no block received
                }
                //set verifiable_nonce here
                _evt_obj->set_clock_cert(m_latest_clock_cert); //carry the cert object
                const uint64_t verifyable_nonce = base::xhash64_t::digest(_evt_obj->get_proposal()->get_last_block_hash()) + base::xhash64_t::digest(_evt_obj->get_clock_cert()->build_block_hash());
                _evt_obj->get_proposal()->get_cert()->set_nonce(verifyable_nonce);
            }
            return false;//let lower layer continue get notified
        }

        //call from lower layer to higher layer(parent)
        bool  xclockcert_view::on_update_view(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xproposal_finish* _evt_obj = (xproposal_finish*)&event;
            //try all of them,and pick highest one
            if(_evt_obj->get_error_code() == enum_xconsensus_code_successful)
            {
                update_vblock_cert(_evt_obj->get_target_proposal());
            }
            update_vblock_cert(_evt_obj->get_latest_cert());
            update_vblock_cert(_evt_obj->get_latest_lock());
            update_vblock_cert(_evt_obj->get_latest_commit());

            return false; //let upper layer continue get notified
        }

        bool  xclockcert_view::on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)  //call from lower layer to higher layer(parent)
        {
            xreplicate_finish * _evt_obj = (xreplicate_finish*)&event;
            if(_evt_obj->get_error_code() == xconsensus::enum_xconsensus_code_successful)
            {
                base::xvblock_t* _target_cert_block   = _evt_obj->get_target_block();
                update_vblock_cert(_target_cert_block);
            }
            return false; //let upper layer continue get notified
        }

        //call from lower layer to higher layer(parent)
        bool  xclockcert_view::on_certificate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xcertificate_finish* _evt_obj = (xcertificate_finish*)&event;
            update_vblock_cert(_evt_obj->get_target_cert());
            return true; //certificate_finish is internal event, so stop throw up
        }

        //note: to return false may call parent'push_event_up,or stop further routing when return true
        bool  xclockcert_view::on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus_commit* _evt_obj = (xconsensus_commit*)&event;
            //try all of them,and pick highest one
            update_vblock_cert(_evt_obj->get_latest_cert());
            update_vblock_cert(_evt_obj->get_latest_lock());
            update_vblock_cert(_evt_obj->get_latest_commit());

            return false; //let upper layer continue get notified
        }

        //note: to return false may call child'push_event_down/up,or stop further routing when return true
        bool  xclockcert_view::on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xconsensus_update* _evt_obj = (xconsensus_update*)&event;
            //update clock
            update_clock_cert(_evt_obj->get_latest_clock());

            //try all of them,and pick highest one
            update_vblock_cert(_evt_obj->get_latest_cert());
            update_vblock_cert(_evt_obj->get_latest_lock());
            update_vblock_cert(_evt_obj->get_latest_commit());

            return false ; //let event contiusely up or down
        }

        //notify this node that is joined into parent-node
        bool  xclockcert_view::on_join_parent_node(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,const xvip2_t & alloc_address,const std::string & extra_data,xionode_t* from_parent)
        {
            xcspacemaker_t::on_join_parent_node(error_code,cur_thread_id,timenow_ms,alloc_address,extra_data,from_parent);

            //safe to init them,since it may trigger view-change event
            init_vblock(get_account());
            return true;
        }

        //////////////////////////////////////////xclockcert_view////////////////////////////////////////////////



    };//end of namespace of xconsensus
};//end of namespace of top
