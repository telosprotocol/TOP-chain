// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvblock.h"
#include "xvstate.h"
#include "xvaccount.h"
#include "xbase/xlru_cache.h"

namespace top
{
    namespace base
    {
        //chain managed account'state by individual unit block,return block-state(xvbstate_t)
        class xvblkstatestore_t
        {
            enum
            {
                enum_max_bstate_newest_count            = 5,  //max persisted store bstate count per account
                enum_max_bstate_fulltable_count         = 3,  //max persisted store bstate for fulltable snapshot sync
                enum_max_table_bstate_lru_cache_max     = 256, //max table state lru cache count
                enum_max_unit_bstate_lru_cache_max      = 1024, //max unit state lru cache count
            };
        protected:
            xvblkstatestore_t();
            virtual ~xvblkstatestore_t(){};
        private:
            xvblkstatestore_t(xvblkstatestore_t &&);
            xvblkstatestore_t(const xvblkstatestore_t &);
            xvblkstatestore_t & operator = (xvblkstatestore_t &&);
            xvblkstatestore_t & operator = (const xvblkstatestore_t &);

        public:
            virtual xauto_ptr<xvbstate_t> get_block_state(xvblock_t * target_block, const int etag=0);
            virtual xauto_ptr<xvbstate_t> get_block_state(const xvaccount_t & account,const uint64_t block_height,const std::string& block_hash, const int etag=0);
            virtual xauto_ptr<xvbstate_t> get_latest_connectted_block_state(const xvaccount_t & account, const int etag=0);
            virtual xauto_ptr<xvbstate_t> get_committed_block_state(const xvaccount_t & account,const uint64_t block_height, const int etag=0);
            virtual bool                  get_full_block_offsnapshot(xvblock_t * target_block, const int etag=0);

            virtual bool                  execute_block(xvblock_t * target_block, const int etag=0);

        private:
            xauto_ptr<xvbstate_t>     get_block_state_internal(const xvaccount_t & target_account, xvblock_t * target_block, const int etag=0);
            bool                      write_state_to_db(const xvaccount_t & target_account, xvbstate_t & target_state,const std::string & target_block_hash);

            xvbstate_t*               read_state_from_db(const xvbindex_t * for_block);
            xvbstate_t*               read_state_from_db(const xvaccount_t & target_account, const xvblock_t * for_block);
            xvbstate_t*               read_state_from_db(const xvaccount_t & target_account,uint64_t block_height,const std::string & block_hash);

            bool                      delete_state_of_db(const xvbindex_t & target_index);
            bool                      delete_state_of_db(const xvblock_t & target_block);
            bool                      delete_state_of_db(const xvaccount_t & target_account,uint64_t block_height,const std::string & block_hash);
            bool                      delete_states_of_db(const xvaccount_t & target_account,const uint64_t block_height);

            bool                      load_latest_blocks_and_state(xvblock_t * target_block, xobject_ptr_t<xvbstate_t> & base_bstate, std::map<uint64_t, xobject_ptr_t<xvblock_t>> & latest_blocks);
            xobject_ptr_t<xvbstate_t> rebuild_bstate(const xvaccount_t & target_account, const xobject_ptr_t<xvbstate_t> & base_state, const std::map<uint64_t, xobject_ptr_t<xvblock_t>> & latest_blocks);
            xobject_ptr_t<xvbstate_t> make_state_from_current_block(const xvaccount_t & target_account, xvblock_t * current_block);
            void                      clear_persisted_state(const xvaccount_t & target_account, xvblock_t * target_block);
            xauto_ptr<xvbstate_t>     execute_target_block(const xvaccount_t & target_account, xvblock_t * target_block);

            xobject_ptr_t<xvbstate_t> get_lru_cache(base::enum_xvblock_level blocklevel, const std::string & hash);
            void                      set_lru_cache(base::enum_xvblock_level blocklevel, const std::string & hash, const xobject_ptr_t<xvbstate_t> & state);
            bool                      recover_highest_execute_height(const xvaccount_t & target_account, uint64_t old_execute_height);
            bool                      try_update_execute_height(const xvaccount_t & target_account);
            void                      set_latest_executed_info(const xvaccount_t & target_account, uint64_t height,const std::string & blockhash);
            uint64_t                  get_latest_executed_block_height(const xvaccount_t & target_account);

        private:
            base::xlru_cache<std::string, xobject_ptr_t<xvbstate_t>> m_table_state_cache;  //tablestate cache
            base::xlru_cache<std::string, xobject_ptr_t<xvbstate_t>> m_unit_state_cache;  //unitstate cache
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
