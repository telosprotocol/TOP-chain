// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xconsengine.h"

namespace top
{
    namespace xconsensus
    {
        class xclockcert_view: public xcspacemaker_t //clock-block drive to make agreement for view#
        {
        public:
            xclockcert_view(xcscoreobj_t&  parent_object);
        protected:
            virtual ~xclockcert_view();
        private:
            xclockcert_view();
            xclockcert_view(const xclockcert_view &);
            xclockcert_view & operator = (const xclockcert_view &);
        public:
            virtual enum_xconsensus_pacemaker_type  get_pacemaker_type() const override {return enum_xconsensus_pacemaker_type_clock_cert;}
            
            const uint64_t  get_latest_xclock_height();
            const uint64_t  get_latest_vblock_viewid();
            const uint64_t  get_latest_viewid() const {return m_latest_view_id;}
            const uint64_t  get_latest_viewid(const std::string &account);//force init first as protection
    
        protected:
            //clock block always pass by higher layer to lower layer
            virtual bool  on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
   
            virtual bool  on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool  on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) final override;
            
        protected: //just used to update commit block
            virtual bool  on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from higher layer to lower layer(child)

            virtual bool  on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;  //call from lower layer to higher layer(parent)
            
            virtual bool  on_certificate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            //notify this node that is joined into parent-node
            virtual bool  on_join_parent_node(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,const xvip2_t & alloc_address,const std::string & extra_data,xionode_t* from_parent) override;

            virtual bool  on_update_view(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
        protected:
            bool  update_view(); //return true if change view successful
            //return specific error code(enum_xconsensus_result_code) to let caller know reason
            virtual int   verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) override; //load and execute block at sanbox
            bool          fire_verify_pduevent_job(xcspdu_fire * _evt_obj);
        private:
            bool    init_vblock(const std::string &account);
            
            //manage latest clock cert(aka Clock HQC)
            bool    load_clock_cert();
            bool    safe_check_clock_cert(base::xvqcert_t * clock_block);
            bool    update_clock_cert(base::xvqcert_t * clock_cert);
            bool    update_clock_cert(base::xvblock_t * clock_block);
            
            //manange latest block cert(aka HQC)
            bool    load_vblock_cert();
            bool    safe_check_vblock_cert(base::xvqcert_t * hqc_block);
            bool    update_vblock_cert(base::xvqcert_t * hqc_cert);
            bool    update_vblock_cert(base::xvblock_t * hqc_block);
            
        protected:
            uint64_t            m_latest_view_id;
            base::xvqcert_t*    m_latest_clock_cert;     //latest/highest cert of clock
            base::xvqcert_t*    m_latest_vblock_cert;    //latest/highest qc cert of block
        };
    };//end of namespace of xconsensus
};//end of namespace of top
