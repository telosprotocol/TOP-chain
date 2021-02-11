// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <mutex>
#include "xbase/xvledger.h"
#include "xvunithub.h"

namespace top
{
    namespace store
    {
        //note: layers for store :  [xvblock-store] --> [xstore] -->[xdb]
        //vblockstore support split table-block into unit blocks
        class xvtablestore_impl : public xvblockstore_impl
        {
        public:
            xvtablestore_impl(const std::string & blockstore_path,xstore_face_t&  _persist_db,base::xcontext_t & _context,const int32_t target_thread_id);
        protected:
            virtual ~xvtablestore_impl();
        private:
            xvtablestore_impl();
            xvtablestore_impl(const xvtablestore_impl &);
            xvtablestore_impl & operator = (const xvtablestore_impl &);

        public: //return raw ptr with added reference,caller respond to release it after that.

            virtual bool                store_block(base::xvblock_t* block)  override; //return false if fail to store
            virtual bool                store_block(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) override;
        private:
            bool                        store_tableblock_units(base::xvblock_t* block);
        };
    } //end of namespace of vstore
} //end of namespace of top
