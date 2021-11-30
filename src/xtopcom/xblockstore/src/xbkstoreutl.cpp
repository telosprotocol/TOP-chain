// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbkstoreutl.h"

namespace top
{
    namespace store
    {
        xvblockplugin_t::xvblockplugin_t(base::xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms)
        :xvactplugin_t(parent_obj,idle_timeout_ms,base::enum_xvaccount_plugin_blockmgr)
        {
            m_layer2_cache_meta = NULL;
        }
        
        xvblockplugin_t::~xvblockplugin_t()
        {
            if(m_layer2_cache_meta != NULL)
                delete m_layer2_cache_meta;
        }
        
        bool  xvblockplugin_t::init_meta(const base::xvactmeta_t & meta)
        {
            if(NULL == m_layer2_cache_meta)
            {
                base::xblockmeta_t* new_meta_obj = new base::xblockmeta_t(meta.clone_block_meta());
                m_layer2_cache_meta = new_meta_obj;
                return true;
            }
            xassert(NULL == m_layer2_cache_meta);
            return false;
        }
        
        const base::xblockmeta_t*   xvblockplugin_t::get_block_meta() const
        {
            return m_layer2_cache_meta;
        }
        
        bool  xvblockplugin_t::save_meta()
        {
            get_account_obj()->update_block_meta(this);
            get_account_obj()->save_meta();//force to save one
            return true;
        }
        
        bool  xvblockplugin_t::update_meta()
        {
            return get_account_obj()->update_block_meta(this);
        }
        
        bool   xvblockplugin_t::set_latest_deleted_block_height(const uint64_t height)
        {
            if(NULL != m_layer2_cache_meta)
            {
                if(height > m_layer2_cache_meta->_highest_deleted_block_height)
                {
                    base::xatomic_t::xstore(m_layer2_cache_meta->_highest_deleted_block_height, height);
                    return true;
                }
            }
            return false;
        }
    
        xblockevent_t::xblockevent_t(enum_blockstore_event type,base::xvbindex_t* index,xvblockplugin_t* plugin,const base::xblockmeta_t& meta)
        : _meta_info(meta)
        {
            _event_type     = type;
            _target_index   = index;
            _target_plugin  = plugin;
            if(_target_index != NULL)
                _target_index->add_ref();
            
            if(_target_plugin != NULL)
                _target_plugin->add_ref();
        }
        xblockevent_t::xblockevent_t(xblockevent_t && obj)
        :_meta_info(obj._meta_info)
        {
            _event_type     = obj._event_type;
            _target_index   = obj._target_index;
            _target_plugin  = obj._target_plugin;
            obj._target_index   = NULL;
            obj._target_plugin  = NULL;
        }
        xblockevent_t::xblockevent_t(const xblockevent_t & obj)
        :_meta_info(obj._meta_info)
        {
            _event_type     = obj._event_type;
            _target_index   = obj._target_index;
            _target_plugin  = obj._target_plugin;
            if(_target_index != NULL)
                _target_index->add_ref();
            if(_target_plugin != NULL)
                _target_plugin->add_ref();
        }
        xblockevent_t & xblockevent_t::operator = (const xblockevent_t & obj)
        {
            base::xvbindex_t* old_index     = _target_index;
            base::xvactplugin_t* old_plugin = _target_plugin;
            
            _event_type     = obj._event_type;
            _target_index   = obj._target_index;
            _target_plugin  = obj._target_plugin;
            _meta_info      = obj._meta_info;
            if(_target_index != NULL)
                _target_index->add_ref();
            if(_target_plugin != NULL)
                _target_plugin->add_ref();
            
            if(old_index != NULL)
                old_index->release_ref();
            
            if(old_plugin != NULL)
                old_plugin->release_ref();
            
            return *this;
        }
        xblockevent_t::~xblockevent_t()
        {
            if(_target_index != NULL)
                _target_index->release_ref();
            if(_target_plugin != NULL)
                _target_plugin->release_ref();
        }
    }
}
