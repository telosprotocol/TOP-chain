// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvledger/xvdbkey.h"

#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"

namespace top
{
    namespace base
    {
        //assemble the full uint64(8 bytes = 16 hex char) string that helpful for key-order in DB
        const std::string  uint64_to_full_hex(const uint64_t uint64value)        //unint64 to string of hex format
        {
            char szBuff[32] = {0};
            const int inBufLen = sizeof(szBuff);
            //snprintf(szBuff,inBufLen,"%16llx",uint64value);
            snprintf(szBuff,inBufLen,"%16llx", (long long unsigned int)uint64value);
            return std::string(szBuff);
        }

        const std::string  xvdbkey_t::create_tx_key(const std::string & org_tx_hash) //where the raw tx are placed
        {
            const std::string key_path = "t/" + org_tx_hash;
            return key_path;
        }

        //tx index ->link to block index
        const std::string  xvdbkey_t::create_tx_index_key(const std::string & org_tx_hash, const enum_txindex_type type)
        {
            const std::string key_path = "t/" + org_tx_hash + "/" + xstring_utl::tostring(type);
            return key_path;
        }
        const std::string xvdbkey_t::get_txhash_from_txindex_key(const std::string & key) {
            if (key.size() == 36) {
                return key.substr(2, 32);
            }
            return {};
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

        //block_object,block_state ...etc are all located by original block' hash
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

        //block_object,block_state are all located by original block' hash
        const std::string  xvdbkey_t::create_block_state_key(const xvaccount_t & account,const std::string & org_block_hash)
        {
            const std::string key_path = "b/" + account.get_xvid_str() + "/" + org_block_hash + "/s";
            return key_path;
        }

        const std::string  xvdbkey_t::create_chain_key(const xvaccount_t & account)
        {
            const std::string key_path = "c/" + account.get_xvid_str();
            return key_path;
        }

        const std::string  xvdbkey_t::create_chain_span_key(const xvaccount_t & account, const uint64_t height)
        {
            const std::string key_path = "c/" + account.get_xvid_str() + "/s/" + xstring_utl::uint642hex(height);
            return key_path;
        }

        const std::string  xvdbkey_t::create_account_meta_key_old(const xvaccount_t & account)
        {
            std::string meta_path;
            meta_path.reserve(256);
            meta_path += xstring_utl::tostring(account.get_chainid());
            meta_path += "/";
            meta_path += account.get_account();
            meta_path += "/meta";
            return meta_path;
        }

        //-------------------------------new key style for block,index,meta---------------------------------//
        const std::string  xvdbkey_t::create_account_meta_key(const xvaccount_t & account)
        {
            //enum_xvdb_cf_type_update_most = 'u'
            const std::string key_path = "u/" + account.get_storage_key() + "/m";
            return key_path;
        }

        const std::string  xvdbkey_t::create_account_span_genesis_height_key(const xvaccount_t & account)
        {
            //enum_xvdb_cf_type_update_most = 'u'
            const std::string key_path = "u/" + account.get_storage_key() + "/g";
            return key_path;
        }

        const std::string  xvdbkey_t::create_account_span_key(const xvaccount_t & account,const uint64_t target_height)
        {
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/a";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_tx_key(const std::string & org_tx_hash) //where the raw tx are placed
        {
            //enum_xvdb_cf_type_FIFO = 'f'
            const std::string key_path = "f/" + org_tx_hash;
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_blockhash_key(const std::string & org_tx_hash)
        {
            //enum_xvdb_cf_type_FIFO = 'f'
            const std::string key_path = "f/" + org_tx_hash + "/h";  // h for height
            return key_path;
        }

        //tx index ->link to block index
        const std::string  xvdbkey_t::create_prunable_tx_index_key(const std::string & org_tx_hash, const enum_txindex_type type)
        {
            //enum_xvdb_cf_type_FIFO = 'f'
            const std::string key_path = "f/" + org_tx_hash + "/" + xstring_utl::tostring(type);
            return key_path;
        }
        //relay tx index ->link to block index
        const std::string  xvdbkey_t::create_prunable_relay_tx_index_key(const std::string & org_tx_hash,const enum_txindex_type type)
        {
            //enum_xvdb_cf_type_FIFO = 'f'
            const std::string key_path = "f/" + org_tx_hash + "/" + xstring_utl::tostring(type) + "/l";
            return key_path;
        }
        const std::string  xvdbkey_t::create_prunable_state_key(const xvaccount_t & account,const uint64_t target_height)
        {
            //enum_xdb_cf_type_read_most = 's'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/s";//a for state
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_state_key(const xvaccount_t & account,const uint64_t target_height,const std::string & block_hash)
        {
            //enum_xdb_cf_type_read_most = 's'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + block_hash + "/s";//a for state
            return key_path;
        }

        const std::string xvdbkey_t::create_prunable_unit_state_key(const xvaccount_t & account, uint64_t target_height,std::string const& block_hash)
        {
            // state key should be different with block key, because of batch_delete policy
            const std::string key_path = "s/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + block_hash + "/u";
            return key_path;
        }
        const std::string xvdbkey_t::create_prunable_fullunit_state_key(const xvaccount_t & account, uint64_t target_height,std::string const& block_hash)
        {
            // fullunit state key same with unitblock, prune with unitblock
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + block_hash + "/f";  // f for fullunit state
            return key_path;
        }        
        const std::string  xvdbkey_t::create_prunable_unit_state_height_key(const xvaccount_t & account,const uint64_t target_height)
        {
            const std::string key_path = "s/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_height_key(const xvaccount_t & account,const uint64_t target_height)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_index_key(const xvaccount_t & account,const uint64_t target_height)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/h";//h for index/header
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_index_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/h";
            return key_path;
        }

        //note: just for committed block
        const std::string  xvdbkey_t::create_prunable_block_object_key(const xvaccount_t & account,const uint64_t target_heigh)//prunable block object that include input/output as well
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_heigh) + "/b";//b for block
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_object_key(const xvaccount_t & account,const uint64_t target_heigh,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_heigh) + "/" + xstring_utl::uint642hex(target_viewid) + "/b";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_input_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/i";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_input_resource_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/l";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_output_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/o";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_output_resource_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/q";
            return key_path;
        }

        const std::string  xvdbkey_t::create_prunable_block_output_offdata_key(const xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            const std::string key_path = "r/" + account.get_storage_key() + "/" + uint64_to_full_hex(target_height) + "/" + xstring_utl::uint642hex(target_viewid) + "/f";
            return key_path;
        }
        const std::string  xvdbkey_t::create_prunable_unit_proof_key(const xvaccount_t & account, const uint64_t target_height)
        {
            //enum_xdb_cf_type_read_most = 'r'
            const std::string key_path = "r/" + account.get_storage_key()+ "/" + uint64_to_full_hex(target_height) + "/p";
            return key_path;
        }

        const std::string xvdbkey_t::create_prunable_mpt_node_key(const xvaccount_t & account, const std::string & key)
        {
            std::string prefix = create_prunable_mpt_node_key_prefix(account);
            return create_prunable_mpt_node_key(prefix, key);
        }
        const std::string xvdbkey_t::create_prunable_mpt_node_key_prefix(const xvaccount_t & account)
        {
            auto zone_bytes = to_bytes(account.get_zone_index());
            auto subaddr_bytes = to_bytes(account.get_ledger_subaddr());
            auto const key_path = std::string{"s/"} + std::string{zone_bytes.begin(), zone_bytes.end()} + std::string{subaddr_bytes.begin(), subaddr_bytes.end()}  + "/";
            return key_path;
        }

        const std::string xvdbkey_t::create_prunable_mpt_node_key(const std::string & prefix, const std::string & key)
        {
            auto const key_path = prefix + key + "/m";
            return key_path;
        }
        enum_xdbkey_type xvdbkey_t::get_dbkey_type_v2(const std::string & key, const char first_char, const char last_char, const int key_length) {
            struct xvdbkey_first_last_char_type_t
            {
                enum_xdbkey_type type;
                char first;
                char last;
            };

            static  std::vector<xvdbkey_first_last_char_type_t> _fist_last_relations = {
                {enum_xdbkey_type_account_meta,         'u', 'm'},
                {enum_xdbkey_type_account_span_height,  'u', 'g'},
                {enum_xdbkey_type_transaction,          'f', 'h'},
                {enum_xdbkey_type_relaytx_index,        'f', 'l'},

                {enum_xdbkey_type_unitstate_v2,         's', 'u'},
                {enum_xdbkey_type_mptnode,              's', 'm'},

                {enum_xdbkey_type_account_span,         'r', 'a'},
                {enum_xdbkey_type_block_object,         'r', 'b'},
                {enum_xdbkey_type_block_out_offdata,    'r', 'f'},
                {enum_xdbkey_type_block_index,          'r', 'h'},
                {enum_xdbkey_type_block_input_resource, 'r', 'l'},
                {enum_xdbkey_type_unit_proof,           'r', 'p'},
                {enum_xdbkey_type_block_output_resource,'r', 'q'},
                {enum_xdbkey_type_state_object,         'r', 's'},
            };

            for (auto & v : _fist_last_relations) {
                if (v.first == first_char && v.last == last_char) {
                    return v.type;
                }
            }

            return enum_xdbkey_type_unknow;
        }

        enum_xdbkey_type xvdbkey_t::get_dbkey_type_v1(const std::string & key, const char first_char, const char last_char, const int key_length) {
            enum_xdbkey_type type = enum_xdbkey_type_unknow;
            if(first_char == 'f')//new version
            {
                type = enum_xdbkey_type_transaction;
            }
            else if(first_char == 'i')//old version
            {
                type = enum_xdbkey_type_block_index;
            }
            else if(first_char == 'b')//old version
            {
                switch(last_char)
                {
                    case 'h':
                    type = enum_xdbkey_type_block_object;
                    break;

                    case 's':
                    type = enum_xdbkey_type_state_object;
                    break;

                    case 'r':
                    if(key[key_length - 2] == 'i')//ir
                        type = enum_xdbkey_type_block_input_resource;
                    else //or
                        type = enum_xdbkey_type_block_output_resource;
                    break;
                }
            }
            else if(first_char == 't')//old version
            {
                type = enum_xdbkey_type_transaction;
            }
            else if(first_char == 'c')//old version
            {
                std::vector<std::string> values;
                base::xstring_utl::split_string(key, '/', values);
                if (values.size() > 2) {
                    type = enum_xdbkey_type_account_span;
                } else {
                    type = enum_xdbkey_type_account_span_height;
                }
            }
            else if(first_char == '0')//old version
            {
                type = enum_xdbkey_type_account_meta;
            }
            return type;
        }

        enum_xdbkey_type   xvdbkey_t::get_dbkey_type(const std::string & key)
        {
            enum_xdbkey_type type = enum_xdbkey_type_unknow;
            const int key_length = (const int)key.size();
            if(key_length < 4)
                return type;
            else if(key[1] != '/')
                return enum_xdbkey_type_unknow;

            type = enum_xdbkey_type_keyvalue;//at least a valid key

            const char first_char = key[0];
            const char last_char  = key[key_length - 1];

            type = get_dbkey_type_v2(key, first_char, last_char, key_length);
            if (type != enum_xdbkey_type_unknow) {
                return type;
            }
            type = get_dbkey_type_v1(key, first_char, last_char, key_length);
            if (type != enum_xdbkey_type_unknow) {
                return type;
            }
            return enum_xdbkey_type_unknow;
        }

        const std::string xvdbkey_t::get_dbkey_type_name(enum_xdbkey_type type)
        {
            static std::string key_name_array[enum_xdbkey_type_max] = {"unknow", "keyvalue", "block_index", "block_object",
                            "state_object", "account_meta", "account_span", "transaction", "input_resource",
                            "output_resource",  "span_height", "unit_proof", "out_offdata", "trelayx_index", "unitstate_v2", "mptnode"};
            if (type >= enum_xdbkey_type_max) {
                return "unknow_2";
            }
            return key_name_array[type];
        }

        const std::string xvdbkey_t::get_account_prefix_key(const std::string & key)
        {
            std::vector<std::string> values;
            base::xstring_utl::split_string(key, '/', values);
            if (values.size() >= 2) {
                const std::string addr = xvaccount_t::compact_address_from(values[2]);
                const std::string account_key = values[0] + "/" + values[1] +"/"+ addr;
                return account_key;
            }
            return key;
        }
        const std::string xvdbkey_t::get_account_address_from_key(const std::string & key)
        {
            std::vector<std::string> values;
            base::xstring_utl::split_string(key, '/', values);
            if (values.size() >= 2) {
                const std::string addr = xvaccount_t::compact_address_from(values[2]);
                return addr;
            }
            return key;
        }
    }//end of namespace of base
}//end of namespace top
