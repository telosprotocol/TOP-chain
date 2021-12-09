// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvaccount.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvdbstore.h"

namespace top
{
    namespace store
    {
        enum enum_xblockstore_version
        {
            enum_xblockstore_version_0            = 0, //
            enum_xblockstore_prunable_version     = 1, //support prune blocks etc as batch mode
        };
    
        class xvblockdb_t : public base::xobject_t
        {
            friend class xunitbkplugin;
            friend class xtablebkplugin;
        public:
            xvblockdb_t(base::xvdbstore_t* xvdb_ptr);
        public:
            virtual ~xvblockdb_t();
        private:
            xvblockdb_t();
            xvblockdb_t(xvblockdb_t &&);
            xvblockdb_t(const xvblockdb_t &);
            xvblockdb_t & operator = (const xvblockdb_t &);
        public:
            base::xvdbstore_t*      get_xdbstore() const {return m_xvdb_ptr;}
            inline const int    get_blockstore_version() const {return m_blockstore_version;}
            
        public://mulitple threads safe
            //[regular block] = [block-object + block-input + block-output]
            bool                load_block_input(base::xvbindex_t* target_index);
            bool                load_block_input(base::xvbindex_t* target_index,base::xvblock_t * target_block);
            
            bool                load_block_output(base::xvbindex_t* target_index);
            bool                load_block_output(base::xvbindex_t* target_index,base::xvblock_t * target_block);
            
            bool                load_block_object(base::xvbindex_t* index_ptr, const int atag = 0);

            bool                delete_block(base::xvbindex_t* index_ptr);
            
        public://mulitple threads safe
            //when successful return the comibned stored-flags, return 0 if nothing changed, but return < 0 when failed
            int                 save_block(base::xvbindex_t* index_ptr);
            int                 save_block(base::xvbindex_t* index_ptr,base::xvblock_t * linked_block_ptr);
            
        public://mulitple threads safe
            const std::string   load_value_by_path(const std::string & full_path_as_key);
            bool                store_value_by_path(const std::string & full_path_as_key,const std::string & value);
            bool                delete_value_by_path(const std::string & full_path_as_key);
            
        protected:
            bool                write_index_to_db(base::xvbindex_t* index_obj);
            base::xvbindex_t*   read_index_from_db(const std::string & index_db_key_path);
            //return map sorted by viewid from lower to high,caller respond to release ptr later
            std::vector<base::xvbindex_t*> read_index_from_db(const base::xvaccount_t & account,const uint64_t target_height);
            
        protected:
            //when successful return the comibned stored-flags, return 0 if nothing changed, but return < 0 when failed
            int                 write_block_object_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            int                 write_block_input_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            int                 write_block_output_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
        protected:
            bool                read_block_object_from_db(base::xvbindex_t* index_ptr);
            bool                read_block_object_from_db(base::xvbindex_t* index_ptr,base::xvdbstore_t* from_db);
            bool                read_block_input_from_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr,base::xvdbstore_t* from_db);
            bool                read_block_output_from_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr,base::xvdbstore_t* from_db);
            
            std::vector<base::xvblock_t*>  read_prunable_block_object_from_db(base::xvaccount_t & account,const uint64_t target_height);

        protected:
            const std::string   create_block_index_key(const base::xvaccount_t & account,const uint64_t target_height);
            const std::string   create_block_index_key(const base::xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid);
            const std::string   create_block_index_key(base::xvbindex_t * index_ptr,const uint64_t target_height,const uint64_t target_viewid);
            
            
            const std::string   create_block_object_key(base::xvbindex_t * index_ptr);
            const std::string   create_block_input_key(base::xvbindex_t * index_ptr);
            const std::string   create_block_input_resource_key(base::xvbindex_t * index_ptr);
            const std::string   create_block_output_key(base::xvbindex_t * index_ptr);
            const std::string   create_block_output_resource_key(base::xvbindex_t * index_ptr);
            
        private:
             base::xvdbstore_t*  m_xvdb_ptr;
             int                 m_blockstore_version;
        };
 
    };//end of namespace of vstore
};//end of namespace of top
