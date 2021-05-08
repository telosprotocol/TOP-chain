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
        //chain managed account'state by individual unit block,return block-state(xvbstate_t)
        class xvblkstatestore_t
        {
        protected:
            xvblkstatestore_t(){};
            virtual ~xvblkstatestore_t(){};
        private:
            xvblkstatestore_t(xvblkstatestore_t &&);
            xvblkstatestore_t(const xvblkstatestore_t &);
            xvblkstatestore_t & operator = (xvblkstatestore_t &&);
            xvblkstatestore_t & operator = (const xvblkstatestore_t &);
            
        public:
            virtual xauto_ptr<xvbstate_t> get_block_state(xvblock_t * target_block);
            virtual xauto_ptr<xvbstate_t> get_block_state(const xvaccount_t & account,const uint64_t block_height,const std::string& block_hash);
            virtual xauto_ptr<xvbstate_t> get_block_state(const xvaccount_t & account,const uint64_t block_height,const uint64_t block_view_id);
            
        private:
            xauto_ptr<xvbstate_t>     load_block_state(const xvbindex_t * target_index);
    
            bool                      write_state_to_db(xvbstate_t & target_state,const std::string & target_block_hash);
            
            xvbstate_t*               read_state_from_db(const xvbindex_t * for_block);
            xvbstate_t*               read_state_from_db(const xvblock_t * for_block);
            xvbstate_t*               read_state_from_db(const xvaccount_t & target_account,const std::string & block_hash);
            
            bool                      delete_state_of_db(const xvbindex_t & target_index);
            bool                      delete_state_of_db(const xvblock_t & target_block);
            bool                      delete_state_of_db(const xvaccount_t & target_account,const std::string & block_hash);
            bool                      delete_states_of_db(const xvaccount_t & target_account,const uint64_t block_height);
            
            xvbstate_t*               rebuild_state_for_full_block(const xvbindex_t & target_index);
            xvbstate_t*               rebuild_state_for_full_block(xvblock_t & target_block);
            bool                      rebuild_state_for_block(xvblock_t & target_block,xvbstate_t & base_state);
        };
    
        //chain managed account 'state by a MPT tree(or likely) according state-hash,return xvactstate_t
        //traditional block that directly manage txs and change states of all account
        class xvactstatestore_t
        {
        protected:
            xvactstatestore_t(){};
            virtual ~xvactstatestore_t(){};
        private:
            xvactstatestore_t(xvactstatestore_t &&);
            xvactstatestore_t(const xvactstatestore_t &);
            xvactstatestore_t & operator = (xvactstatestore_t &&);
            xvactstatestore_t & operator = (const xvactstatestore_t &);
            
        public:
            //construct new xvactstate_t based on last state of account(through last_state_root_hash)
            virtual xauto_ptr<xvactstate_t> get_account_state(const xvaccount_t & account,const std::string last_state_root_hash)
            {
                return nullptr;
            }
        };
        
        //universal entry
        class xvstatestore_t : public xvblkstatestore_t,public xvactstatestore_t,public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvstatestore";} //"xvstatestore"
            virtual std::string         get_obj_name() const override {return name();}
        protected:
            xvstatestore_t();
            virtual ~xvstatestore_t();
        private:
            xvstatestore_t(xvstatestore_t &&);
            xvstatestore_t(const xvstatestore_t &);
            xvstatestore_t & operator = (xvstatestore_t &&);
            xvstatestore_t & operator = (const xvstatestore_t &);
            
        public:
            //caller need to cast (void*) to related ptr
            virtual void*           query_interface(const int32_t _enum_xobject_type_) override;
            xvblkstatestore_t*      get_blkstate_store() const {return (xvblkstatestore_t*)this;}
            xvactstatestore_t*      get_actstate_store() const {return (xvactstatestore_t*)this;}
            
        public: //universal api
            //note: when target_account = block_to_hold_state.get_account() -> return get_block_state
            //othewise return get_account_state
            virtual xauto_ptr<xvexestate_t> load_state(const xvaccount_t & target_account,xvblock_t * block_to_hold_state);
        };
        
    }//end of namespace of base
}//end of namespace top
