// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xconsengine.h"
#include "xbase/xthread.h"

namespace top
{
    namespace xconsensus
    {
        struct block_helper
        {
        public:
            template<typename T>
            bool  add_block(T* _target_block,std::map<uint64_t,T*> & stdmap)
            {
                T* & existing_block = stdmap[_target_block->get_viewid()];//using reference
                if(existing_block == _target_block) //test whether it is same one
                    return true;
                
                _target_block->add_ref();
                if(existing_block != NULL)
                    existing_block->release_ref();
                
                existing_block = _target_block;
                return true;
            }
            
            template<typename T>
            bool remove_block(const uint64_t view_id,std::map<uint64_t,T*> & stdmap)
            {
                auto it = stdmap.find(view_id);
                if(it != stdmap.end())
                {
                    T* block_ptr = it->second;
                    stdmap.erase(it);
                    block_ptr->release_ref();
                    
                    return true;
                }
                return false;
            }
            
            template<typename T>
            bool clean_blocks(std::map<uint64_t,T*> & stdmap)
            {
                for(auto it = stdmap.begin(); it != stdmap.end();++it)
                {
                    T * _block = it->second;
                    if(_block != NULL)
                        _block->release_ref();
                }
                stdmap.clear();
                return true;
            }
            
            template<typename T>
            T*  find_block(const uint64_t view_id,const std::map<uint64_t,T*> & stdmap) const
            {
                auto it = stdmap.find(view_id);
                if(it != stdmap.end())
                {
                    return it->second;
                }
                return NULL;
            }
            
            template<typename T>
            T*  get_latest_block(const std::map<uint64_t,T*> & stdmap) const
            {
                if(stdmap.empty())
                    return NULL;
                
                auto it = stdmap.rbegin();
                return it->second;
            }
        };
        
        class xBFTRules : public xcsdriver_t,public block_helper
        {
        protected:
            xBFTRules(xcscoreobj_t & parent_object);
            virtual ~xBFTRules();
        private:
            xBFTRules();
            xBFTRules(const xBFTRules &);
            xBFTRules & operator = (const xBFTRules &);
        public:
            virtual base::xdataunit_t::enum_xpdu_type   get_target_pdu_class() const {return base::xdataunit_t::enum_xpdu_type_consensus_xbft;} //what pdu is handled by engine
 
            virtual std::string  dump() const override;  //dump driver ' main information
            
            xvip2_t             get_leader_address(base::xvblock_t * _block)
            {
                const xvip2_t& leader_xip = _block->get_cert()->get_validator();
                if (get_node_id_from_xip2(leader_xip) != 0x3FF)//valid node id instead of broadcast address
                    return leader_xip;
                
                return  _block->get_cert()->get_auditor();
            }
        protected: //blocks mangagement
            base::xvblock_t *   get_commit_block();
            bool                set_commit_block(base::xvblock_t * lastst_commit_block);//update block of  commited one
            
            base::xvblock_t *   get_lock_block();
            virtual bool        set_lock_block(base::xvblock_t * lastst_lock_block);//update block of  locked one
 
            //block managed fro proposal
            xproposal_t*        add_proposal(base::xvblock_t * proposal,base::xvblock_t * parent_block,const uint32_t expired_ms,base::xvqcert_t * clock_cert);//add into local cache
            bool                add_proposal(xproposal_t & proposal_block);
            bool                clean_proposals();
            xproposal_t*        find_proposal(const uint64_t view_id) const;
            xproposal_t*        get_latest_proposal() const;
            base::xvblock_t*    get_latest_proposal_block() const;
            
            //block manage for certified block
            bool                on_cert_verified(base::xvqcert_t * new_cert);
            bool                add_cert_block(base::xvblock_t* _target_block,bool & found_matched_proposal);
            bool                remove_cert_block(const uint64_t view_id);
            bool                clean_cert_blocks();
            base::xvblock_t*    get_latest_cert_block() const;
            base::xvblock_t*    find_first_cert_block(const uint64_t block_height) const;
            base::xvblock_t*    find_cert_block(const uint64_t view_id) const;
            base::xvblock_t*    find_cert_block(const uint64_t block_height,const std::string & block_hash);
 
            base::xauto_ptr<base::xvbindex_t> load_block_index(const uint64_t block_height,const std::string & block_hash);
            
            uint64_t            get_latest_voted_viewid() const {return m_latest_voted_viewid;}
            uint64_t            get_latest_voted_height() const {return m_latest_voted_height;}

            const uint64_t      get_latest_viewid() const {return m_latest_viewid;}
            const uint64_t      get_lastest_clock() const {return m_latest_clock;}
            
            void                update_voted_metric(base::xvblock_t * _block);
        protected: //safe rules
            virtual bool       is_proposal_expire(xproposal_t * _proposal);
            virtual bool       safe_check_for_block(base::xvblock_t * _test_block);//the minimal rule for block,
            virtual bool       safe_check_for_packet(base::xcspdu_t & _test_packet);//the minimal rule for packet
            
