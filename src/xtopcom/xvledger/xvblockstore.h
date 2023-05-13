// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvaccount.h"
#include "xvblock.h"
#include "xvbindex.h"
#include "xvtransact.h"

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
            virtual xauto_ptr<xvblock_t>  get_genesis_block(const xvaccount_t & account,const int atag = 0)           = 0;//genesis block
            virtual xauto_ptr<xvblock_t>  get_latest_cert_block(const xvaccount_t & account,const int atag = 0)       = 0;//highest view# for any status
            virtual xauto_ptr<xvblock_t>  get_latest_locked_block(const xvaccount_t & account,const int atag = 0)     = 0;//block with locked status
            virtual xauto_ptr<xvblock_t>  get_latest_committed_block(const xvaccount_t & account,const int atag = 0)  = 0;//block with committed status
            virtual xauto_ptr<xvblock_t>  get_latest_connected_block(const xvaccount_t & account,const int atag = 0)  = 0;//block connected to genesis or fullblock
            virtual xauto_ptr<xvblock_t>  get_latest_genesis_connected_block(const xvaccount_t & account,bool ask_full_search = true,const int atag = 0) = 0; //block has connected to genesis
            virtual xauto_ptr<xvbindex_t> get_latest_genesis_connected_index(const xvaccount_t & account,bool ask_full_search = true,const int atag = 0) = 0; //block has connected to genesis

            virtual xauto_ptr<xvblock_t>  get_latest_committed_full_block(const xvaccount_t & account,const int atag = 0)  = 0; // full block with committed status, genesis is a full block
            virtual xblock_mptrs          get_latest_blocks(const xvaccount_t & account,const int atag = 0)      = 0; //better performance for batch operations
            virtual uint64_t get_latest_committed_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_locked_block_height(const base::xvaccount_t & account, const int atag = 0) = 0;
            virtual uint64_t get_latest_cert_block_height(const base::xvaccount_t & account, const int atag = 0) = 0;
            virtual uint64_t get_latest_connected_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_full_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_genesis_connected_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t update_get_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t update_get_db_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_executed_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_lowest_executed_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual uint64_t get_latest_deleted_block_height(const xvaccount_t & account,const int atag = 0) = 0;
            virtual bool                  set_latest_executed_info(const xvaccount_t & account,uint64_t height) = 0;

            //mostly used for query cert-only block,note:return any block at target height if viewid is 0
            virtual xblock_vector         query_block(const xvaccount_t & account,const uint64_t height,const int atag = 0) = 0;//might mutiple certs at same height
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,const uint64_t viewid,const int atag = 0) = 0;
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,const std::string & blockhash,const int atag = 0) = 0;
            virtual xauto_ptr<xvblock_t>  query_block(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block,const int atag = 0) = 0; //just return the highest viewid of matched flag


        public://note:load_block/store/delete may work with both persist db and cache layer

            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual xblock_vector         load_block_object(const xvaccount_t & account,const uint64_t height,const int atag = 0) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load,const int atag = 0) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load,const int atag = 0) = 0;
            virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block,bool ask_full_load,const int atag = 0) = 0; //just return the highest viewid of matched flag
            //virtual xauto_ptr<xvblock_t>  load_block_object(const xvaccount_t & account, const uint64_t height, bool ask_full_load, const int atag = 0) = 0;
            virtual std::vector<base::xvblock_ptr_t> load_block_object(const std::string & tx_hash,const enum_transaction_subtype type,const int atag = 0) = 0;

            virtual bool                  load_block_input(const xvaccount_t & account,xvblock_t* block,const int atag = 0) = 0;
            virtual bool                  load_block_output(const xvaccount_t & account,xvblock_t* block,const int atag = 0) = 0;
            virtual bool                  load_block_output_offdata(const xvaccount_t & account,xvblock_t* block,const int atag = 0) = 0;

            virtual bool                  store_block(const xvaccount_t & account,xvblock_t* block,const int atag = 0)  = 0;
            virtual bool                  delete_block(const xvaccount_t & account,xvblock_t* block,const int atag = 0) = 0;

            //better performance for batch operations
            virtual bool                  store_blocks(const xvaccount_t & account,std::vector<xvblock_t*> & batch_store_blocks,const int atag = 0) = 0;

            virtual base::xauto_ptr<base::xvbindex_t> recover_and_load_commit_index(const base::xvaccount_t & account, uint64_t height) = 0;

        public://note:load_index may work with both persist db and cache layer
            virtual xvbindex_vector       load_block_index(const xvaccount_t & account,const uint64_t height,const int atag = 0) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,const uint64_t viewid,const int atag = 0) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,const std::string & blockhash,const int atag = 0) = 0;
            virtual xauto_ptr<xvbindex_t> load_block_index(const xvaccount_t & account,const uint64_t height,enum_xvblock_flag required_block,const int atag = 0) = 0;//just return the highest viewid of matched flag

        public:
            //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
            virtual bool                  clean_caches(const xvaccount_t & account,const int atag = 0) = 0;
        public:
            //execute_block will move to statestore soon
            //execute block and update state of acccount
            //note: block must be committed and connected
            virtual base::xauto_ptr<base::xvblock_t>    get_block_by_hash(const std::string& hash) = 0;
        public:
            // check if genesis block exist
            virtual bool exist_genesis_block(const base::xvaccount_t & account, const int atag = 0) = 0;
            virtual base::xauto_ptr<base::xvblock_t> create_genesis_block(const base::xvaccount_t & account, std::error_code & ec) = 0;
            virtual void register_create_genesis_callback(std::function<base::xauto_ptr<base::xvblock_t>(base::xvaccount_t const &, std::error_code &)> cb) = 0;

        public:
            // genesis connected  blocks
            virtual bool        set_genesis_height(const base::xvaccount_t & account, const std::string &height) = 0;
            virtual const std::string    get_genesis_height(const base::xvaccount_t & account) = 0;
            virtual bool        set_block_span(const base::xvaccount_t & account, const uint64_t height,  const std::string &span) = 0;
            virtual bool        delete_block_span(const base::xvaccount_t & account, const uint64_t height) = 0;
            virtual const std::string get_block_span(const base::xvaccount_t & account, const uint64_t height) = 0;

        public: // unit related apis
            virtual bool    store_units(base::xvblock_t* table_block, std::vector<base::xvblock_ptr_t> const& units) {return false;}
            virtual bool    store_units(base::xvblock_t* table_block) {return false;}
            virtual bool    store_unit(const base::xvaccount_t & account,base::xvblock_t* unit) {return false;}
            virtual base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid) {return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash) {return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height) {return nullptr;}            
            virtual bool    exist_unit(const base::xvaccount_t & account) const {return false;}     
            virtual bool    delete_unit(const xvaccount_t & account,xvblock_t* block) {return false;}

        protected:
            //only allow remove flag within xvblockstore_t
            void                          remove_block_flag(xvblock_t* to_block, enum_xvblock_flag flag);
            //using xiobject_t::add_ref;
            //using xiobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
