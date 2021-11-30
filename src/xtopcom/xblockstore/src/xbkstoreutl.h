// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvaccount.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvledger.h"

namespace top
{
    namespace store
    {
        enum enum_blockstore_event
        {
            enum_blockstore_event_committed  = 1, //block is committed
            enum_blockstore_event_revoke     = 2, //block is revoke and removed by consensus
            enum_blockstore_event_stored     = 4, //block is stored persistedly
        };
        
        class xvblockplugin_t : public base::xvactplugin_t
        {
        protected:
            xvblockplugin_t(base::xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms);
            virtual ~xvblockplugin_t();
        private:
            xvblockplugin_t();
            xvblockplugin_t(xvblockplugin_t &&);
            xvblockplugin_t(const xvblockplugin_t &);
            xvblockplugin_t & operator = (const xvblockplugin_t &);
            
        public:
            bool   set_latest_deleted_block_height(const uint64_t height);//multiple thread safe
            
        protected:
            //only allow call once
            virtual bool                    init_meta(const base::xvactmeta_t & meta) override;
            
        protected:
            virtual const base::xblockmeta_t*     get_block_meta() const override;
            virtual bool                    save_meta()  override; //peristen meta
            virtual bool                    update_meta() override;//update meta into cache
        private:
            base::xblockmeta_t*   m_layer2_cache_meta; //L2 cache for plugin,and account ' meta is L1 cache
        };
        
        class xblockevent_t
        {
        public:
            xblockevent_t(enum_blockstore_event type,base::xvbindex_t* index,xvblockplugin_t* plugin,const base::xblockmeta_t& meta);
            xblockevent_t(xblockevent_t && obj);
            xblockevent_t(const xblockevent_t & obj);
            xblockevent_t & operator = (const xblockevent_t & obj);
            ~xblockevent_t();
        private:
            xblockevent_t();
        public:
            inline enum_blockstore_event        get_type() const {return _event_type;}
            inline base::xvbindex_t*            get_index()const {return _target_index;}
            inline xvblockplugin_t*             get_plugin()const {return _target_plugin;}
            inline const base::xblockmeta_t&    get_meta() const {return _meta_info;}
        protected:
            enum_blockstore_event   _event_type;
            base::xvbindex_t*       _target_index;
            xvblockplugin_t*        _target_plugin;
            base::xblockmeta_t      _meta_info;
        };
    }
}
