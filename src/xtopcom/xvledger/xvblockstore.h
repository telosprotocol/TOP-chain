// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvaccount.h"
#include "xvblock.h"
#include "xvbindex.h"
#include "xvtransaction.h"

namespace top
{
    namespace base
    {
        class xblock_mptrs
        {
        public:
            xblock_mptrs()
            {
                m_latest_cert     = nullptr;
                m_latest_lock     = nullptr;
                m_latest_commit   = nullptr;
                m_latest_executed = nullptr;
            }
            xblock_mptrs(std::nullptr_t)
            {
                m_latest_cert     = nullptr;
                m_latest_lock     = nullptr;
                m_latest_commit   = nullptr;
                m_latest_executed = nullptr;
            }
            xblock_mptrs(xvblock_t* latest_cert,xvblock_t* latest_lock,xvblock_t* latest_commit)
            {
                m_latest_cert       = latest_cert;
                m_latest_lock       = latest_lock;
                m_latest_commit     = latest_commit;
                m_latest_executed   = nullptr;
            }
            xblock_mptrs(xvblock_t* latest_cert,xvblock_t* latest_lock,xvblock_t* latest_commit,xvblock_t* latest_executed)
            {
                m_latest_cert       = latest_cert;
                m_latest_lock       = latest_lock;
                m_latest_commit     = latest_commit;
                m_latest_executed   = latest_executed;
            }
            xblock_mptrs(xblock_mptrs && moved)
            {
                m_latest_cert       = moved.m_latest_cert;
                m_latest_lock       = moved.m_latest_lock;
                m_latest_commit     = moved.m_latest_commit;
                m_latest_executed   = moved.m_latest_executed;
                
                moved.m_latest_cert     = nullptr;
                moved.m_latest_lock     = nullptr;
                moved.m_latest_commit   = nullptr;
                moved.m_latest_executed = nullptr;
            }
            ~xblock_mptrs()//auto release those references
            {
                if(m_latest_cert != nullptr)
                    m_latest_cert->release_ref();
                if(m_latest_lock != nullptr)
                    m_latest_lock->release_ref();
                if(m_latest_commit != nullptr)
                    m_latest_commit->release_ref();
                if(m_latest_executed != nullptr)
                    m_latest_executed->release_ref();
            }
        private:
            xblock_mptrs(const xblock_mptrs &);
            xblock_mptrs & operator = (const xblock_mptrs &);
            xblock_mptrs & operator = (xblock_mptrs &&);
        public:
            inline xvblock_t*   get_latest_cert_block()       const noexcept {return m_latest_cert;}
            inline xvblock_t*   get_latest_locked_block()     const noexcept {return m_latest_lock;}
            inline xvblock_t*   get_latest_committed_block()  const noexcept {return m_latest_commit;}
            inline xvblock_t*   get_latest_executed_block()   const noexcept {return m_latest_executed;}
        private:
            xvblock_t*  m_latest_cert;
            xvblock_t*  m_latest_lock;
            xvblock_t*  m_latest_commit;
            xvblock_t*  m_latest_executed;
        };
        
        class xblock_vector
        {
        public:
            xblock_vector()
            {
            }
            xblock_vector(std::nullptr_t)
            {
            }
            xblock_vector(std::vector<xvblock_t*> & blocks)
            {
                m_vector = blocks;//transfer owner of ptr
                blocks.clear();
            }
            xblock_vector(std::vector<xvblock_t*> && blocks)
            {
                m_vector = blocks; //transfer owner of ptr
                blocks.clear();
            }
            xblock_vector(xblock_vector && moved)
            {
                m_vector = moved.m_vector;//transfer owner of ptr
                moved.m_vector.clear();
            }
            ~xblock_vector()
            {
                for(auto it : m_vector)
                {
                    if(it != nullptr)
                        it->release_ref();
                }
            }
        private:
            xblock_vector(const xblock_vector &);
            xblock_vector & operator = (const xblock_vector &);
            xblock_vector & operator = (xblock_vector && moved);
        public:
            const std::vector<xvblock_t*> &  get_vector() const {return m_vector;}
        private:
            std::vector<xvblock_t*> m_vector;
        };
 
        //manage/connect "virtual block" with "virtual header",usally it implement based on xvblocknode_t
        //note: each chain may has own block store by assined different store_path at DB/disk
        class xvblockstore_t : public xiobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvblockstore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}

        protected:
            xvblockstore_t(base::xcontext_t & _context,const int32_t target_thread_id);
            virtual ~xvblockstore_t();
            
