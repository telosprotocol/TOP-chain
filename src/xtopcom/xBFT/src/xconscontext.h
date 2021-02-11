// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xconsengine.h"

namespace top
{
    namespace xconsensus
    {
        class xBFTcontext_t : public xcscontext_t
        {
        public:
            xBFTcontext_t(xcscoreobj_t&  parent_object);
        protected:
            virtual ~xBFTcontext_t();
        private:
            xBFTcontext_t();
            xBFTcontext_t(const xBFTcontext_t &);
            xBFTcontext_t & operator = (const xBFTcontext_t &);
 
        protected:
            virtual bool  on_proposal_start(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from higher layer to lower layer(child)
            

            virtual bool  on_proposal_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
            virtual bool  on_consensus_update(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
            virtual bool  on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;//call from lower layer to higher layer(parent)
            
        protected:
            virtual  bool   do_update(base::xvblock_t * new_block); //update blocks and connect each other
            virtual  bool   do_execute(base::xvbnode_t * new_node); //execute 3-chain or 2-chain rules
            virtual  bool   do_submit(base::xvblock_t* highest_cert_block,base::xvblock_t* highest_proposal);//submit to upper layer
            virtual  bool   do_clean(base::xvblock_t* highest_cert_block,base::xvblock_t* highest_proposal);//clean expired or cancel one
            
        protected:
            bool     safe_check_for_block(base::xvblock_t * _block);//any block must be a valid certified block at context layer
            bool     safe_check_for_cert_block(base::xvblock_t  * _block);//check and compare with latest_commit
 
            bool     update_lock_block(base::xvblock_t * new_node);
            bool     update_commit_block(base::xvblock_t * new_node);
            
            inline base::xvblock_t*       get_latest_commit_block() const {return m_latest_commit_block;}
            inline base::xvblock_t*       get_latest_lock_block()   const {return m_latest_lock_block;}
        private:
            base::xvblock_t*              m_latest_commit_block;
            base::xvblock_t*              m_latest_lock_block;
            std::map<std::string,base::xvbnode_t*>  m_hash_pool;  //mapping block-hash ->node
            std::set<uint64_t>                      m_view_pool;  //test whether cert of viewid has been handle
            //note: allow multile cert block at same height after lock,but each view# just allow one cert block
            //note: highest qc and highest proposal managed by driver layer
        };
    }; //end of namespace of xconsensus
};//end of namespace of top
