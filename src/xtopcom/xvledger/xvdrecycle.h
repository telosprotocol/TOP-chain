// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xobject.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvstate.h"
#include "xbasic/xmodule_type.h"

namespace top
{
    namespace base
    {
        enum enum_vdata_recycle_type
        {
            enum_vdata_recycle_type_start       = 0,
            enum_vdata_recycle_type_block       = enum_vdata_recycle_type_start + 0,//block
            enum_vdata_recycle_type_state       = enum_vdata_recycle_type_start + 1,//state object
            enum_vdata_recycle_type_tx          = enum_vdata_recycle_type_start + 2,//transactions,tx
            enum_vdata_recycle_type_contract    = enum_vdata_recycle_type_start + 3,//contracts
            enum_vdata_recycle_type_object      = enum_vdata_recycle_type_start + 4,//timeout object,or TTL obj
            enum_vdata_recycle_type_memory      = enum_vdata_recycle_type_start + 5,//unsed memory/cache
            enum_vdata_recycle_type_tmp         = enum_vdata_recycle_type_start + 6,//temp data/files
            enum_vdata_recycle_type_log         = enum_vdata_recycle_type_start + 7,//log files
            enum_vdata_recycle_type_end         = enum_vdata_recycle_type_start + 8,//[start,end)
        };
    
        class xvdrecycle_t : public xobject_t
        {
            friend class xvdrecycle_mgr;
        protected:
            xvdrecycle_t(enum_vdata_recycle_type type);
            virtual ~xvdrecycle_t();
        private:
            xvdrecycle_t();
            xvdrecycle_t(xvdrecycle_t &&);
            xvdrecycle_t(const xvdrecycle_t &);
            xvdrecycle_t & operator = (const xvdrecycle_t &);
            
        public:
            inline enum_vdata_recycle_type  get_recycle_type() const {return m_recycle_type;}
            
        private: //only allow xvdrecycle_mgr mangage reference
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
        private:
            enum_vdata_recycle_type m_recycle_type;
        };
        
        class xblockrecycler_t : public xvdrecycle_t
        {
        protected:
            xblockrecycler_t();
            virtual ~xblockrecycler_t();
        private:
            xblockrecycler_t(xblockrecycler_t &&);
            xblockrecycler_t(const xblockrecycler_t &);
            xblockrecycler_t & operator = (const xblockrecycler_t &);
            
        public:
            virtual bool   recycle(const xvbindex_t * block) = 0;//recyle one block
            virtual bool   recycle(const std::vector<xvbindex_t*> & mblocks) = 0;//recycle multiple blocks
            virtual bool   recycle(const xvaccount_t & account_obj,xblockmeta_t & account_meta) = 0;//recylce possible blocks under account
            virtual bool   watch(const chainbase::enum_xmodule_type mod_id, const xvaccount_t & account_obj) = 0; // just for table
            virtual bool   unwatch(const chainbase::enum_xmodule_type mod_id, const xvaccount_t & account_obj) = 0; // just for table
            virtual bool   refresh(const chainbase::enum_xmodule_type mod_id, const xvaccount_t & account_obj, uint64_t permit_prune_right_boundary) = 0; // just for table
        };
    
        class xstaterecycler_t : public xvdrecycle_t
        {
        protected:
            xstaterecycler_t();
            virtual ~xstaterecycler_t();
        private:
            xstaterecycler_t(xstaterecycler_t &&);
            xstaterecycler_t(const xstaterecycler_t &);
            xstaterecycler_t & operator = (const xstaterecycler_t &);
            
        public:
            virtual bool   recycle(const xvbstate_t * state) = 0;//recyle one block
            virtual bool   recycle(const std::vector<xvbstate_t*> & mstates) = 0;//recycle multiple blocks
            virtual bool   recycle(const xvaccount_t & account_obj,xblockmeta_t & account_meta) = 0;//recylce possible states under account
        };
    
        //global plugin to manage for data recycle and garbage collection
        class xvdrecycle_mgr : public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvdrecycle_mgr";} //"xvdrecycle_mgr"
            virtual std::string         get_obj_name() const override {return name();}
        protected:
            xvdrecycle_mgr();
            virtual ~xvdrecycle_mgr();
        private:
            xvdrecycle_mgr(xvdrecycle_mgr &&);
            xvdrecycle_mgr(const xvdrecycle_mgr &);
            xvdrecycle_mgr & operator = (const xvdrecycle_mgr &);
            
        public://turn on/off for recycler. note: all are off as default
            bool              turn_on_recycler(enum_vdata_recycle_type target);
            bool              turn_off_recycler(enum_vdata_recycle_type target);
            
        public:
            xblockrecycler_t* get_block_recycler(bool break_through = false);
            bool              set_block_recycler(xblockrecycler_t& new_recycler);
            
        protected:
            xvdrecycle_t*     get_recycler(enum_vdata_recycle_type target, bool break_through = false);
            bool              set_recycler(xvdrecycle_t& new_recycler);
            
        private: //only allow xvchain_t mangage reference
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
            
        private:
            xvdrecycle_t*     m_recyclers_obj[enum_vdata_recycle_type_end - enum_vdata_recycle_type_start];
            int8_t            m_recycler_switch[enum_vdata_recycle_type_end - enum_vdata_recycle_type_start];
            //turn on when > 0
        };
    

    }
}
