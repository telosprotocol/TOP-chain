#pragma once

#include <string>
#include <xvledger/xvblockstore.h>

namespace top {
namespace mock {

        // only some simple apis for test
        class xmem_blockstore_t : public base::xvblockstore_t
        {
            virtual std::string         get_store_path() const override {return {};}
        public:
            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_genesis_connected_block(const base::xvaccount_t & account,bool ask_full_search = true,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvbindex_t> get_latest_genesis_connected_index(const base::xvaccount_t & account,bool ask_full_search = true,const int atag = 0) {xassert(false);return nullptr;}

            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_full_block(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return nullptr;}
            virtual base::xblock_mptrs          get_latest_blocks(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return {};}
            virtual uint64_t get_latest_committed_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t get_latest_locked_block_height(const base::xvaccount_t & account, const int atag = 0) {xassert(false);return 0;}
            virtual uint64_t get_latest_cert_block_height(const base::xvaccount_t & account, const int atag = 0) {xassert(false);return 0;}
            virtual uint64_t get_latest_full_block_height(const base::xvaccount_t & account, const int atag = 0) {xassert(false);return 0;}
            virtual uint64_t get_latest_connected_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t get_latest_genesis_connected_block_height(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return 0;}
            virtual uint64_t get_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t update_get_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t update_get_db_latest_cp_connected_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t get_latest_executed_block_height(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return 0;}
            virtual uint64_t get_lowest_executed_block_height(const base::xvaccount_t & account,const int atag = 0)  {xassert(false);return 0;}
            virtual uint64_t get_latest_deleted_block_height(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return 0;}
            virtual bool                  set_latest_executed_info(const base::xvaccount_t & account,uint64_t height)  {xassert(false);return true;}

            //mostly used for query cert-only block,note:return any block at target height if viewid is 0
            virtual base::xblock_vector         query_block(const base::xvaccount_t & account,const uint64_t height,const int atag = 0) {xassert(false);return {};}
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,const int atag = 0)  {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,const int atag = 0)  {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,const int atag = 0) {xassert(false);return nullptr;}


        public://note:load_block/store/delete may work with both persist db and cache layer

            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual base::xblock_vector         load_block_object(const base::xvaccount_t & account,const uint64_t height,const int atag = 0)  {xassert(false);return {};}
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load,const int atag = 0){xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,bool ask_full_load,const int atag = 0) {xassert(false);return nullptr;}
            //virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account, const uint64_t height, bool ask_full_load, const int atag = 0) = 0;
            virtual std::vector<base::xvblock_ptr_t> load_block_object(const std::string & tx_hash,const base::enum_transaction_subtype type,const int atag = 0)  {xassert(false);return {};}

            virtual bool                  load_block_input(const base::xvaccount_t & account,base::xvblock_t* block,const int atag = 0)  {return true;}
            virtual bool                  load_block_output(const base::xvaccount_t & account,base::xvblock_t* block,const int atag = 0)  {return true;}

            virtual bool                  store_block(const base::xvaccount_t & account,base::xvblock_t* block,const int atag = 0)  {
                auto account_map_pos  = m_blocks.emplace(account.get_account(),std::map<uint64_t,base::xvblock_t*>());
                auto & height_map = account_map_pos.first->second;
                if (height_map.end() == height_map.find(block->get_height())) {
                    block->add_ref();
                    height_map.emplace(block->get_height(),block);
                }                
            }
            virtual bool                  delete_block(const base::xvaccount_t & account,base::xvblock_t* block,const int atag = 0) {xassert(false);return false;}

            //better performance for batch operations
            virtual bool                  store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks,const int atag = 0) {xassert(false);return false;}

            virtual base::xauto_ptr<base::xvbindex_t> recover_and_load_commit_index(const base::xvaccount_t & account, uint64_t height) {xassert(false);return nullptr;}

        public://note:load_index may work with both persist db and cache layer
            virtual base::xvbindex_vector       load_block_index(const base::xvaccount_t & account,const uint64_t height,const int atag = 0) {xassert(false);return {};}
            virtual base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,const int atag = 0) {xassert(false);return nullptr;}
            virtual base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,const int atag = 0) {xassert(false);return nullptr;}

        public:
            //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
            virtual bool                  clean_caches(const base::xvaccount_t & account,const int atag = 0) {xassert(false);return false;}
        public:
            //execute_block will move to statestore soon
            //execute block and update state of acccount
            //note: block must be committed and connected
            virtual base::xauto_ptr<base::xvblock_t>    get_block_by_hash(const std::string& hash) {xassert(false);return nullptr;}
        public:
            // check if genesis block exist
            virtual bool exist_genesis_block(const base::xvaccount_t & account, const int atag = 0) {xassert(false);return false;}
            virtual base::xauto_ptr<base::xvblock_t> create_genesis_block(const base::xvaccount_t & account, std::error_code & ec) {xassert(false);return nullptr;}
            virtual void register_create_genesis_callback(std::function<base::xauto_ptr<base::xvblock_t>(base::xvaccount_t const &, std::error_code &)> cb) {xassert(false);}

        public:
            // genesis connected  blocks
            virtual bool        set_genesis_height(const base::xvaccount_t & account, const std::string &height) {xassert(false);return false;}
            virtual const std::string    get_genesis_height(const base::xvaccount_t & account) {xassert(false);return {};}
            virtual bool        set_block_span(const base::xvaccount_t & account, const uint64_t height,  const std::string &span)  {xassert(false);return false;}
            virtual bool        delete_block_span(const base::xvaccount_t & account, const uint64_t height)  {xassert(false);return false;}
            virtual const std::string get_block_span(const base::xvaccount_t & account, const uint64_t height) {xassert(false);return {};}

        private:
            // < account: <height,block*> > sort from lower height to higher,and the first one block is genesis block
            std::map< std::string,std::map<uint64_t,base::xvblock_t*> > m_blocks;
        };

}
}
