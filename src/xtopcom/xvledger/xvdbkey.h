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
       enum enum_txindex_type
       {
           enum_txindex_type_main     = 0x00,  //main entry if need(optional)
           enum_txindex_type_send     = 0x01,  // send or self tx
           enum_txindex_type_receive  = 0x02,  // recv tx
           enum_txindex_type_confirm  = 0x04,  // confirm tx
       };

       class xvdbkey_t
       {
        public:
           static const std::string  get_xdb_version_key()              {return "/version";}
           static const std::string  get_blockstore_version_key()       {return "/blockstore/version";}
           static const std::string  get_statestore_version_key()       {return "/statestore/version";}
           static const std::string  get_txstore_version_key()          {return "/txstore/version";}
           static const std::string  get_constractstore_version_key()   {return "/constractstore/version";}
           
        public://old definition,put here just for compatible purpose
           //tx index ->link to block index
           static const std::string  create_tx_key(const std::string & org_tx_hash); //where the raw tx are placed
           static const std::string  create_tx_index_key(const std::string & org_tx_hash, const enum_txindex_type type);

           //block-index ->linke block-object,block-state
           static const std::string  create_block_index_key(const xvaccount_t & account,const uint64_t target_height);//main entry
           static const std::string  create_block_index_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);//second intry

           //block_object,block_state ..etc are all located by original block' hash
           //which may reduce keys size by dlt encode and might be higher possiblility stored at same section
           static const std::string  create_block_object_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_input_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_input_resource_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_output_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_block_output_resource_key(const xvaccount_t & account,const std::string & org_block_hash);

           static const std::string  create_block_state_key(const xvaccount_t & account,const std::string & org_block_hash);
           static const std::string  create_chain_key(const xvaccount_t & account);
           static const std::string  create_chain_span_key(const xvaccount_t & account, const uint64_t height);
           
        public://new key style for block,index,meta and designed for multiple CF purpose
           //meta,span related
           static const std::string  create_account_meta_key(const xvaccount_t & account);
           static const std::string  create_account_span_key(const xvaccount_t & account);
           static const std::string  create_account_span_key(const xvaccount_t & account,const uint64_t target_height);
           
           //all keys under of same height
           static const std::string  create_prunable_block_height_key(const xvaccount_t & account,const uint64_t target_height);
           
           //block index related
           static const std::string  create_prunable_block_index_key(const xvaccount_t & account,const uint64_t target_heigh);
           static const std::string  create_prunable_block_index_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);//second intry
           
           //block object related
           static const std::string  create_prunable_block_object_key(const xvaccount_t & account,const uint64_t target_heigh);//prunable block object that include input/output as well
            static const std::string  create_prunable_block_object_key(const xvaccount_t & account,const uint64_t target_heigh,const uint64_t target_viewid);
           
           //block input/output related
           static const std::string  create_prunable_block_input_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);
           static const std::string  create_prunable_block_input_resource_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);
           static const std::string  create_prunable_block_output_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);
           static const std::string  create_prunable_block_output_resource_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);
       };

    }//end of namespace of base
}//end of namespace top
