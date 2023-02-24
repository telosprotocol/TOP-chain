// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <queue>
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvcertauth.h"
#include "xconsevent.h"
#include "xconspdu.h"


namespace top
{
    namespace xconsensus
    {
        //The whole design to let each layer of consensus work with eachother seamless and transparently,and together may solve different requirements, especial for test purpose.To reach this goal, every layer is xconsobject_t with same rules for interface, and the following are typical objects:
        
        //[consensus_node   ] present one logic node, a physical node might be multiple [consensus_node   ] as different role
        //[consensus_table  ] manage group consensus for  [consensus_account]
        //[consensus_account] manage related block   with Persisted storage(DB,or file)
        //[consensus_pacemaker] manage view of bft
        //[consensus_context] manage related status  with Erasable  storage(memory,or tmp file)
        //[consensus_driver ] manage related event   with Stateless
        //[consensus_runtime] manage threads pool and given exectued env
 
        //those xconsobject_t could be connected as diffierent purpose:
        //1. The ideal framework/chain   as: ...[consensus_node    ]<---->[consensus_table   ]<---->[consensus_account]<---->[consensus_context]<---->[consensus_driver]
        //      or simplify chain        as: ...[consensus_node    ]<---->[consensus_account ]<---->[consensus_context]<---->[consensus_driver ]
        //2. To compatible old framework as: ...[consensus_account ]<---->[consensus_instance]<---->[consensus_context]<---->[consensus_driver ]
        //3. To simulate shard consensus as: ...[consensus_network ]<---->[consensus_testnode]<---->[consensus_context]<---->[consensus_driver ]
        //4. To test consenus algorithm  as: ...[consensus_context ]<---->[consensus_driver  ]
        //5. To test consenus event      as: ...[consensus_driver  ]
        //6. other cases
        
        /*high level view for TOP Chain Platform:
         [xsync_network_t]<--->Sync      Panel: [xsync_node_t]<--->[xsync_book_t  ]<--->[xsync_table_t  ]<--->[xsync_account_t  ]
                                                       |                 |                      |                    |
         [xstore]<--->Local    Data      Panel: [xchainledger]<--->[xledger_book_t]<--->[xledger_table_t]<--->[xledger_account_t]
                                                       |                 |                      |                    |
         [xcons_network_t]<--->Consensus Panel: [xcsnode_t]<--->[xcons_book_t  ]<--->[xcstable_t  ]<--->[xcsaccount_t  ]
         */
 
        enum enum_xconsensus_result_code
        {
            enum_xconsensus_code_need_data          =  3, //usally ask trigger dedicated syn module to get data
            enum_xconsensus_code_async_back         =  2, //result may deliver at asynchronized way later
            enum_xconsensus_code_more_cmds          =  1, //usally happens at pipeline mode
            
            //above here to define code that not exact error but is problem to handle
            enum_xconsensus_code_successful         =  0,
            //below here to define real error
            
            enum_xconsensus_error_fail              =  -1, //unknow reason,just general fail
            enum_xconsensus_error_outofdate         =  -2, //data is too old or expired
            enum_xconsensus_error_cancel            =  -3, //stop/cancel consensus
            enum_xconsensus_error_timeout           =  -4, //timeout for consensus
            enum_xconsensus_error_duplicated        =  -5, //duplicated packet/events/blocks ...

            
            enum_xconsensus_error_not_inited        = -10, //module/object is not initialized yet
            enum_xconsensus_error_not_available     = -11, //module not registered or announce yet
            enum_xconsensus_error_not_handled       = -12, //not handle or processed
            enum_xconsensus_error_not_found         = -13, //not found target object/block...
            enum_xconsensus_error_not_authorized    = -14, //fail to test certificate/proof
            
            enum_xconsensus_error_undeliver_header  = -15, //header(QC) has missed content
            enum_xconsensus_error_undeliver_block   = -16, //block has missed content
    
            enum_xconsensus_error_fail_precheck     = -17,
            enum_xconsensus_error_fail_postcheck    = -18,
            enum_xconsensus_error_fail_addproposal  = -19,
            
            enum_xconsensus_error_bad_param         = -20,
            enum_xconsensus_error_bad_packet        = -21,
            enum_xconsensus_error_bad_account       = -22,
            enum_xconsensus_error_bad_signature     = -23,
            enum_xconsensus_error_bad_qc            = -24,
            enum_xconsensus_error_bad_header        = -25,
            enum_xconsensus_error_bad_recept        = -26,
            enum_xconsensus_error_bad_block         = -27,
            enum_xconsensus_error_bad_proposal      = -28,
            enum_xconsensus_error_bad_height        = -29,
            enum_xconsensus_error_bad_viewid        = -30,
            enum_xconsensus_error_bad_vote          = -31,
            enum_xconsensus_error_bad_commit        = -32,
            enum_xconsensus_error_bad_netaddress    = -33, //bad network address
            enum_xconsensus_error_bad_clock_cert    = -34, //bad xclock cert
            enum_xconsensus_error_not_found_drand   = -35,
            
