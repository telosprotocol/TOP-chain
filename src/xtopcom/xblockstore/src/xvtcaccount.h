// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xvblock.h"
#include "xbase/xvledger.h"
#include "xvblockhub.h"
#include "xvunithub.h"
#include "xblockstore/xblockstore_face.h"

// TC(timeout cert), QC(quorum cert)
// view_id is not used for timeout cert block, since it contains no viewid
// and the qc is aggregated by different node
// use commit height as the highest height for the tc height
namespace top
{
    namespace store
    {
        //each account has own virtual store
        class xtcaccount_t : public xchainacct_t
        {
        public:
            xtcaccount_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore);

        protected:
            virtual ~xtcaccount_t();
        private:
            xtcaccount_t();
            xtcaccount_t(const xtcaccount_t &);
            xtcaccount_t & operator = (const xtcaccount_t &);

        public://indicated the last block who is connected allway to genesis block
            bool        init() override;
            bool        close(bool force_async) override;

        public://return raw ptr with added reference,caller respond to release it after that.

            //one api to get latest_commit/latest_lock/latest_cert for better performance
            bool                    get_latest_blocks_list(base::xvblock_t* & cert_block,base::xvblock_t* & lock_block,base::xvblock_t* & commit_block);

        public://return raw ptr with added reference,caller respond to release it after that

            bool                    store_block(base::xvblock_t* block) override; //update old one or insert as new
            bool                    delete_block(base::xvblock_t* block) override;//return error code indicate what is result
            bool                    delete_block(uint64_t height) override;//return error code indicate what is result

            virtual bool                    store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks) override; //better performance

            virtual bool                    execute_block(base::xvblock_t* block) override; //execute block and update state of acccount

        protected:
            bool              save_to_xdb(base::xvblock_t* this_block);
            bool              save_block(base::xvblock_t* this_block); //save block to persisted storage
            //to connect prev block, load_block may call load_block again to get prev-block, reenter_allow_count decide how many times can reenter
            base::xvblock_t*  load_block(const uint64_t height,int reenter_allow_count);       //load block from persisted storage
            bool      clean_blocks(const size_t keep_blocks_count) override;  //not allow cache too much blocks

            //query block from cached map,return raw ptr without adding reference
            base::xvblock_t*  query_latest_block(base::enum_xvblock_flag request_flag);

            bool      connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reload_allow_count) override;

        private:
            void              close_blocks(); //clean all cached blocks
            void              set_meta_info(base::xvblock_t* this_block);
            void              set_meta_info(uint64_t height, const std::string& hash);
            void              set_block_flag(base::xvblock_t* this_block);
        };

    };//end of namespace of vstore
};//end of namespace of top
