// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xconsobj.h"

namespace top
{
    namespace xconsensus
    {
        /*high level view :
         [xsync_network_t]<--->Sync      Panel: [xsync_node_t]<--->[xsync_book_t  ]<--->[xsync_table_t  ]<--->[xsync_account_t  ]
                                                        |                 |                    |                     |
         [xstore]<--->Local    Data      Panel: [xchainledger]<--->[xledger_book_t]<--->[xledger_table_t]<--->[xledger_account_t]
                                                        |                 |                    |                     |
         [xcons_network_t]<--->Consensus Panel: [xcsnode_t]<--->[xcons_book_t  ]<--->[xcstable_t  ]<--->[xcsaccount_t  ]
         */
        //xcsaccount_t is the adapter object that underly link the storage of xledger'account and pluggin the consensus process
        //note: xcsaccount_t running at same thread of xcstable_t
        class xcsaccount_t : public xcscoreobj_t
        {
        public:
            xcsaccount_t(xcsobject_t & parent_object,const std::string & account_addr);
            xcsaccount_t(base::xcontext_t & _context,const int32_t target_thread_id,const std::string & account_addr);
        protected:
            virtual ~xcsaccount_t();
        private:
            xcsaccount_t();
            xcsaccount_t(const xcsaccount_t &);
            xcsaccount_t & operator = (const xcsaccount_t &);
        public:
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;

            bool            is_mailbox_over_limit(const int32_t max_mailbox_num) ;

        protected:
            virtual bool    on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override; //clock block always pass by higher layer to lower layer
            
            virtual bool    on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override; //call from higher layer to lower layer(child)
            
            virtual bool    on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override; //call from lower layer to higher layer(parent)
            
            virtual bool    on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override; //call from lower layer to higher layer(parent)
            
            virtual bool    on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override; //bidirect possible
            
            virtual bool    on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool    on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
        protected:
            //subclass need override this default behavior that create context of enum_xconsensus_context_type_xthbft
            virtual xcscoreobj_t* create_engine_object();
   
            virtual bool          store_cert_block(base::xvblock_t* _qcert_block); //just verified once by signature
            virtual bool          store_lock_block(base::xvblock_t* _lock_block);  //has been locked
            virtual bool          store_commit_block(base::xvblock_t* _commit_block);//has been  commited status
            
            //return specific error code(enum_xconsensus_result_code) to let caller know reason
            virtual int     verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) override; //load and execute block at sanbox
        };
        
    }; //end of namespace of xconsensus
};//end of namespace of top
