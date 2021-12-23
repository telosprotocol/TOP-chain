// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvaccount.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvledger.h"
#include <atomic>
namespace top
{
    namespace store
    {
        //manage to prune blocks
        class xvblockprune_impl : public base::xblockrecycler_t
        {
            enum
            {
               enum_reserved_blocks_count           = 8,  //reserved blocks even it is qualified to recycel
               enum_min_batch_recycle_blocks_count  = 64, //min blocks to recyce each time
            };
        public:
            xvblockprune_impl(base::xvdbstore_t & xdb_api);
        protected:
            virtual ~xvblockprune_impl();
        private: 
            xvblockprune_impl();
            xvblockprune_impl(xvblockprune_impl &&);
            xvblockprune_impl(const xvblockprune_impl &);
            xvblockprune_impl & operator = (const xvblockprune_impl &);
            
        public:
            virtual bool   recycle(const base::xvbindex_t * block) override;//recyle one block
            virtual bool   recycle(const std::vector<base::xvbindex_t*> & mblocks) override;//recycle multiple blocks
            virtual bool   recycle(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta) override;//recylce any qualified blocks under account
            virtual bool   watch(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj) override; // just for table
            virtual bool   unwatch(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj) override; // just for table
            virtual bool   refresh(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj, const uint64_t permit_prune_upper_boundary) override; // just for table

        protected:
            base::xvdbstore_t *  get_xvdb() const {return m_xvdb_ptr;}
            
            //manage to prune contract blocks
            bool  recycle_contract(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta);
            //mange to prune table blocks
            bool  recycle_table(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta);
            //mange to prune unit blocks
            bool  recycle_unit(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta);
            bool  recycle_drand_timer(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta);

        private:
            bool get_prune_boundary(const base::xvaccount_t & account_obj, uint64_t &height);
        private:
            base::xvdbstore_t *  m_xvdb_ptr{NULL};
            std::map<std::string, std::map<chainbase::enum_xmodule_type, uint64_t>> m_prune_boundary;
            std::mutex m_lock;
        };
    
    }
}