            int     safe_check_add_cert_fork(base::xvblock_t * _test_for_block);//rule to resolve any possible fork for cert 
            
            //return  > 0 when true, and return  < 0 when false, and return  0  when unknow
            int     safe_check_follow_commit_branch(base::xvblock_t * _test_for_block); //test whether at commited branch
            int     safe_check_follow_locked_branch(base::xvblock_t * _test_for_block); //test whether at locked branch
            bool    safe_check_for_lock_block(base::xvblock_t * _locking_block);//safe rule for lock block
            
            //sanity check and verify for block
            bool    safe_check_for_proposal_block(base::xvblock_t * _proposal_block);//safe rule for proposal block
            bool    safe_check_for_sync_block(base::xvblock_t * _commit_block);//safe rule for commit block
            
            bool    safe_precheck_for_voting(xproposal_t* new_proposal);
            bool    safe_precheck_for_voting(base::xvblock_t * _voting_block);//safe rule for voting block
            //check again before send voting msg and after verified signature
            bool    safe_finalcheck_for_voting(xproposal_t* new_proposal);
            bool    safe_finalcheck_for_voting(base::xvblock_t * _vote_block);//safe rule for voting block;
            
            //sanity check and verify for packet
            bool    safe_check_for_proposal_packet(base::xcspdu_t & packet,xproposal_msg_t & out_msg);//sanity check for proposal msg
            bool    safe_check_for_proposal_packet(base::xcspdu_t & packet,xproposal_msg_v2_t & out_msg);
            bool    safe_check_for_vote_packet(base::xcspdu_t & in_packet,xvote_msg_t & out_msg);//sanity check for vote msg
            bool    safe_check_for_commit_packet(base::xcspdu_t & in_packet,xcommit_msg_t & out_msg);
            bool    safe_check_for_sync_request_packet(base::xcspdu_t & packet,xsync_request_t & _syncrequest_msg);
            bool    safe_check_for_sync_respond_packet(base::xcspdu_t & packet,xsync_respond_t & _sync_respond_msg);
            bool    safe_check_for_sync_respond_v2_packet(base::xcspdu_t & packet,xsync_respond_v2_t & _sync_respond_msg);
            
        protected:
            inline std::map<uint64_t,xproposal_t*> &    get_proposals()  {return m_proposal_blocks;}
            inline std::map<uint64_t,base::xvblock_t*>& get_cert_blocks(){return m_certified_blocks;}
            bool    safe_align_with_blockstore(xproposal_t* new_proposal);
        private:
            /*
             Term: "Proposal-block"    = has full data(header,input,basic cert) of block but not finish verification and certificate
             Term: "syncing_blocks"    = certificate has been been verified but dont have raw data like input, pending to download/sync
             Term: "certified_blocks"  = "proposal" + full certificate & has been verified
             {proposal_blocks}
             /
             [last_lock]<---{certified_blocks}
             \
             {syncing_blocks}
             */
            base::xvblock_t *                    m_latest_commit_block; //latest commited block passed by context
            base::xvblock_t *                    m_latest_lock_block; //latest locked block passed by context
            std::map<uint64_t,base::xvblock_t*>  m_certified_blocks;  //sort by view#id from lower to higher
            std::map<uint64_t,xproposal_t*>      m_proposal_blocks;   //sort by view#id from lower to higher
        private:
            uint64_t                             m_latest_voted_height; //height of latest proposal voted
            uint64_t                             m_latest_voted_viewid; //view# of latest proposal voted
        protected:
            uint64_t                             m_latest_viewid;
            uint64_t                             m_latest_clock;
        };

        //sync function under xBFTRules
        class xBFTSyncdrv : public xBFTRules
        {
        public:
            xBFTSyncdrv(xcscoreobj_t & parent_object);
        protected:
            virtual ~xBFTSyncdrv();
        private:
            xBFTSyncdrv();
            xBFTSyncdrv(const xBFTSyncdrv &);
            xBFTSyncdrv & operator = (const xBFTSyncdrv &);
            
        protected://block managed for synchoronization
            virtual bool        set_lock_block(base::xvblock_t * lastst_lock_block) override;//update block of  locked one
 
            //fire view-change event
            virtual bool        on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //clock block always pass by higher layer to lower layer
            virtual bool        on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            virtual bool        on_new_block_fire(base::xvblock_t * new_cert_block){return false;}
        protected:
            int   handle_sync_request_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            int   handle_sync_respond_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
            bool  send_sync_request(const xvip2_t & from_addr,const xvip2_t & to_addr,const uint64_t target_block_height,const std::string & target_block_hash,base::xvqcert_t* proof_cert,const uint64_t proof_cert_height,const uint64_t expired_at_clock,const uint64_t chainid);
            bool  send_sync_request(const xvip2_t & from_addr,const xvip2_t & to_addr,const uint64_t target_block_height,const std::string & target_block_hash,const uint64_t proof_block_viewid,const uint32_t proof_block_viewtoken,const uint64_t proof_block_height,const uint64_t expired_at_clock,const uint64_t chainid);
            
