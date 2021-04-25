// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvblock.h"
#include "xvstate.h"
#include "xvaccount.h"

namespace top
{
    namespace base
    {
        class xvstatestore_t : public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvstatestore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}
            
        protected:
            xvstatestore_t();
            virtual ~xvstatestore_t();
        private:
            xvstatestore_t(xvstatestore_t &&);
            xvstatestore_t(const xvstatestore_t &);
            xvstatestore_t & operator = (const xvstatestore_t &);
        public:
            //caller need to cast (void*) to related ptr
            virtual void*             query_interface(const int32_t _enum_xobject_type_) override;
            //a full path to load xvbstate could be  get_store_path()/account/height/block-hash
            virtual std::string       get_store_path() const {return "/state/";};//each store may has own space at DB/disk
            
        public:
            virtual bool                  get_block_state(xvblock_t * target_block);//once successful,assign xvbstate_t into block
            virtual xauto_ptr<xvbstate_t> get_block_state(xvaccount_t & account,const uint64_t height,const uint64_t view_id);
            virtual xauto_ptr<xvbstate_t> get_block_state(xvaccount_t & account,const uint64_t height,const std::string& block_hash);
            
        protected:
            bool                          write_state_to_db(xvblock_t * block_ptr);
            bool                          write_state_to_db(xvaccount_t & target_account,xvblock_t * block_ptr);
            xvbstate_t*                   read_state_from_db(xvblock_t * for_block);
            xvbstate_t*                   read_state_from_db(xvaccount_t & target_account,xvblock_t * for_block);
            xvbstate_t*                   read_state_from_db(xvaccount_t & target_account,const std::string & block_hash);
        
            bool                          delete_state_of_db(xvaccount_t & target_account,const std::string & block_hash);
            bool                          delete_states_of_db(xvaccount_t & target_account,const uint64_t height);
            
            bool                          rebuild_state_for_block(xvblock_t & target_block);
            bool                          rebuild_state_for_block(xvbstate_t & target_state,xvblock_t & target_block);
        };
    
    }//end of namespace of base
}//end of namespace top
