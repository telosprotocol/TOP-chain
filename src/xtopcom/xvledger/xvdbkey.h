// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvaccount.h"

namespace top
{
    namespace base
    {
       class xvdbkey_t
       {
        public:
           //tx index ->link to block index
           static const std::string  create_tx_key(const std::string & org_tx_hash); //where the raw tx are placed
           static const std::string  create_tx_index_key(const std::string & org_tx_hash, const uint8_t index_type);

           //block-index ->linke block-object,block-state,block-offdata
           static const std::string  create_block_index_key(const xvaccount_t & account,const uint64_t target_height);//main entry
           static const std::string  create_block_index_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);//second intry
           
           //block_object,block_state and block_offdata,..etc are all located by original block' hash
           //which may reduce keys size by dlt encode and might be higher possiblility stored at same section
           static const std::string  create_block_object_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_input_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_input_resource_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_output_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_output_resource_key(const xvaccount_t & account,const std::string & org_block_hash);
           
           static const std::string  create_block_state_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_offdata_key(const xvaccount_t & account,const std::string & org_block_hash);
       };
    
    }//end of namespace of base
}//end of namespace top
