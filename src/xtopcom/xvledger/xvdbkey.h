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

       enum enum_xdbkey_type
       {
           enum_xdbkey_type_unknow          = 0x0000, //unknow class
           enum_xdbkey_type_keyvalue        = 0x0001, //key-value
           enum_xdbkey_type_block_index     = 0x0002, //block index
           enum_xdbkey_type_block_object    = 0x0003, //block object
           enum_xdbkey_type_state_object    = 0x0004, //state object
           enum_xdbkey_type_account_meta    = 0x0005, //account meta
           enum_xdbkey_type_account_span    = 0x0006, //account span
           enum_xdbkey_type_transaction     = 0x0007, //txs

           enum_xdbkey_type_block_input_resource   = 0x0008,
           enum_xdbkey_type_block_output_resource  = 0x0009,
           enum_xdbkey_type_account_span_height    = 0x000a, //account span height
           enum_xdbkey_type_unit_proof             = 0x000b, //unit proof
           enum_xdbkey_type_block_out_offdata      = 0x000c,
           enum_xdbkey_type_relaytx_index          = 0x000d,
           enum_xdbkey_type_unitstate_v2           = 0x000e,
           enum_xdbkey_type_mptnode                = 0x000f,

           enum_xdbkey_type_max, //not over this max value
       };

       class xvdbkey_t
       {
        public:
           static const std::string  get_xdb_version_key()              {return "/version";}
           static const std::string  get_blockstore_version_key()       {return "/blockstore/version";}
           static const std::string  get_statestore_version_key()       {return "/statestore/version";}
           static const std::string  get_txstore_version_key()          {return "/txstore/version";}
           static const std::string  get_constractstore_version_key()   {return "/constractstore/version";}

           static enum_xdbkey_type   get_dbkey_type(const std::string & key);
           static enum_xdbkey_type   get_dbkey_type_v2(const std::string & key, const char first_char, const char last_char, const int key_length);
           static enum_xdbkey_type   get_dbkey_type_v1(const std::string & key, const char first_char, const char last_char, const int key_length);
           static const std::string  get_dbkey_type_name(enum_xdbkey_type type);
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
           static const std::string  create_account_meta_key_old(const xvaccount_t & account);

        public://new key style for block,index,meta and designed for multiple CF purpose
           //meta,span related
           static const std::string  create_account_meta_key(const xvaccount_t & account);
           static const std::string  create_account_span_genesis_height_key(const xvaccount_t & account);
           static const std::string  create_account_span_key(const xvaccount_t & account,const uint64_t target_height);

           static const std::string  create_prunable_state_key(const xvaccount_t & account,const uint64_t target_height);
           static const std::string  create_prunable_state_key(const xvaccount_t & account,const uint64_t target_height,const std::string & block_hash);
           // now unit state key, different from block
           static const std::string  create_prunable_unit_state_key(const xvaccount_t & account, uint64_t target_height,std::string const& block_hash);
           static const std::string  create_prunable_fullunit_state_key(const xvaccount_t & account, uint64_t target_height,std::string const& block_hash);
           //all keys under of same height state
           static const std::string  create_prunable_unit_state_height_key(const xvaccount_t & account,const uint64_t target_height);

           //prunable tx and tx index
           static const std::string  create_prunable_tx_key(const std::string & org_tx_hash);
           static const std::string  create_prunable_blockhash_key(const std::string & org_tx_hash);
           static const std::string  create_prunable_tx_index_key(const std::string & org_tx_hash, const enum_txindex_type type);
           static const std::string  create_prunable_relay_tx_index_key(const std::string & org_tx_hash,const enum_txindex_type type);

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
           static const std::string  create_prunable_block_output_offdata_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);

           static const std::string  create_prunable_unit_proof_key(const xvaccount_t & account, const uint64_t target_height);
           static const std::string  create_prunable_mpt_node_key(const xvaccount_t & account, const std::string & key);
           static const std::string  create_prunable_mpt_node_key_prefix(const xvaccount_t & account);
           static const std::string  create_prunable_mpt_node_key(const std::string & prefix, const std::string & key);

           static const std::string  get_account_prefix_key(const std::string & key);
           static const std::string  get_account_address_from_key(const std::string & key);
           static const std::string  get_txhash_from_txindex_key(const std::string & key);
       };

    }//end of namespace of base
}//end of namespace top