            enum_xconsensus_error_wrong_view        = -40,
            enum_xconsensus_error_wrong_leader      = -41,
            
        };
        
        enum enum_xconsensus_object_type
        {
            enum_xconsensus_object_type_min = base::enum_xobject_type_xcons_min,
            enum_xconsensus_object_type_base     = enum_xconsensus_object_type_min + 0,
            enum_xconsensus_object_type_ledger   = enum_xconsensus_object_type_min + 1,
            enum_xconsensus_object_type_network  = enum_xconsensus_object_type_min + 2,
            enum_xconsensus_object_type_node     = enum_xconsensus_object_type_min + 3,
            enum_xconsensus_object_type_book     = enum_xconsensus_object_type_min + 4,
            enum_xconsensus_object_type_table    = enum_xconsensus_object_type_min + 5,
            
            //engine related object types
            enum_xconsensus_object_type_core     = enum_xconsensus_object_type_min + 10,
            enum_xconsensus_object_type_account  = enum_xconsensus_object_type_min + 11,
            enum_xconsensus_object_type_pacemaker= enum_xconsensus_object_type_min + 12,
            enum_xconsensus_object_type_context  = enum_xconsensus_object_type_min + 13,
            enum_xconsensus_object_type_driver   = enum_xconsensus_object_type_min + 14,

            enum_xconsensus_object_type_max = base::enum_xobject_type_xcons_max
        };
        
        //general consensus object
        class xcsobject_t : public base::xionode_t
        {
        public:
            static  const std::string   get_xclock_account_address(); //find the account address of global clock-contract
        protected:
            xcsobject_t(xcsobject_t & parent_object,base::enum_xobject_type obj_type);
            xcsobject_t(base::xcontext_t & _context,const int32_t target_thread_id,base::enum_xobject_type obj_type);
            virtual ~xcsobject_t();
        private:
            xcsobject_t();
            xcsobject_t(const xcsobject_t &);
            xcsobject_t & operator = (const xcsobject_t &);
        public:
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;
            
