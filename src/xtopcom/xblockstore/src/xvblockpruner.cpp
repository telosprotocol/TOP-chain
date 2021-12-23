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
            xassert(enum_reserved_blocks_count > 0);
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
            bool result = false;
            for(auto block : mblocks)
            {
                if(recycle(block))
                    result = true;
            }
            return result;
        }
    
        bool  xvblockprune_impl::recycle(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)//recylce any qualified blocks under account
        {
            //only prune blocks before full_block
            if(0 == account_meta._highest_full_block_height)
                return false;

            auto zone_id = account_obj.get_zone_index();
            // if consensus zone
            if ((zone_id == base::enum_chain_zone_zec_index) || (zone_id == base::enum_chain_zone_beacon_index)) {
                return false;
            }
            
            if(account_obj.is_unit_address())
                return recycle_unit(account_obj,account_meta);
            else if(account_obj.is_table_address())
                return recycle_table(account_obj,account_meta);
            else if(account_obj.is_contract_address())
                return recycle_contract(account_obj,account_meta);
            
            return false;
        }
    
        bool  xvblockprune_impl::recycle_contract(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            return false;
        }
        
        bool  xvblockprune_impl::recycle_table(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            if(account_meta._highest_full_block_height <= enum_reserved_blocks_count) //start prune at least > 8
                return false;
    
            return false;//XTODO,disable table_prune temporary
            
            //[lower_bound_height,upper_bound_height)
            const uint64_t upper_bound_height = account_meta._highest_full_block_height - enum_reserved_blocks_count;
            const uint64_t lower_bound_height = std::max(account_meta._lowest_vkey2_block_height,account_meta._highest_deleted_block_height) + 1;
            
            xinfo("xvblockprune_impl::recycle account %s, upper %llu, lower %llu, connect_height %llu", account_obj.get_address().c_str(),
                upper_bound_height, lower_bound_height, account_meta._highest_connect_block_height);
            
            if(lower_bound_height >= upper_bound_height)
                return false;
            else if((upper_bound_height - lower_bound_height) < 5000)
                return false;//collect big range for each prune op as performance consideration
            
            const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,lower_bound_height);
            const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,upper_bound_height - 5000);
            if(get_xvdb()->delete_range(begin_delete_key, end_delete_key))//["begin_key", "end_key")
            {
                xinfo("xvblockprune_impl::recycle,succsssful for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
                
                account_meta._highest_deleted_block_height = upper_bound_height - 5000 + 1;
                return true;//return true to indicate it has successful deleted
            }
            else
            {
                xerror("xvblockprune_impl::recycle,failed for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
            }
            return false;
        }
    
        bool  xvblockprune_impl::recycle_unit(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            if(account_meta._highest_full_block_height <= enum_reserved_blocks_count) //start prune at least > 8
                return false;
 
            //[lower_bound_height,upper_bound_height)
            const uint64_t upper_bound_height = account_meta._highest_full_block_height - enum_reserved_blocks_count;
            const uint64_t lower_bound_height = std::max(account_meta._lowest_vkey2_block_height,account_meta._highest_deleted_block_height) + 1;
            
            xdbg("xvblockprune_impl::recycle account %s, upper %llu, lower %llu, connect_height %llu", account_obj.get_address().c_str(),
                upper_bound_height, lower_bound_height, account_meta._highest_connect_block_height);
            
            if(lower_bound_height >= upper_bound_height)
                return false;
            else if((upper_bound_height - lower_bound_height) < enum_min_batch_recycle_blocks_count)
                return false;//collect big range for each prune op as performance consideration
            
            const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,lower_bound_height);
            const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,upper_bound_height);
            if(get_xvdb()->delete_range(begin_delete_key, end_delete_key))//["begin_key", "end_key")
            {
                xinfo("xvblockprune_impl::recycle,succsssful for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
                
                account_meta._highest_deleted_block_height = upper_bound_height - 1;
                return true;//return true to indicate it has successful deleted
            }
            else
            {
                xerror("xvblockprune_impl::recycle,failed for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
            }
            return false;
        }
    }
}
