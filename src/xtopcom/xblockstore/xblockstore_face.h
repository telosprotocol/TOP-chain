// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xvledger.h"

#if defined(__MAC_PLATFORM__) && defined(__ENABLE_MOCK_XSTORE__)
namespace top
{
    namespace store
    {
        class xstore_face_t : virtual public base::xrefcount_t
        {
        protected:
            xstore_face_t(){};
            virtual ~xstore_face_t(){};
        private:
            xstore_face_t(const xstore_face_t &);
            xstore_face_t & operator = (const xstore_face_t &);
            
        public://block object manage
            virtual bool             set_vblock(const std::string & store_path,base::xvblock_t* block) = 0;
            virtual bool             set_vblock_header(const std::string & store_path,base::xvblock_t* block) = 0;

            virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const = 0;
            virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const = 0;
            virtual bool             get_vblock_input(const std::string & store_path,base::xvblock_t* for_block)  const = 0;//just load input
            virtual bool             get_vblock_output(const std::string & store_path,base::xvblock_t* for_block) const = 0;//just load output
            
            virtual bool             execute_block(base::xvblock_t* block) = 0;
            
        public://key-value manage
            virtual bool              set_value(const std::string & key, const std::string& value) = 0;
            virtual bool              delete_value(const std::string & key) = 0;
            virtual const std::string get_value(const std::string & key) const = 0;
        };
    };
};
#else
    #include "xstore/xstore_face.h"
#endif

namespace top
{
    namespace store
    {
        //note: layers for store :  [xvblock-store] --> [xstore] -->[xdb]
        class xblockstorehub_t //manage multiple blockstores
        {
        public:
            static xblockstorehub_t &    instance();
        protected:
            xblockstorehub_t();
            virtual ~xblockstorehub_t();
        private:
            xblockstorehub_t(const xblockstorehub_t &);
            xblockstorehub_t & operator = (const xblockstorehub_t &);
        public:
            //xdb_ptr is to xstore object,but we not expose type/class name here for let caller
            virtual base::xvblockstore_t*  get_block_store(xstore_face_t & _persist_db,const std::string & account) = 0;
            
        public://debug purpose only
            virtual base::xvblockstore_t*  create_block_store(xstore_face_t& _persist_db,const std::string & store_path) = 0;  //create multiple blockstore
        };
        
    };//end of namespace of xledger
};//end of namespace of top
