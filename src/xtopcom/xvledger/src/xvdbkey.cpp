// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvdbkey.h"
#include "xbase/xutl.h"
 
namespace top
{
    namespace base
    {
        const std::string  xvdbkey_t::create_tx_key(const std::string & org_tx_hash) //where the raw tx are placed
        {
            const std::string key_path = "t/" + org_tx_hash;
            return key_path;
        }
        
        //tx index ->link to block index
        const std::string  xvdbkey_t::create_tx_index_key(const std::string & org_tx_hash, enum_txindex_type type)
        {
            const std::string key_path = "t/" + org_tx_hash + "/" + xstring_utl::tostring(type);
            return key_path;
        }
    
        //block-index ->linke block-object,block-state,block-offdata
        const std::string  xvdbkey_t::create_block_index_key(const xvaccount_t & account,const uint64_t target_height)//for main entry
        {
            const std::string key_path = "i/" + account.get_address() + "/" + xstring_utl::uint642hex(target_height);
            return key_path;
        }
        
        const std::string  xvdbkey_t::create_block_index_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            const std::string key_path = "i/" + account.get_address() + "/" + xstring_utl::uint642hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid);
            return key_path;
        }
        
        //block_object,block_state and block_offdata,...etc are all located by original block' hash
        //which may reduce keys size by dlt encode and might be higher possiblility stored at same section
        const std::string  xvdbkey_t::create_block_object_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/h";
            return key_path;
        }
    
        const std::string  xvdbkey_t::create_block_input_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/i";
            return key_path;
        }
    
        const std::string  xvdbkey_t::create_block_input_resource_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/ir";
            return key_path;
        }
    
        const std::string  xvdbkey_t::create_block_output_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/o";
            return key_path;
        }
    
        const std::string  xvdbkey_t::create_block_output_resource_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/or";
            return key_path;
        }
        
        //block_object,block_state and block_offdata are all located by original block' hash
        const std::string  xvdbkey_t::create_block_state_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/s";
            return key_path;
        }
        
        //block_object,block_state and block_offdata are all located by original block' hash
        const std::string  xvdbkey_t::create_block_offdata_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/d";
            return key_path;
        }

    }//end of namespace of base
}//end of namespace top
