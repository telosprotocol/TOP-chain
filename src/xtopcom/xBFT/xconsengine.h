// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <map>
#include "xconsobj.h"

namespace top
{
    namespace xconsensus
    {
        //guarantee all message are deliver and handle at single thread that is xcspacemaker_t'host thread
        class xcspacemaker_t : public xcscoreobj_t //general pacemaker for BFT
        {
            friend class xcscoreobj_t;
            static  xcspacemaker_t*  create_pacemaker_object(xcscoreobj_t&  parent,enum_xconsensus_pacemaker_type _type);
        protected:
            xcspacemaker_t(xcscoreobj_t&  parent_object);
            virtual ~xcspacemaker_t();
        private:
            xcspacemaker_t();
            xcspacemaker_t(const xcspacemaker_t &);
            xcspacemaker_t & operator = (const xcspacemaker_t &);
            
        public:
            virtual enum_xconsensus_pacemaker_type  get_pacemaker_type()       const = 0;
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;
        };
        
        //note: xcscontext_t must be running at same thread of parent_object of xcspacemaker_t
        //guarantee all message are deliver and handle at single thread that is xcscontext_t'host thread
        class xcscontext_t : public xcscoreobj_t  //general consensus context that manage QC-Blocks
        {
            friend class xcscoreobj_t;
            static xcscontext_t *  create_context_object(xcspacemaker_t&  parent_pacemaker);
        protected:
            xcscontext_t(xcscoreobj_t&  parent_object);
            virtual ~xcscontext_t();
        private:
            xcscontext_t();
            xcscontext_t(const xcscontext_t &);
            xcscontext_t & operator = (const xcscontext_t &);
        public:
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;
            
        protected:
            //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
            virtual bool    on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode) override;
            
            virtual bool    on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
        };
    
        class block_cert_pair
        {
        public:
            block_cert_pair(base::xvblock_t * _block_ptr,base::xvqcert_t * _cert_ptr)
            {
                m_block_ptr = NULL;
                m_cert_ptr  = NULL;
                
                m_block_ptr = _block_ptr;
                if(m_block_ptr != NULL)
                    m_block_ptr->add_ref(); //hold reference
                
                m_cert_ptr  = _cert_ptr;
                if(m_cert_ptr != NULL)  //hold reference
                    m_cert_ptr->add_ref();
            }
            ~block_cert_pair()
            {
                if(m_block_ptr != NULL)
                    m_block_ptr->release_ref();
                
                if(m_cert_ptr != NULL)
                    m_cert_ptr->release_ref();
            }
        public:
            base::xvblock_t *   get_block_ptr() const {return m_block_ptr;}
            base::xvqcert_t *   get_cert_ptr()  const {return m_cert_ptr;}
        private:
            base::xvblock_t * m_block_ptr = NULL;
            base::xvqcert_t * m_cert_ptr = NULL;
        };
    
        //note: xcsdriver_t must be running at same thread of xcscontext_t
        //guarantee all message are deliver and handle at single thread that is xcsdriver_t'host thread
        class xcsdriver_t : public xcscoreobj_t //general consenus driver of event & state
        {
            friend class xcscoreobj_t;
            static xcsdriver_t *  create_driver_object(xcscontext_t & parent_context);
        protected:
            xcsdriver_t(xcscoreobj_t & parent_object);
            virtual ~xcsdriver_t();
        private:
            xcsdriver_t();
            xcsdriver_t(const xcsdriver_t &);
            xcsdriver_t & operator = (const xcsdriver_t &);
        public:
            virtual void*     query_interface(const int32_t _enum_xobject_type_) override;
        };
        
        //general proposal wrap that hold raw block inside
        class xproposal_t : public base::xvbnode_t
        {
            typedef base::xvbnode_t base_class;
        public:
            xproposal_t(base::xvblock_t & _block,const base::xvblock_t* parent_block);
            xproposal_t(base::xvblock_t & _block,const base::xvqcert_t* parent_block_cert);
            xproposal_t(const xproposal_t & obj);
        protected:
            virtual ~xproposal_t();
        private:
            xproposal_t();
            xproposal_t(xproposal_t && obj);
            xproposal_t & operator = (xproposal_t &&);
            xproposal_t & operator = (const xproposal_t &);
        public:
            #ifdef DEBUG //tracking memory of proposal block
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
            #endif
            
            inline base::xvqcert_t* get_last_block_cert()    const {return m_last_block_cert;}
            inline base::xvqcert_t* get_bind_clock_cert()    const {return m_bind_clock_cert;}
            inline base::xvqcert_t* get_proposal_cert()      const {return m_proposal_cert;}
        public: //note below api might be called from woker'thread of workerpool,but from the same woker'thread for same account
            bool                 add_voted_cert(const xvip2_t & voter_xip,base::xvqcert_t * qcert_ptr,base::xvcertauth_t * cert_auth);
 
            bool                 is_validators_finish_vote() const;
            bool                 is_auditors_finish_vote() const;
            bool                 is_vote_finish() const;
            int                  get_result_of_verify_proposal() const {return m_result_verify_proposal;}
            const xvip2_t &      get_proposal_source_addr() const {return m_proposal_from_addr;}
            const uint32_t       get_proposal_msg_nonce() const {return m_proposal_msg_nonce;}
            
            const std::map<xvip2_t,std::string,xvip2_compare> &  get_voted_validators()   const {return m_voted_validators;}
            const std::map<xvip2_t,std::string,xvip2_compare> &  get_voted_auditors()     const {return m_voted_auditors;}
            
