// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconsevent.h"

namespace top
{
    namespace xconsensus
    {
        xcspdu_fire::xcspdu_fire()
            :base::xvevent_t(base::enum_xevent_core_type_pdu)
        {
            _packet_xclock_cert = NULL;
            _packet_vblock_cert = NULL;
        }
        
        xcspdu_fire::xcspdu_fire(const base::xcspdu_t & packet)
            :base::xvevent_t(base::enum_xevent_core_type_pdu)
        ,_packet(packet)
        {
            _packet_xclock_cert = NULL;
            _packet_vblock_cert = NULL;
        }
        
        xcspdu_fire::~xcspdu_fire()
        {
            if(_packet_xclock_cert != NULL)
                _packet_xclock_cert->release_ref();
            
            if(_packet_vblock_cert != NULL)
                _packet_vblock_cert->release_ref();
        }
        
        void  xcspdu_fire::set_xclock_cert(base::xvqcert_t* cert)
        {
            if(cert != NULL)
                cert->add_ref();
            
            base::xvqcert_t* old_ptr = base::xatomic_t::xexchange(_packet_xclock_cert, cert);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
        void  xcspdu_fire::set_vblock_cert(base::xvqcert_t* cert)
        {
            if(cert != NULL)
                cert->add_ref();
            
            base::xvqcert_t* old_ptr = base::xatomic_t::xexchange(_packet_vblock_cert, cert);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }

        xcscreate_block_evt::xcscreate_block_evt(const xvip2_t & from_addr, base::xvblock_t *vote, uint64_t clock, uint32_t context_id)
            :base::xvevent_t(base::enum_xevent_core_type_create_block)
        {
            _from_addr = from_addr;
            _vote = vote;
            _clock = clock;
            _context_id = context_id;
            _block = NULL;
        }
        
        xcscreate_block_evt::xcscreate_block_evt(const xvip2_t & from_addr, base::xvblock_t *vote, base::xvblock_t *block, uint32_t context_id)
            :base::xvevent_t(base::enum_xevent_core_type_create_block)
        {
            _from_addr = from_addr;
            _vote = vote;
            _context_id = context_id;
            _block = block;
        }
        
        xcscreate_block_evt::~xcscreate_block_evt()
        {
        }

        xcstc_fire::xcstc_fire(base::xvblock_t * block, uint32_t broadcast_round)
            :base::xvevent_t(base::enum_xevent_core_type_tc)
        {
            block->add_ref();
            _tc_block = block;
            m_broadcast_round = broadcast_round;
        }

        xcstc_fire::~xcstc_fire()
        {
            if(_tc_block != NULL)
                _tc_block->release_ref();
        }

        xcsclock_fire::xcsclock_fire(base::xvblock_t & clock_block)
            :base::xvevent_t(base::enum_xevent_core_type_clock)
        {
            _latest_block = NULL;
            clock_block.add_ref();
            _clock_block = &clock_block;
        }
    
        xcsclock_fire::~xcsclock_fire()
        {
            if(_clock_block != NULL)
                _clock_block->release_ref();
            
            if(_latest_block != NULL)
                _latest_block->release_ref();
        }
    
        void   xcsclock_fire::reset_latest_block(base::xvblock_t* highest_block)
        {
            if(_latest_block != NULL)
            {
                _latest_block->release_ref();
                _latest_block = NULL;
            }
            if(highest_block != NULL)
            {
                highest_block->add_ref();
                _latest_block = highest_block;
            }
        }
        
        xcsview_fire::xcsview_fire(const std::string & target_account,const uint64_t new_view_id,const uint64_t global_clock)
            :base::xvevent_t(enum_xcsevent_type_on_view_fire)
        {
            m_new_view_id       = new_view_id;
            m_global_clock      = global_clock;
            m_target_account    = target_account;
        }
 
        xcsview_fire::~xcsview_fire()
        {
        }
        
        xcsevent_t::xcsevent_t(enum_xcsevent_type type)
        :base::xvevent_t(type)
        {
            m_latest_commit_block   = NULL;
            m_latest_lock_block     = NULL;
            m_latest_cert_block     = NULL;
            m_latest_proposal_block = NULL;
            m_latest_clock_block    = NULL;
        }
        
        xcsevent_t::~xcsevent_t()
        {
            if(m_latest_commit_block != NULL)
                m_latest_commit_block->release_ref();
            
            if(m_latest_lock_block != NULL)
                m_latest_lock_block->release_ref();
            
            if(m_latest_cert_block != NULL)
                m_latest_cert_block->release_ref();
            
            if(m_latest_proposal_block != NULL)
                m_latest_proposal_block->release_ref();
            
            if(m_latest_clock_block != NULL)
                m_latest_clock_block->release_ref();
        }
        
        void   xcsevent_t::set_latest_commit(base::xvblock_t* block)
        {
            if(block != m_latest_commit_block)
            {
                if(block != NULL)
                    block->add_ref();
                
                if(m_latest_commit_block != NULL)
                    m_latest_commit_block->release_ref();
                
                m_latest_commit_block = block;
            }
        }
        void   xcsevent_t::set_latest_lock(base::xvblock_t* block)
        {
            if(block != m_latest_lock_block)
            {
                if(block != NULL)
                    block->add_ref();
                
                if(m_latest_lock_block != NULL)
                    m_latest_lock_block->release_ref();
                
                m_latest_lock_block = block;
            }
        }
        void   xcsevent_t::set_latest_cert(base::xvblock_t* block)
        {
            if(block != m_latest_cert_block)
            {
                if(block != NULL)
                    block->add_ref();
                
                if(m_latest_cert_block != NULL)
                    m_latest_cert_block->release_ref();
                
                m_latest_cert_block = block;
            }
        }
        void   xcsevent_t::set_latest_proposal(base::xvblock_t* block)
        {
            if(block != m_latest_proposal_block)
            {
                if(block != NULL)
                    block->add_ref();
                
                if(m_latest_proposal_block != NULL)
                    m_latest_proposal_block->release_ref();
                
                m_latest_proposal_block = block;
            }
        }
        
        void   xcsevent_t::set_latest_clock(base::xvblock_t* block)
        {
            if(block != m_latest_clock_block)
            {
                if(block != NULL)
                    block->add_ref();
                
                if(m_latest_clock_block != NULL)
                    m_latest_clock_block->release_ref();
                
                m_latest_clock_block = block;
            }
        }
        
        xproposal_start::xproposal_start()
        :xcsevent_t(enum_xcsevent_type_on_proposal_start)
        {
            m_bind_clock_cert= NULL;
            m_proposal_block = NULL;
            m_expired_ms = 15000; //default as 15 seconds
        }
        
        xproposal_start::xproposal_start(base::xvblock_t*proposal_block)
        :xcsevent_t(enum_xcsevent_type_on_proposal_start)
        {
            m_expired_ms = 15000; //default as 15 seconds
            
            m_bind_clock_cert= NULL;
            m_proposal_block = NULL;
            if(proposal_block != NULL)
                proposal_block->add_ref();
            
            m_proposal_block = proposal_block;
        }
        
        xproposal_start::~xproposal_start()
        {
            if(m_proposal_block != NULL)
                m_proposal_block->release_ref();
            
            if(m_bind_clock_cert != NULL)
                m_bind_clock_cert->release_ref();
        }
        
        void  xproposal_start::set_clock_cert(base::xvqcert_t* new_clock_cert)
        {
            if(new_clock_cert != NULL)
                new_clock_cert->add_ref();
            
            base::xvqcert_t* old_one = m_bind_clock_cert;
            m_bind_clock_cert = new_clock_cert;
            if(old_one != NULL)
                old_one->release_ref();
        }
        
        xproposal_finish::xproposal_finish(base::xvblock_t* proposal)   //successful case
        :xcsevent_t(enum_xcsevent_type_on_proposal_finish)
        {
            m_target_proposal = NULL;
            if(proposal != NULL)
                proposal->add_ref();
            m_target_proposal = proposal;
        }
        
        xproposal_finish::xproposal_finish(const int errcode,const std::string & err_detail,base::xvblock_t* proposal)
        :xcsevent_t(enum_xcsevent_type_on_proposal_finish)
        {
            m_target_proposal = NULL;
            if(proposal != NULL)
                proposal->add_ref();
            m_target_proposal = proposal;
            
            m_error_code    = errcode;
            m_result_data   = err_detail;
        }
        
        xproposal_finish::~xproposal_finish()
        {
            if(m_target_proposal != NULL)
                m_target_proposal->release_ref();
        }
        
        void   xproposal_finish::reset_target_proposal(base::xvblock_t* new_proposal)
        {
            if(new_proposal != NULL)
                new_proposal->add_ref();
            
            base::xvblock_t* old_ptr = base::xatomic_t::xexchange(m_target_proposal, new_proposal);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }

        xupdate_view::xupdate_view(base::xvblock_t* proposal)   //successful case
        :xcsevent_t(enum_xcsevent_type_update_view)
        {
            m_target_proposal = NULL;
            if(proposal != NULL)
                proposal->add_ref();
            m_target_proposal = proposal;
        }
        
        xupdate_view::xupdate_view(const int errcode,const std::string & err_detail,base::xvblock_t* proposal)
        :xcsevent_t(enum_xcsevent_type_update_view)
        {
            m_target_proposal = NULL;
            if(proposal != NULL)
                proposal->add_ref();
            m_target_proposal = proposal;
            
            m_error_code    = errcode;
            m_result_data   = err_detail;
        }
        
        xupdate_view::~xupdate_view()
        {
            if(m_target_proposal != NULL)
                m_target_proposal->release_ref();
        }
        
        xconsensus_commit::xconsensus_commit(base::xvblock_t * target_commit)
        :xcsevent_t(enum_xcsevent_type_on_consensus_commit)
        {
            if(target_commit != NULL)
                target_commit->add_ref();
            m_target_commit = target_commit;
        }
        
        xconsensus_commit::~xconsensus_commit()
        {
            if(m_target_commit != NULL)
                m_target_commit->release_ref();
        }
        
        xconsensus_update::xconsensus_update()
        :xcsevent_t(enum_xcsevent_type_on_consensus_update)
        {
        }
        
        xconsensus_update::~xconsensus_update()
        {
        }
        
        xreplicate_finish::xreplicate_finish(base::xvblock_t * target_block)
            :xcsevent_t(enum_xcsevent_type_on_replicate_finish)
        {
            m_target_block = NULL;
            if(target_block != NULL)
                target_block->add_ref();
            m_target_block = target_block;
        }        
        xreplicate_finish::~xreplicate_finish()
        {
            if(m_target_block != NULL)
                m_target_block->release_ref();
        }
        
        xcertificate_finish::xcertificate_finish(base::xvqcert_t * target_cert)
            :xcsevent_t(enum_xcsevent_type_on_certificate_finish)
        {
            m_target_cert  = NULL;
            m_target_block = NULL;
            if(target_cert != NULL)
                target_cert->add_ref();
            m_target_cert = target_cert;
        }
        
        xcertificate_finish::~xcertificate_finish()
        {
            if(m_target_cert != NULL)
                m_target_cert->release_ref();
        }

        
    };//end of namespace of xconsensus
};//end of namespace of top