            //return specific error code(enum_xconsensus_result_code) to let caller know reason
            virtual int     verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child); //load and execute block at sanbox
            virtual bool    verify_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, std::string & result);
            virtual void    add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result);
            virtual bool    proc_vote_complate(base::xvblock_t * proposal_block);
            virtual bool    verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data);

            // for preproposal
            virtual bool    proc_preproposal(const xvip2_t & leader_xip,  uint64_t height, uint64_t viewid, uint64_t clock, uint32_t viewtoken, const std::string & msgdata);
            
            //send clock event to child objects
            virtual bool    fire_clock(base::xvblock_t & latest_clock_block,int32_t cur_thread_id,uint64_t timenow_ms);
            //dispatch view-change event to both upper(parent objects) and lower layers(child objects)
            virtual bool    fire_view(const std::string & target_account,const uint64_t new_view_id,const uint64_t global_clock,int32_t cur_thread_id,uint64_t timenow_ms);
            
            //send packet from this object to parent layers
            virtual bool    send_out(const xvip2_t & from_addr,const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms);
            
            //recv_in packet from this object to child layers
            virtual bool    recv_in(const xvip2_t & from_addr,const xvip2_t & to_addr,const base::xcspdu_t & packet,int32_t cur_thread_id,uint64_t timenow_ms);
            
            bool    fire_asyn_job(base::xfunction_t job_at_woker_thread,base::xfunction_t callback_to_this_thread);//execute job at worker pool but callback running at host thread of this object
  
        protected://guanrentee be called  at object'thread,triggered by push_event_up or push_event_down
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_pdu_event_up(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool  on_pdu_event_down(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
            //clock block always pass by higher layer to lower layer
            virtual bool  on_clock_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            //on_view_change event
            virtual bool  on_view_fire(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            //time cert block event
            virtual bool  on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            //time cert block event
            virtual bool  on_time_cert_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
        protected: //guanrentee be called  at object'thread,triggered by push_event_up or push_event_down
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool    on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool    on_event_down(const base::xvevent_t & event,xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
 
            //notify has child-node joined this node,errorcode refer enum_error_code ,return true when the event is handled
            virtual bool    on_child_node_join(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xionode_t* childnode) override;
            
            //notify this node that is joined into parent-node
            virtual bool    on_join_parent_node(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,const xvip2_t & alloc_address,const std::string & extra_data,xionode_t* from_parent) override;
            
        protected:
            base::xvcertauth_t*     get_vcertauth();
            base::xvblockstore_t*   get_vblockstore();
            base::xworkerpool_t*    get_workerpool();
            
            void                    set_vcertauth(base::xvcertauth_t* new_ptr);
            void                    set_vblockstore(base::xvblockstore_t* new_ptr);
            void                    set_workerpool(base::xworkerpool_t* new_ptr);
        private:
            base::xvcertauth_t  *   m_vcertauth_plugin;
            base::xvblockstore_t*   m_vblockstore_plugin;
            base::xworkerpool_t *   m_workerpool_plugin;
        };
                
        enum enum_xconsensus_pacemaker_type
        {
            enum_xconsensus_pacemaker_type_clock_cert   = 1,  //clock-block drive to make agreement for view#
            enum_xconsensus_pacemaker_type_timeout_cert = 2,  //timeout-certificaiton drive to make agreement for view#
            enum_xconsensus_pacemaker_type_view_cert    = 3,  //new-view protocol to make agreement for view#
        };
        
        //introduce some special events for core objects
        class xcscoreobj_t : public xcsobject_t,public base::xvaccount_t
        {
        public://wrap function to create engines that include pacemaker,context,driver, and attach into parent_object
            static xcscoreobj_t *  create_engine(xcscoreobj_t& parent_object,enum_xconsensus_pacemaker_type pacemaker_type);
            
        protected:
            xcscoreobj_t(xcscoreobj_t & parentobj,base::enum_xobject_type type);
            xcscoreobj_t(base::xcontext_t & _context,const int32_t target_thread_id,base::enum_xobject_type type,const std::string & account_addr);
            virtual ~xcscoreobj_t();
        private:
            xcscoreobj_t();
            xcscoreobj_t(const xcscoreobj_t &);
            xcscoreobj_t & operator = (const xcscoreobj_t &);
            
        public:
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;
            virtual bool    reset_xip_addr(const xvip2_t & new_addr) override; //note:it may reset all child'xip address
 
        protected:
            
            //call from higher layer to lower layer(child)
            virtual bool  on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
            //call from lower layer to higher layer(parent)
            virtual bool  on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_consensus_commit(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
        
            //call from lower layer to higher layer(parent)
            virtual bool  on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
            //call from lower layer to higher layer(parent)
            virtual bool  on_certificate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
            
        public: //help function and allow called from outside
            
            //proposal_start_event always go down from higher layer
            bool    fire_proposal_start_event(base::xvblock_t*proposal_block);//for leader start a proposal
            bool    fire_proposal_start_event(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block);//just for replica to update information
 
        protected:
            //proposal_finish_event always go up from lower layer
            bool    fire_proposal_finish_event(base::xvblock_t* target_proposal,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block,base::xvblock_t* latest_proposal_block);
            bool    fire_proposal_finish_event(const int errcode,const std::string & err_detail,base::xvblock_t* target_proposal,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block,base::xvblock_t* latest_proposal_block);

            //consensus_commit_event always go up from lower layer
            bool    fire_consensus_commit_event(base::xvblock_t* target_commit,base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block);
            
            //consensus_update_event always go up from lower layer
            bool    fire_consensus_update_event_up(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block);
            bool    fire_consensus_update_event_down(base::xvblock_t* latest_commit_block,base::xvblock_t* latest_lock_block,base::xvblock_t* latest_cert_block);
            
            //certifcate_finish always go up from lower layer
            bool    fire_replicate_finish_event(base::xvblock_t * target_block);
            //certifcate_finish always go up from lower layer
            bool    fire_certificate_finish_event(base::xvqcert_t* target_cert);
            
            //pdu event related help function
            bool    fire_pdu_event_up(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block);
            bool    fire_pdu_event_up(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block,const std::string & vblock_cert_bin,const std::string & vlatest_clock_cert);
            bool    fire_pdu_event_down(const uint8_t msg_type,const std::string & msg_content,const uint16_t msg_nonce,const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t* for_block);
            
        protected:
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool  on_event_up(const base::xvevent_t & event,xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool  on_event_down(const base::xvevent_t & event,xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            virtual int   get_default_msg_ttl()  const {return 12;} //no more than 12 hop for p2p
        };
    }; //end of namespace of xconsensus
};//end of namespace of top
