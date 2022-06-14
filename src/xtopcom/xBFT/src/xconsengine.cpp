// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconsengine.h"

namespace top
{
    namespace xconsensus
    {
        //wrap function to create engines that include pacemaker,context,driver, and attach into parent_object
        xcscoreobj_t *  xcscoreobj_t::create_engine(xcscoreobj_t& parent_object,enum_xconsensus_pacemaker_type pacemaker_type)
        {
            xcspacemaker_t * _pacemaker_ptr = xcspacemaker_t::create_pacemaker_object(parent_object, pacemaker_type);
            if(_pacemaker_ptr != NULL)
            {
                base::xauto_ptr<xcscontext_t> _context_obj(xcscontext_t::create_context_object(*_pacemaker_ptr));
                if(_context_obj)
                {
                    base::xauto_ptr<xcsdriver_t>_driver_obj(xcsdriver_t::create_driver_object(*_context_obj));
                    if(_driver_obj)
                    {
                        return _pacemaker_ptr;
                    }
                }
            }
            xerror("xcscoreobj_t::create_engine,fail to create engine with pacemaker_type(%d)",pacemaker_type);
            if(_pacemaker_ptr != NULL)
                _pacemaker_ptr->release_ref();
            return NULL;
        }
        
        //////////////////////////////////////////xcspacemaker_t////////////////////////////////////////////////
        xcspacemaker_t::xcspacemaker_t(xcscoreobj_t&  parent_object)
        :xcscoreobj_t(parent_object,(base::enum_xobject_type)enum_xconsensus_object_type_pacemaker)
        {
            xinfo("xcspacemaker_t::xcspacemaker_t,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
 
        xcspacemaker_t::~xcspacemaker_t()
        {
            xinfo("xcspacemaker_t::~xcspacemaker_t,destroy,this=%p",this);
        }
        
        void*   xcspacemaker_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xconsensus_object_type_pacemaker)
                return this;
            
            return xcscoreobj_t::query_interface(_enum_xobject_type_);
        }

        //////////////////////////////////////////xcscontext_t////////////////////////////////////////////////
        xcscontext_t::xcscontext_t(xcscoreobj_t& parent_object)
        :xcscoreobj_t(parent_object,(base::enum_xobject_type)enum_xconsensus_object_type_context)
        {
            xinfo("xcscontext_t::xcscontext_t,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
        
        xcscontext_t::~xcscontext_t()
        {
            xinfo("xcscontext_t::~xcscontext_t,destroy,this=%p",this);
        }
        
        void*  xcscontext_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(enum_xconsensus_object_type_context == _enum_xobject_type_)
                return this;
            
            return xcscoreobj_t::query_interface(_enum_xobject_type_);
        }
        
        bool   xcscontext_t::on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            return false;
        }
        
        //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
        bool   xcscontext_t::on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode)
        {
            //double check whether childnode is driver
            xassert(childnode != NULL);
            void* child_driver_ptr = childnode->query_interface(enum_xconsensus_object_type_driver);
            xassert(child_driver_ptr != NULL);
            if(NULL == child_driver_ptr)
                return false;
            
            //do default handle
            return xcsobject_t::on_child_node_join(error_code,cur_thread_id,timenow_ms,childnode);
        }
        
        //////////////////////////////////////////xcsdriver_t////////////////////////////////////////////////
        xcsdriver_t::xcsdriver_t(xcscoreobj_t & parent_object)
        :xcscoreobj_t(parent_object,(base::enum_xobject_type)enum_xconsensus_object_type_driver)
        {
            xinfo("xcsdriver_t::xcsdriver_t,create,this=%p,parent=%p,account=%s",this,&parent_object,parent_object.get_account().c_str());
        }
        xcsdriver_t::~xcsdriver_t()
        {
            xinfo("xcsdriver_t::~xcsdriver_t,destroy,this=%p",this);
            
        }
        void*   xcsdriver_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(enum_xconsensus_object_type_driver == _enum_xobject_type_)
                return this;
            
