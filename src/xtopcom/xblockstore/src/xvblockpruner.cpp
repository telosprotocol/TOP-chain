// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xvblockpruner.h"

namespace top
{
    namespace store
    {
        xvblockprune_impl::xvblockprune_impl(base::xvdbstore_t & xdb_api)
        {
            m_xvdb_ptr = &xdb_api;
        }
    
        xvblockprune_impl::~xvblockprune_impl()
        {
        }
    
        bool  xvblockprune_impl::recycle(const base::xvbindex_t * block) //recyle one block
        {
            if(NULL == block)
               return false;
               
            return false;
        }
    
        bool  xvblockprune_impl::recycle(const std::vector<base::xvbindex_t*> & mblocks) //recycle multiple blocks
        {
            for(auto block : mblocks)
            {
                recycle(block);
            }
            return true;
        }
    
        bool  xvblockprune_impl::recycle(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)//recylce any qualified blocks under account
        {
            //only prune new blocks with new key of db
            if(0 == account_meta._lowest_vkey2_block_height)
                return false;
            
            //only prune blocks before full_block
            if(0 == account_meta._highest_full_block_height)
                return false;
      
            if(account_obj.is_unit_address())
                return recycle_unit(account_obj,account_meta);
            else
                return recycle_table(account_obj,account_meta);
        }
        
        bool  xvblockprune_impl::recycle_table(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            return false;
        }
    
        bool  xvblockprune_impl::recycle_unit(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            //[lower_bound_height,upper_bound_height]
            const uint64_t upper_bound_height = account_meta._highest_full_block_height - 1;
            const uint64_t lower_bound_height = std::max(account_meta._lowest_vkey2_block_height,account_meta._highest_deleted_block_height) + 1;
            if(lower_bound_height >= upper_bound_height)
                return false;
            
            if((upper_bound_height - lower_bound_height) < 21) //collect big range for each prune op
                return false;
            
            /*
            const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_object_key(account_obj,lower_bound_height);
            const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_object_key(account_obj,upper_bound_height);
            if(get_xvdb()->delete_range(begin_delete_key, end_delete_key))
            {
                account_meta._highest_deleted_block_height = upper_bound_height;
                return true;
            }
            xassert(0);
            */
            return false;
        }
    }
}
