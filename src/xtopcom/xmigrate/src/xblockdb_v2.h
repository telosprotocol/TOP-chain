// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>
#include "xvledger/xvbindex.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvdbstore.h"
#include "xvledger/xvaccount.h"

namespace top
{
    namespace base
    {
        class xblockdb_v2_t : public base::xobject_t
        {
        public:
            xblockdb_v2_t(base::xvdbstore_t* xvdb_ptr);
        public:
            virtual ~xblockdb_v2_t();
        private:
            xblockdb_v2_t();
            xblockdb_v2_t(xblockdb_v2_t &&);
            xblockdb_v2_t(const xblockdb_v2_t &);
            xblockdb_v2_t & operator = (const xblockdb_v2_t &);
        public:
            base::xvdbstore_t*      get_xdbstore() const {return m_xvdb_ptr;}
            
        public:
            bool                    load_blocks(const base::xvaccount_t & account,const uint64_t target_height, std::vector<xobject_ptr_t<base::xvblock_t>> & blocks);
            static base::xauto_ptr<base::xvactmeta_t>  get_meta(base::xvdbstore_t* dbstore, const base::xvaccount_t & account);
            static base::xauto_ptr<base::xvactmeta_t>  get_v3_meta(base::xvdbstore_t* dbstore, const base::xvaccount_t & account);
            static uint64_t                            get_genesis_height(xvdbstore_t* dbstore, const base::xvaccount_t & account);
            static uint64_t                            get_v3_genesis_height(xvdbstore_t* dbstore, const base::xvaccount_t & account);
        protected:
            xobject_ptr_t<base::xvbindex_t>                 read_index_from_db(const std::string & index_db_key_path);
            std::vector<xobject_ptr_t<base::xvbindex_t>>    read_indexs(const base::xvaccount_t & account,const uint64_t target_height);
            bool    load_block_object(base::xvbindex_t* index_ptr);
            bool    load_block_input(base::xvbindex_t* target_index);
            bool    load_block_output(base::xvbindex_t* target_index);            
        private:
             base::xvdbstore_t*  m_xvdb_ptr;
        };    
    }//end of namespace of base
}//end of namespace top
