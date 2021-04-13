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
        class xvstatestore : public xobject_t
        {
            friend class xvchaint_t;
        public:
            static  const std::string   name(){return "xvstatestore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}
            
        protected:
            xvstatestore();
            virtual ~xvstatestore();
        private:
            xvstatestore(xvstatestore &&);
            xvstatestore(const xvstatestore &);
            xvstatestore & operator = (const xvstatestore &);
        public:
            //caller need to cast (void*) to related ptr
            virtual void*             query_interface(const int32_t _enum_xobject_type_) override;
            //a full path to load xvbstate could be  get_store_path()/account/height/block-hash
            virtual std::string       get_store_path() const {return "/state/";};//each store may has own space at DB/disk
            
        public:
            virtual bool                  get_block_state(xvblock_t * target_block); //once successful,assign xvbstate_t into block
            virtual xauto_ptr<xvbstate_t> get_block_state(xvaccount_t & account,const uint64_t height,const uint64_t view_id);
            virtual xauto_ptr<xvbstate_t> get_block_state(xvaccount_t & account,const uint64_t height,const std::string& block_hash);
            
        protected:
            using xobject_t::add_ref;
            using xobject_t::release_ref;
        };
    
    }//end of namespace of base
}//end of namespace top
