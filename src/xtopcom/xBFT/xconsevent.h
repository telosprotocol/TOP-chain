// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <queue>
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xvledger/xvblock.h"

namespace top
{
    namespace xconsensus
    {
        enum enum_xcsevent_type
        {
            enum_xcsevent_type_invalid              = 0,
            enum_xcsevent_type_on_life_cycle        = 2,
            
            //related event for basic function
            enum_xcsevent_type_on_view_fire         = 3, //on_new_view event about new-view# is issued
            enum_xcsevent_type_update_view          = 4,
            
            //related event for proposal status
            enum_xcsevent_type_on_proposal_start    = 11,
            enum_xcsevent_type_on_proposal_finish   = 12,
            
            //related event for consensus status
            enum_xcsevent_type_on_consensus_commit  = 21,
            enum_xcsevent_type_on_consensus_update  = 22,
            
            //related event for replicate & sync function
            enum_xcsevent_type_on_replicate_finish  = 31, //a replicated block from other ndoes through syncing method
            enum_xcsevent_type_on_certificate_finish= 32, //a certification has been verified,but dont have related header/input found

        };
        
        //event related xcspdu_t packet
        class xcspdu_fire : public base::xvevent_t
        {
        public:
            xcspdu_fire();
            xcspdu_fire(const base::xcspdu_t & packet);
        protected:
            virtual ~xcspdu_fire();
        private:
            xcspdu_fire(const xcspdu_fire & obj);
            xcspdu_fire& operator = (const xcspdu_fire & obj);
        public:
            inline base::xvqcert_t*  get_xclock_cert() const {return _packet_xclock_cert;}
            inline base::xvqcert_t*  get_vblock_cert() const {return _packet_vblock_cert;}
            
            void  set_xclock_cert(base::xvqcert_t* cert);
            void  set_vblock_cert(base::xvqcert_t* cert);
        public:
            base::xbftpdu_t         _packet;
        private:
            base::xvqcert_t*        _packet_xclock_cert;
            base::xvqcert_t*        _packet_vblock_cert;
        };

        //event for timer_picker create clock block for timecertview
        class xcscreate_block_evt : public base::xvevent_t
        {
        public:
            xcscreate_block_evt(const xvip2_t & from_addr, base::xvblock_t *vote, uint64_t clock, uint32_t context_id);
            xcscreate_block_evt(const xvip2_t & from_addr, base::xvblock_t *vote, base::xvblock_t *block, uint32_t context_id);
        protected:
            virtual ~xcscreate_block_evt();
        private:
            xcscreate_block_evt();
            xcscreate_block_evt(const xcscreate_block_evt & obj);
            xcscreate_block_evt& operator = (const xcscreate_block_evt & obj);
        public:
            const xvip2_t &get_xip() const {return _from_addr;}
            base::xvblock_t* get_vote() const {return _vote;}
            base::xvblock_t* get_block() const {return _block;}
            uint64_t get_clock() const {return _clock;}
            uint32_t get_context_id() const {return _context_id;}
        private:
            xvip2_t _from_addr;
            base::xvblock_t *_vote{};
            uint32_t _context_id;
            uint64_t _clock;
            base::xvblock_t *_block{};
        };

        // event for broadcast tc block to all nodes
        class xcstc_fire : public base::xvevent_t
        {
        public:
            xcstc_fire(base::xvblock_t * block, uint32_t broadcast_round);
        protected:
            virtual ~xcstc_fire();
        private:
            xcstc_fire();
            xcstc_fire(const xcstc_fire & obj);
            xcstc_fire& operator = (const xcstc_fire & obj);
        public:
            base::xvblock_t*  get_tc_block() const {return _tc_block;}
            uint32_t          get_broadcast_round() const {return m_broadcast_round;}
        private:
            base::xvblock_t*  _tc_block;
            uint32_t          m_broadcast_round{0};  // 0-first round elect leader by FTS;1-second round elect leader by RANDOM from prev tc block signature nodes
        };
        