            void                  set_proposal_cert(base::xvqcert_t* new_proposal_cert);
            void                  set_bind_clock_cert(base::xvqcert_t* clock_cert);
        public: //below apis are called from engine'own thread
            bool                 is_leader() const {return m_is_leader;}
            bool                 is_voted()  const {return m_is_voted;}
            bool                 is_certed() const {return m_is_certed;}
            bool                 is_expired()const {return m_is_expired;}
            bool                 is_pending()const {return m_is_pending;}
            bool                 is_valid_packet(base::xcspdu_t & packet);
            bool                 is_vote_enable() const {return (m_allow_vote > 0);}
            bool                 is_vote_disable() const {return (m_allow_vote < 0);}
            
            const uint64_t       get_expired_ms() const {return m_expired_ms;}
            void                 set_expired_ms(const uint64_t unit_time_ms){ m_expired_ms = unit_time_ms;}
            void                 mark_certed() { m_is_certed = true;}
            void                 mark_leader() { m_is_leader = true;} //indicate it is leader'original proposal
            void                 mark_voted()  { m_is_voted = true;}  //indicate whether node has voted for proposal
            void                 mark_expired(){ m_is_expired = true;}
            void                 mark_pending(){ m_is_pending = true;}
            void                 enable_vote()  { m_allow_vote = 1;}  //allow vote
            void                 disable_vote() { m_allow_vote = -1;} //disallow vote

            void                 set_result_of_verify_proposal(const int result){m_result_verify_proposal = result;}
            void                 set_proposal_source_addr(const xvip2_t & from_addr){m_proposal_from_addr = from_addr;}
            void                 set_proposal_msg_nonce(const uint32_t nonce){m_proposal_msg_nonce = nonce;}
            
            bool                 set_highest_QC_viewid(const uint64_t new_viewid);
            const uint64_t       get_highest_QC_viewid() const;
        private:
            std::atomic<int32_t>           m_voted_validators_count;    //atomic for m_voted_validators' size
            std::atomic<int32_t>           m_voted_auditors_count;      //atomic for m_voted_auditors 'size
            
            std::set<std::string>          m_all_votors;    //to filter duplicated account of node
            std::set<std::string>          m_all_voted_cert;//to remove duplicated certificates,possible attack or duplicated
            std::map<xvip2_t,std::string,xvip2_compare> m_voted_validators;          //include leader as well
            std::map<xvip2_t,std::string,xvip2_compare> m_voted_auditors;            //include leader as well if need
        private:
            base::xvqcert_t *              m_proposal_cert;             //dedicated cert to store signature of leader
            base::xvqcert_t *              m_bind_clock_cert;           //each proposal ask carry the related clock cert
            base::xvqcert_t *              m_last_block_cert;           //certification of last header,it might be nil
            uint64_t                       m_expired_ms;                //when the proposal expired to clean up
            bool                           m_is_leader;                 //indicate whether it is generated from leader'proposal
            bool                           m_is_voted;                  //for node mark whether has voted or not
            //some proposal that is behind locked/commit block is not allow vote anymore ,bu keep it and  waiting related commit-cert to reduce sync
            bool                           m_is_certed;                 //tell whether it has been a valid cert block at leader
            bool                           m_is_expired;                //tell whether proposal expired
            bool                           m_is_pending;                //pending == a outofdated proposal but waiting for commist-msg
            int                            m_allow_vote;                //unknow for 0, false for < 0 and true for > 0
            int                            m_result_verify_proposal;    //indicated what is result of verify_proposal
            xvip2_t                        m_proposal_from_addr;        //record the source addr,so that we may vote it async mode
            uint32_t                       m_proposal_msg_nonce;        //record orginal message 'nonce
            uint64_t                       m_highest_QC_viewid;         //record what is highest viewid collected from backup
        };
        struct sort_proposal
        {
            bool operator()(const xproposal_t * front,const xproposal_t * back)
            {
                if(nullptr == front)
                    return true;
                
                if(nullptr == back)
                    return false;
                
                return (front->get_viewid() < back->get_viewid());
            }
            
            bool operator()(const xproposal_t & front,const xproposal_t & back)
            {
                return (front.get_viewid() < back.get_viewid());
            }
        };
        
        //manage a task to replicate one block from peer
        class xreplicate_t
        {
        public:
            xreplicate_t(const base::xvqcert_t & receipt);
        protected:
            virtual ~xreplicate_t();
        private:
            xreplicate_t();
            xreplicate_t(const xreplicate_t &);
            xreplicate_t & operator = (const xreplicate_t &);
        public:
            inline base::xvqcert_t *    get_receipt_cert()  const {return m_replicate_cert;}
            inline base::xvblock_t *    get_target_block()  const {return m_target_block;}
            inline const std::string&   get_target_input()  const {return m_target_input;}
            inline const std::string&   get_target_output() const {return m_target_output;}
            
            void   set_block(base::xvblock_t* block);
            void   set_input(const std::string & input);
            void   set_output(const std::string & output);
        private:
            base::xvqcert_t *  m_replicate_cert;  //receipt to pull/push data while replicating
            base::xvblock_t*   m_target_block; //object of block
            std::string        m_target_input; //raw data of input,allow pull data seperately with m_target_block,then merge
            std::string        m_target_output;//raw data of input,allow pull data seperately with m_target_block,then merge
        };
    }; //end of namespace of xconsensus
};//end of namespace of top