        private:
            xvblockstore_t();
            xvblockstore_t(xvblockstore_t &&);
            xvblockstore_t(const xvblockstore_t &);
            xvblockstore_t & operator = (const xvblockstore_t &);
        public:
            //caller need to cast (void*) to related ptr
            virtual void*                query_interface(const int32_t _enum_xobject_type_) override;
            //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
            virtual std::string          get_store_path() const = 0;//each store may has own space at DB/disk
            
        public://just search from cached blocks for below api (without persist db involved)
            
            //and return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
            virtual xauto_ptr<xvblock_t>  get_genesis_block(const xvaccount_t & account)           = 0;//genesis block
            virtual xauto_ptr<xvblock_t>  get_latest_cert_block(const xvaccount_t & account)       = 0;//highest view# for any status
            virtual xauto_ptr<xvblock_t>  get_latest_locked_block(const xvaccount_t & account)     = 0;//block with locked status
            virtual xauto_ptr<xvblock_t>  get_latest_committed_block(const xvaccount_t & account)  = 0;//block with committed status
            virtual xauto_ptr<xvblock_t>  get_latest_executed_block(const xvaccount_t & account)   = 0;//block with executed status
            virtual xauto_ptr<xvblock_t>  get_latest_connected_block(const xvaccount_t & account)  = 0;//block connected to genesis or fullblock
            virtual xauto_ptr<xvblock_t>  get_latest_genesis_connected_block(const xvaccount_t & account) = 0; //block has connected to genesis
            
            virtual xauto_ptr<xvblock_t>  get_latest_full_block(const xvaccount_t & account)  = 0; //block has full state,genesis is a full block
            virtual xauto_ptr<xvblock_t>  get_latest_committed_full_block(const xvaccount_t & account)  = 0; // full block with committed status, genesis is a full block           
            virtual xblock_mptrs          get_latest_blocks(const xvaccount_t & account)      = 0; //better performance for batch operations
            
            //mostly used for query cert-only block,note:return any block at target height if viewid is 0
            virtual xblock_vector         query_block(const xvaccount_t & account,const uint64_t height) = 0;//might mutiple certs at same height
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,const uint64_t viewid) = 0;
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,const std::string & blockhash) = 0;
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block) = 0; //just return the highest viewid of matched flag
            
            
        public://note:load_block/store/delete may work with both persist db and cache layer
        
            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual xblock_vector         load_block_object(const xvaccount_t & account,const uint64_t height) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block,bool ask_full_load) = 0; //just return the highest viewid of matched flag
            
            virtual bool                  load_block_input(const xvaccount_t & account,xvblock_t* block) = 0;
            virtual bool                  load_block_output(const xvaccount_t & account,xvblock_t* block) = 0;
            //load xvboffdata_t and set into xvblock_t
            virtual bool                  load_block_offdata(const xvaccount_t & account,xvblock_t* block) = 0;
            
            virtual bool                  store_block(const xvaccount_t & account,xvblock_t* block)  = 0;
            virtual bool                  delete_block(const xvaccount_t & account,xvblock_t* block) = 0;
          
            //better performance for batch operations
            virtual bool                  store_blocks(const xvaccount_t & account,std::vector<xvblock_t*> & batch_store_blocks) = 0;

        public://note:load_index may work with both persist db and cache layer
            virtual xvbindex_vector       load_block_index(const xvaccount_t & account,const uint64_t height) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,const uint64_t viewid) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,const std::string & blockhash) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block) = 0;//just return the highest viewid of matched flag
            
        public:
            //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
            virtual bool                  clean_caches(const xvaccount_t & account) = 0;
            //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
            virtual bool                  reset_cache_timeout(const xvaccount_t & account,const uint32_t max_idle_time_ms) = 0;
            
        public:
            //execute_block will move to statestore soon
            //execute block and update state of acccount
            //note: block must be committed and connected
            virtual bool                 execute_block(const base::xvaccount_t & account,base::xvblock_t* block) = 0;
            virtual xvtransaction_store_ptr_t  query_tx(const std::string & txhash, enum_transaction_subtype type) = 0;

        public:
            //check if genesis block exist 
            virtual bool                  exist_genesis_block(const base::xvaccount_t & account) = 0;
            
        protected:
            //only allow remove flag within xvblockstore_t
            void                          remove_block_flag(xvblock_t* to_block, enum_xvblock_flag flag);
            //using xiobject_t::add_ref;
            //using xiobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