            return xcscoreobj_t::query_interface(_enum_xobject_type_);
        }
        
        
        //////////////////////////////////////////xproposal_t////////////////////////////////////////////////
        xproposal_t::xproposal_t(base::xvblock_t & _block,const base::xvblock_t* parent_block)
        :base::xvbnode_t(NULL,_block)
        {
            m_voted_validators_count = 0;
            m_voted_auditors_count   = 0;
            m_highest_QC_viewid      = 0;
            m_is_pending= false;
            m_is_expired= false;
            m_is_certed = false;
            m_is_leader = false;
            m_is_voted  = false;
            m_allow_vote = 0; //as default for unknow status
            m_result_verify_proposal = enum_xconsensus_error_fail;
            m_proposal_from_addr.low_addr  = 0;
            m_proposal_from_addr.high_addr = 0;
            m_proposal_msg_nonce           = 0;
            
            m_expired_ms = -1;
            m_last_block_cert = NULL;
            m_bind_clock_cert = NULL;
            m_proposal_cert   = NULL;
            if(NULL != parent_block)
            {
                m_last_block_cert = parent_block->get_cert();
                m_last_block_cert->add_ref();
                //record qc viewid of prev cert
                m_highest_QC_viewid = m_last_block_cert->get_viewid();
            }
        }
        
        xproposal_t::xproposal_t(base::xvblock_t & _block,const base::xvqcert_t* parent_block_cert)
        :base::xvbnode_t(NULL,_block)
        {
            m_voted_validators_count = 0;
            m_voted_auditors_count   = 0;
            m_highest_QC_viewid      = 0;
            m_is_pending= false;
            m_is_expired= false;
            m_is_certed = false;
            m_is_leader = false;
            m_is_voted  = false;
            m_allow_vote = 0; //as default for unknow status
            m_result_verify_proposal = enum_xconsensus_error_fail;
            m_proposal_from_addr.low_addr  = 0;
            m_proposal_from_addr.high_addr = 0;
            m_proposal_msg_nonce           = 0;
            
            m_expired_ms = -1;
            m_last_block_cert = NULL;
            m_bind_clock_cert = NULL;
            m_proposal_cert   = NULL;
            if(NULL != parent_block_cert)
            {
                m_last_block_cert = (base::xvqcert_t*)parent_block_cert;
                m_last_block_cert->add_ref();
                //record qc viewid of prev cert
                m_highest_QC_viewid = m_last_block_cert->get_viewid();
            }
        }
        
        xproposal_t::xproposal_t(const xproposal_t & obj)
        :base::xvbnode_t(NULL,*obj.get_block())
        {
            m_last_block_cert = NULL;
            m_bind_clock_cert = NULL;
            m_proposal_cert   = NULL;

            m_voted_validators_count = (int32_t)obj.m_voted_validators_count;
            m_voted_auditors_count   = (int32_t)obj.m_voted_auditors_count;
            m_highest_QC_viewid      = obj.m_highest_QC_viewid;
            m_proposal_msg_nonce     = obj.m_proposal_msg_nonce;
            m_proposal_from_addr     = obj.m_proposal_from_addr;
            m_result_verify_proposal = obj.m_result_verify_proposal;
            m_is_pending      = obj.m_is_pending;
            m_is_expired      = obj.m_is_expired;
            m_is_certed       = obj.m_is_certed;
            m_is_leader       = obj.m_is_leader;
            m_is_voted        = obj.m_is_voted;
            m_allow_vote      = obj.m_allow_vote;
            m_expired_ms      = obj.m_expired_ms;
            m_last_block_cert = obj.m_last_block_cert;
            if(m_last_block_cert != NULL)
                m_last_block_cert->add_ref();
            
            m_bind_clock_cert = obj.m_bind_clock_cert;
            if(m_bind_clock_cert != NULL)
                m_bind_clock_cert->add_ref();
            
            m_proposal_cert = obj.m_proposal_cert;
            if(m_proposal_cert != NULL)
                m_proposal_cert->add_ref();
            
            m_voted_validators = obj.m_voted_validators;
            m_voted_auditors   = obj.m_voted_auditors;
            m_all_voted_cert   = obj.m_all_voted_cert;
        }
        
        xproposal_t::~xproposal_t()
        {
            if(m_last_block_cert != NULL)
                m_last_block_cert->release_ref();
            
            if(m_bind_clock_cert != NULL)
                m_bind_clock_cert->release_ref();
            
            if(m_proposal_cert != NULL)
                m_proposal_cert->release_ref();
            
            //xdbg("xproposal_t::destroy,dump=%s",dump().c_str());
        }
    
        bool  xproposal_t::set_highest_QC_viewid(const uint64_t new_viewid)
        {
            if(new_viewid > m_highest_QC_viewid)
            {
                base::xatomic_t::xstore(m_highest_QC_viewid, new_viewid);
                return true;
            }
            return false;
        }
    
        const uint64_t   xproposal_t::get_highest_QC_viewid() const
        {
            return m_highest_QC_viewid;
        }
    
        void   xproposal_t::set_proposal_cert(base::xvqcert_t* new_proposal_cert)
        {
            if(new_proposal_cert != NULL)
                new_proposal_cert->add_ref();
            
            base::xvqcert_t* old_one = m_proposal_cert;
            m_proposal_cert  = new_proposal_cert;
            if(old_one != NULL)
                old_one->release_ref();
        }
        
        void   xproposal_t::set_bind_clock_cert(base::xvqcert_t* new_clock_cert)
        {
            if(new_clock_cert != NULL)
                new_clock_cert->add_ref();
            
            base::xvqcert_t* old_one = m_bind_clock_cert;
            m_bind_clock_cert = new_clock_cert;
            if(old_one != NULL)
                old_one->release_ref();
        }
        
        #ifdef DEBUG //tracking memory of proposal block
        int32_t   xproposal_t::add_ref()
        {
            return base_class::add_ref();
        }
        int32_t   xproposal_t::release_ref()
        {
            return base_class::release_ref();
        }
        #endif
        
        bool   xproposal_t::is_valid_packet(base::xcspdu_t & packet)
        {
            base::xvblock_t * _raw_block = get_block();
            if(   (_raw_block->get_height()     != packet.get_block_height())
               || (_raw_block->get_chainid()    != packet.get_block_chainid())
               || (_raw_block->get_viewid()     != packet.get_block_viewid())
               || (_raw_block->get_viewtoken()  != packet.get_block_viewtoken())
               || (_raw_block->get_account()    != packet.get_block_account())
               )
            {
                return false;
            }
            return true;
        }
        bool    xproposal_t::is_validators_finish_vote() const
        {
            if(m_voted_validators_count >= (int32_t) get_cert()->get_validator_threshold())
                return true; //2f + 1,inlude leader
            
            return false;
        }
        bool    xproposal_t::is_auditors_finish_vote() const
        {
            if(m_voted_auditors_count >= (int32_t) get_cert()->get_auditor_threshold())
                return true;//2f + 1,inlude leader
            
            return false;
        }
        bool     xproposal_t::is_vote_finish() const
        {
            if(is_validators_finish_vote() && (is_auditors_finish_vote()) )
                return true;
            
            return false;
        }
        
        bool     xproposal_t::add_voted_cert(const xvip2_t & voter_xip,base::xvqcert_t * qcert_ptr,base::xvcertauth_t * cert_auth)
        {
            if(NULL == qcert_ptr)
                return false;
                
            if(false == is_xip2_empty(voter_xip)) //sanity test first
            {
                const std::string account_addr_of_node = cert_auth->get_signer(voter_xip);
                if(account_addr_of_node.empty())
                {
                    xwarn("xproposal_t::add_replica_cert,fail-received vote from empty account of replica_xip=%llu",voter_xip.low_addr);
                    return false;
                }
                
                if(m_all_votors.find(account_addr_of_node) == m_all_votors.end()) //filter any duplicated account
                {
                    if(get_cert()->is_validator(voter_xip))
                    {
                        //NOTE: following code ask nodes of validtor and auditor with SAME election round
                        if(get_network_height_from_xip2(get_cert()->get_validator())
                           != get_network_height_from_xip2(voter_xip) )
                        {
                            xwarn("xproposal_t::add_voted_cert,fail-received vote from different election round.leader'validator=%llu vs voter_xip=%llu",get_cert()->get_validator().high_addr, voter_xip.high_addr);
                            return false;
                        }
                        
                        const std::string& signature = qcert_ptr->get_verify_signature();
                        auto set_res = m_all_voted_cert.emplace(signature);//emplace do test whether item already in set
                        if(set_res.second)//return true when it is a new element
                        {
                            auto map_res = m_voted_validators.emplace(voter_xip,signature);
                            if(map_res.second)
                            {
                                m_all_votors.emplace(account_addr_of_node);//record account to avoid duplicated nodes of same account
                                ++m_voted_validators_count;
                                return true;
                            }
                            xerror("xproposal_t::add_replica_cert,received two different verify certificate from replica_xip=%llu",voter_xip.low_addr);
                        }
                        else
                        {
                            xerror("xproposal_t::add_replica_cert,received a duplicated validator'certificate from replica_xip=%llu",voter_xip.low_addr);//possible attack
                        }
                        
                    }
                    else if(get_cert()->is_auditor(voter_xip))
                    {
                        //NOTE: following code ask nodes of validtor and auditor with SAME election round
                        if(get_network_height_from_xip2(get_cert()->get_auditor())
                           != get_network_height_from_xip2(voter_xip) )
                        {
                            xwarn("xproposal_t::add_voted_cert,fail-received vote from different election round.leader'auditor=%llu vs voter_xip=%llu",get_cert()->get_auditor().high_addr, voter_xip.high_addr);
                            return false;
                        }
                        
                        const std::string& signature = qcert_ptr->get_audit_signature();
                        auto set_res = m_all_voted_cert.emplace(signature);//emplace do test whether item already in set
                        if(set_res.second)//return true when it is a new element
                        {
                            auto map_res = m_voted_auditors.emplace(voter_xip,signature);
                            if(map_res.second)
                            {
                                m_all_votors.emplace(account_addr_of_node);//record account to avoid duplicated nodes of same account
                                ++m_voted_auditors_count;
                                return true;
                            }
                            xerror("xproposal_t::add_replica_cert,received two different audit certificate from replica_xip=%llu",voter_xip.low_addr);
                        }
                        else
                        {
                            xerror("xproposal_t::add_replica_cert,received a duplicated auditor'certificate from replica_xip=%llu",voter_xip.low_addr);//possible attack
                        }
                    }
                }
                else
                {
                    xwarn("xproposal_t::add_replica_cert,received signature from duplicated account(%s) of replica_xip=%llu",account_addr_of_node.c_str(),voter_xip.low_addr);
                }
            }
            else
            {
                xerror("xproposal_t::add_replica_cert,empty replica_xip");
            }
            return false;
        }
        
        xreplicate_t::xreplicate_t(const base::xvqcert_t & receipt)
        {
            m_replicate_cert = NULL;
            m_target_block   = NULL;
            
            m_replicate_cert = (base::xvqcert_t*)&receipt;
            m_replicate_cert->add_ref();
        }
        
        xreplicate_t::~xreplicate_t()
        {
            if(m_replicate_cert != NULL)
                m_replicate_cert->release_ref();
            
            if(m_target_block != NULL)
                m_target_block->release_ref();
        }
        
        void   xreplicate_t::set_block(base::xvblock_t* block)
        {
            if(block != NULL)
                block->add_ref();
            
            if(m_target_block != NULL)
                m_target_block->release_ref();
         
            m_target_block = block;
        }
        
        void   xreplicate_t::set_input(const std::string & input)
        {
            m_target_input = input;
        }
        void   xreplicate_t::set_output(const std::string & output)
        {
            m_target_output = output;
        }
        
    };//end of namespace of xconsensus
};//end of namespace of top