        class xcsclock_fire : public base::xvevent_t
        {
        public:
            xcsclock_fire(base::xvblock_t & clock_block);
        protected:
            virtual ~xcsclock_fire();
        protected:
            xcsclock_fire();
            xcsclock_fire(const xcsclock_fire & obj);
            xcsclock_fire& operator = (const xcsclock_fire & obj);
        public:
            base::xvblock_t*  get_clock_block() const {return _clock_block;}
            base::xvblock_t*  get_latest_block()const {return _latest_block;}
            void              reset_latest_block(base::xvblock_t* highest_block);
        protected:
            base::xvblock_t*  _clock_block;  //global clock block
            base::xvblock_t*  _latest_block; //let clock carry highest block to update view as well
        };
        
        class xcsview_fire : public base::xvevent_t
        {
        public:
            xcsview_fire(const std::string & target_account,const uint64_t new_view_id,const uint64_t global_clock);
        protected:
            virtual ~xcsview_fire();
        protected:
            xcsview_fire();
            xcsview_fire(const xcsview_fire & obj);
            xcsview_fire& operator = (const xcsview_fire & obj);
        public:
            const std::string & get_account() const {return m_target_account;}
            const uint64_t      get_viewid()  const {return m_new_view_id;}
            const uint64_t      get_clock()   const {return m_global_clock;}
        protected:
            uint64_t            m_new_view_id;      //generate latest view id
            uint64_t            m_global_clock;     //clock block'height to generated new_viewid
            std::string         m_target_account;   //account'view changed
        };
        
        class xcsevent_t : public base::xvevent_t
        {
        protected:
            xcsevent_t(enum_xcsevent_type type);
            virtual ~xcsevent_t();
        private:
            xcsevent_t();
            xcsevent_t(const xcsevent_t &);
            xcsevent_t & operator = (const xcsevent_t &);
            
        public:
            inline base::xvblock_t*  get_latest_commit()   const {return m_latest_commit_block;}
            inline base::xvblock_t*  get_latest_lock()     const {return m_latest_lock_block;}
            inline base::xvblock_t*  get_latest_cert()     const {return m_latest_cert_block;}
            inline base::xvblock_t*  get_latest_proposal() const {return m_latest_proposal_block;}
            inline base::xvblock_t*  get_latest_clock()    const {return m_latest_clock_block;}
            
            void                     set_latest_commit(base::xvblock_t* block);
            void                     set_latest_lock(base::xvblock_t* block);
            void                     set_latest_cert(base::xvblock_t* block);
            void                     set_latest_proposal(base::xvblock_t* block);
            void                     set_latest_clock(base::xvblock_t* block);
        private:
            //update information of overall
            base::xvblock_t*       m_latest_commit_block;
            base::xvblock_t*       m_latest_lock_block;
            base::xvblock_t*       m_latest_cert_block;
            base::xvblock_t*       m_latest_proposal_block;
            //carry latest clock
            base::xvblock_t*       m_latest_clock_block;
        };
        
        
        /*  How consenus mechanisam running:
         on_proposal_start push the process of consensus of lower layer to go forward.
         and on_proposal_finish or on_consensus_update  push the process of consensus of upper layer to go forward.
         so the parent and child layer  together to  make the consenus is Process-Completely machine.
         moreover, multiple layers may do this tier like chain struture. that is why we define xcsobject_t as abstract & root
         */
        class xproposal_start : public xcsevent_t
        {
        public:
            xproposal_start();
            xproposal_start(base::xvblock_t*proposal_block);
        protected:
            virtual ~xproposal_start();
        private:
            xproposal_start(const xproposal_start & obj);
            xproposal_start& operator = (const xproposal_start & obj);
        public:
            const uint32_t                        get_expired_ms()      const {return m_expired_ms;}
            base::xvblock_t*                      get_proposal()        const {return m_proposal_block;}
            base::xvqcert_t*                      get_clock_cert()      const {return m_bind_clock_cert;}
        public:
            void   set_clock_cert(base::xvqcert_t* new_clock_cert);
        protected:
            base::xvblock_t*               m_proposal_block;
            base::xvqcert_t*               m_bind_clock_cert;       //bind clock cert with this proposal
            uint32_t                       m_expired_ms;            //proposal expired after this duration of m_expired_ms
        };
        