            bool  resync_local_and_peer(base::xvblock_t* peer_block,const xvip2_t & peer_addr,const xvip2_t & my_addr,const uint64_t cur_clock);
        private:
            bool                fire_verify_syncblock_job(base::xvblock_t * target_block,base::xvqcert_t * paired_cert);
            
            class xsyn_request
            {
            public:
                xsyn_request()
                {
                    target_height   = 0;
                    expired_viewid  = 0;
                    expired_clock   = 0;
                    sync_trycount   = 0;
                }
                xsyn_request(const uint64_t _targetheight,const uint64_t _expired_clock,const uint8_t _trycount)
                {
                    target_height   = _targetheight;
                    expired_clock   = _expired_clock;
                    sync_trycount   = _trycount;
                }
                xsyn_request(const xsyn_request & obj)
                {
                    *this = obj;
                }
                xsyn_request & operator = (const xsyn_request & obj)
                {
                    target_height    = obj.target_height;
                    expired_viewid   = obj.expired_viewid;
                    expired_clock    = obj.expired_clock;
                    sync_trycount    = obj.sync_trycount;
                    return *this;
                }
            public:
                uint64_t  target_height;
                uint64_t  expired_clock; //clock height
                uint64_t  expired_viewid;
                uint8_t   sync_trycount;
            };
        private:
            std::map<std::string,xsyn_request>  m_syncing_requests; //hash -->height
        };
        
        //general BFT core layer
        class xBFTdriver_t : public xBFTSyncdrv //general consenus driver of event & state,
        {
        public:
            xBFTdriver_t(xcscoreobj_t & parent_object);
        protected:
            virtual ~xBFTdriver_t();
        private:
            xBFTdriver_t();
            xBFTdriver_t(const xBFTdriver_t &);
            xBFTdriver_t & operator = (const xBFTdriver_t &);
            
        protected: //handling consensus event
            //call from higher layer to lower layer(child)
            virtual bool    on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool    on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //clock block always pass by higher layer to lower layer
            virtual bool    on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //fire view-change event
            virtual bool    on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            virtual bool  on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
            virtual bool        on_new_block_fire(base::xvblock_t * new_cert_block) override;
            
        protected: //event' help function
            bool                async_fire_consensus_update_event();
            bool                async_fire_proposal_finish_event(const int err_code,base::xvblock_t* proposal);
            bool                async_fire_proposal_finish_event(const int err_code,const std::string & err_detail,base::xvblock_t* proposal);
            
            bool                fire_verify_qc_job(base::xvblock_t * target_block,base::xfunction_t  callback,base::xworkerpool_t * _workers_pool);
            bool                fire_verify_block_job(const xvip_t replica_xip,base::xvblock_t * target_block,base::xfunction_t callback,base::xworkerpool_t * _workers_pool);
            
            bool                fire_verify_cert_job(base::xvqcert_t * target_cert);
            bool                fire_verify_commit_job(base::xvblock_t * target_block,base::xvqcert_t * paired_cert);
            bool                fire_verify_vote_job(const xvip2_t replica_xip,base::xvqcert_t*replica_cert,xproposal_t * local_proposal,base::xfunction_t &callback, const std::string & vote_extend_data);
            bool                fire_verify_proposal_job(const xvip2_t leader_xip,const xvip2_t replica_xip,xproposal_t * target_proposal,base::xfunction_t &callback);
            
            bool                notify_proposal_fail(std::vector<xproposal_t*> & timeout_list,std::vector<xproposal_t*> &outofdate_list);
            
            bool                send_report(const int result,const xvip2_t & from_addr,const xvip2_t & to_addr);
            
        private: //message handling
            virtual int   handle_proposal_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
            virtual int   handle_vote_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
            virtual int   handle_commit_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
            virtual int   handle_votereport_msg(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
            bool  on_proposal_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            bool  on_vote_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            bool  on_commit_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            bool  on_sync_request_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            bool  on_sync_respond_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            bool  on_votereport_msg_recv(const xvip2_t & from_addr,const xvip2_t & to_addr,xcspdu_fire * event_obj,int32_t cur_thread_id,uint64_t timenow_ms,xcsobject_t * _parent);
            
        private:
            int  sync_for_proposal(xproposal_t* _proposal);
            int  vote_for_proposal(xproposal_t* _proposal);
            virtual bool is_proposal_expire(xproposal_t * _proposal) override;
        };
        
    }; //end of namespace of xconsensus
};//end of namespace of top
