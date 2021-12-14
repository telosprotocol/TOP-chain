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
            
            return true;
        }
    
        bool  xvblockprune_impl::recycle_contract(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            return false;
        }
        
        bool  xvblockprune_impl::recycle_table(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            if(account_meta._highest_full_block_height <= enum_reserved_blocks_count) //start prune at least > 8
                return false;
            
            //[lower_bound_height,upper_bound_height)
            uint64_t upper_bound_height = account_meta._highest_full_block_height - enum_reserved_blocks_count;
            const uint64_t lower_bound_height = std::max(account_meta._lowest_vkey2_block_height,account_meta._highest_deleted_block_height) + 1;
            
            xdbg("xvblockprune_impl::recycle account %s, upper %llu, lower %llu, connect_height %llu", account_obj.get_address().c_str(),
                upper_bound_height, lower_bound_height, account_meta._highest_connect_block_height);
            
            if(lower_bound_height >= upper_bound_height)
                return false;
            else if((upper_bound_height - lower_bound_height) < (enum_min_batch_recycle_blocks_count << 1))
                return false;//collect big range for each prune op as performance consideration
            
            upper_bound_height = upper_bound_height - (enum_min_batch_recycle_blocks_count << 1);

            uint64_t boundary = get_prune_boundary(account_obj);
            if (boundary < upper_bound_height) {
                upper_bound_height = boundary;
            }

            xdbg("xvblockprune_impl::recycle account %s, adjust upper %llu, lower %llu, connect_height %llu", account_obj.get_address().c_str(),
                upper_bound_height, lower_bound_height, account_meta._highest_connect_block_height);
            if (lower_bound_height >= upper_bound_height) {
                return false;
            }

            const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,lower_bound_height);
            const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,upper_bound_height);
            if(get_xvdb()->delete_range(begin_delete_key, end_delete_key))//["begin_key", "end_key")
            {
                xinfo("xvblockprune_impl::recycle,succsssful for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
                
                account_meta._highest_deleted_block_height = upper_bound_height - 1;
            }
            else
            {
                xerror("xvblockprune_impl::recycle,failed for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
            }
            return true;
        }
    
        bool  xvblockprune_impl::recycle_unit(const base::xvaccount_t & account_obj,base::xblockmeta_t & account_meta)
        {
            if(account_meta._highest_full_block_height <= enum_reserved_blocks_count) //start prune at least > 8
                return false;
             
            //[lower_bound_height,upper_bound_height)

            uint64_t upper_bound_height = account_meta._highest_full_block_height - enum_reserved_blocks_count;
            const uint64_t lower_bound_height = std::max(account_meta._lowest_vkey2_block_height,account_meta._highest_deleted_block_height) + 1;
            

            xdbg("xvblockprune_impl::recycle account %s, upper %llu, lower %llu, connect_height %llu", account_obj.get_address().c_str(),
                upper_bound_height, lower_bound_height, account_meta._highest_connect_block_height);
            
            if(lower_bound_height >= upper_bound_height)
                return false;
            else if((upper_bound_height - lower_bound_height) < enum_min_batch_recycle_blocks_count)
                return false;//collect big range for each prune op as performance consideration
            
            upper_bound_height = upper_bound_height - enum_min_batch_recycle_blocks_count;

            const std::string begin_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,lower_bound_height);
            const std::string end_delete_key = base::xvdbkey_t::create_prunable_block_height_key(account_obj,upper_bound_height);
            if(get_xvdb()->delete_range(begin_delete_key, end_delete_key))//["begin_key", "end_key")
            {

                xinfo("xvblockprune_impl::recycle,succsssful for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
                
                account_meta._highest_deleted_block_height = upper_bound_height - 1;
            }
            else
            {
                xerror("xvblockprune_impl::recycle,failed for account %s between %s and %s", account_obj.get_address().c_str(), begin_delete_key.c_str(), end_delete_key.c_str());
            }

            return true;
        }

        bool  xvblockprune_impl::refresh(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj, const uint64_t permit_prune_upper_boundary) {
            if(!account_obj.is_table_address()) {
                return false;
            }

            // if consensus zone
            auto zone_id = account_obj.get_zone_index();
            if ((zone_id == base::enum_chain_zone_zec_index) || (zone_id == base::enum_chain_zone_beacon_index)) {
                return false;
            }
            
            xdbg("xvblockprune_impl::refresh, account:%s, module:%llx, height:%llu", account_obj.get_address().c_str(), mod_id, permit_prune_upper_boundary);
            
            //设置lock为1
            std::unique_lock<std::mutex> lock(m_lock);
            auto mod_prune_boundary = m_prune_boundary.find(account_obj.get_account());
            if (mod_prune_boundary != m_prune_boundary.end()) {
                if (mod_prune_boundary->second.find(mod_id) != mod_prune_boundary->second.end()) {
                    if (mod_prune_boundary->second[mod_id] >= permit_prune_upper_boundary) {
                        return true;
                    }
                } 
                mod_prune_boundary->second[mod_id] = permit_prune_upper_boundary;
            } else {
                std::map<chainbase::enum_xmodule_type, uint64_t> prune_boundary;
                prune_boundary[mod_id] = permit_prune_upper_boundary;
                m_prune_boundary[account_obj.get_account()] = prune_boundary;
            }
            
            return true;
        }

        bool  xvblockprune_impl::watch(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj) {
            if(!account_obj.is_table_address()) {
                return false;
            }

            // if consensus zone
            auto zone_id = account_obj.get_zone_index();
            if ((zone_id == base::enum_chain_zone_zec_index) || (zone_id == base::enum_chain_zone_beacon_index)) {
                return false;
            }

            xdbg("xvblockprune_impl::watch, account:%s, module:%llx", account_obj.get_address().c_str(), mod_id);

            //设置lock为1
            std::unique_lock<std::mutex> lock(m_lock);  
            auto mod_prune_boundary = m_prune_boundary.find(account_obj.get_account());
            if (mod_prune_boundary == m_prune_boundary.end()) {
                std::map<chainbase::enum_xmodule_type, uint64_t> prune_boundary;
                prune_boundary[mod_id] = 0;
                m_prune_boundary[account_obj.get_account()] = prune_boundary;
            }
            return true;
        }

        bool  xvblockprune_impl::unwatch(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj) {
            if(!account_obj.is_table_address()) {
                return false;
            }

            // if consensus zone
            auto zone_id = account_obj.get_zone_index();
            if ((zone_id == base::enum_chain_zone_zec_index) || (zone_id == base::enum_chain_zone_beacon_index)) {
                return false;
            }

            xdbg("xvblockprune_impl::unwatch, account:%s, module:%llx", account_obj.get_address().c_str(), mod_id);

            //设置lock为1
            std::unique_lock<std::mutex> lock(m_lock);  
            auto mod_prune_boundary = m_prune_boundary.find(account_obj.get_account());
            if (mod_prune_boundary != m_prune_boundary.end()) {
                mod_prune_boundary->second.erase(mod_id);
                if (mod_prune_boundary->second.empty()) {
                    m_prune_boundary.erase(account_obj.get_account());
                }
            }

            return true;
        }

        uint64_t xvblockprune_impl::get_prune_boundary(const base::xvaccount_t & account_obj) {
            //设置lock为1
            std::unique_lock<std::mutex> lock(m_lock); 
            auto mod_prune_boundary = m_prune_boundary.find(account_obj.get_account());
            if (mod_prune_boundary == m_prune_boundary.end()) {
                return 0;
            }
            
            uint64_t min_prune_boundary = (uint64_t)-1;
            for (auto it : mod_prune_boundary->second) {
                if (it.second < min_prune_boundary) {
                    min_prune_boundary = it.second;
                }
            }
            return min_prune_boundary;
        }
    }
}