        //on_proposal_finish event = the proposal has been certified at least one-time,but still possible rollback/deny finally
        //once receive this notification, leader start new proposal of next round
        class xproposal_finish : public xcsevent_t
        {
        public:
            xproposal_finish(base::xvblock_t* proposal);  //successful case
            xproposal_finish(const int errcode,const std::string & err_detail,base::xvblock_t* proposal);//failue case
        protected:
            virtual ~xproposal_finish();
        private:
            xproposal_finish();
            xproposal_finish(const xproposal_finish & obj);
            xproposal_finish& operator = (const xproposal_finish & obj);
        public:
            base::xvblock_t*  get_target_proposal() const {return m_target_proposal;}
            void              reset_target_proposal(base::xvblock_t* new_proposal);
        protected:
            base::xvblock_t*  m_target_proposal;
        };

        class xupdate_view : public xcsevent_t
        {
        public:
            xupdate_view(base::xvblock_t* proposal);  //successful case
            xupdate_view(const int errcode,const std::string & err_detail,base::xvblock_t* proposal);//failue case
        protected:
            virtual ~xupdate_view();
        private:
            xupdate_view();
            xupdate_view(const xupdate_view & obj);
            xupdate_view& operator = (const xupdate_view & obj);
        public:
            base::xvblock_t*  get_target_proposal() const {return m_target_proposal;}
        protected:
            base::xvblock_t*  m_target_proposal;
        };
        
        //on_consensus_commit event = the proposal has been committed(certified at least 3times) and not allow rollback/forked anymore;
        class xconsensus_commit : public xcsevent_t
        {
        public:
            xconsensus_commit(base::xvblock_t * target_commit);  //successful case
        protected:
            virtual ~xconsensus_commit();
        private:
            xconsensus_commit();
            xconsensus_commit(const xconsensus_commit & obj);
            xconsensus_commit& operator = (const xconsensus_commit & obj);
        public:
            base::xvblock_t*  get_target_commit() const {return m_target_commit;}
        protected:
            base::xvblock_t*  m_target_commit;
        };
        
        //on_consensus_update event is to sync information between different layer
        class xconsensus_update : public xcsevent_t
        {
        public:
            xconsensus_update();
        protected:
            virtual ~xconsensus_update();
        private:
            xconsensus_update(const xconsensus_update & obj);
            xconsensus_update& operator = (const xconsensus_update & obj);
        protected:
            std::queue<base::xvblock_t*> m_commit_blocks;
        };
        
        //on_replicate_finish event = a replicated block from other ndoes through syncing method,and has been certified by signature,
        class xreplicate_finish : public xcsevent_t
        {
        public:
            xreplicate_finish(base::xvblock_t * target_block);
        protected:
            virtual ~xreplicate_finish();
        private:
            xreplicate_finish();
            xreplicate_finish(const xreplicate_finish & obj);
            xreplicate_finish& operator = (const xreplicate_finish & obj);
        public:
            base::xvblock_t*    get_target_block() const {return m_target_block;}
        protected:
            base::xvblock_t*    m_target_block;
        };
        
        //on_certificate_finish event = the proposal has been certified at least one-time,but still possible rollback/deny finally
        //an event that xBFT internal use only
        class xcertificate_finish : public xcsevent_t
        {
        public:
            xcertificate_finish(base::xvqcert_t * target_cert);
        protected:
            virtual ~xcertificate_finish();
        private:
            xcertificate_finish();
            xcertificate_finish(const xcertificate_finish & obj);
            xcertificate_finish& operator = (const xcertificate_finish & obj);
        public:
            base::xvqcert_t*    get_target_cert()  const {return m_target_cert;}
            base::xvblock_t*    get_target_block() const {return m_target_block;}
        protected: //carry m_target_cert or m_target_block
            base::xvqcert_t*    m_target_cert;
            base::xvblock_t*    m_target_block;
        };

    };//end of namespace of xconsensus
};//end of namespace of top
