// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvledger/xvledger.h"
#include "xblockstore_face.h"
#include "xvunithub.h"
#include "xvblockpruner.h"

namespace top
{
    namespace store
    {
        class xblockstorehub_impl
        {
        public:
            xblockstorehub_impl();
            ~xblockstorehub_impl();
        private:
            xblockstorehub_impl(const xblockstorehub_impl &);
            xblockstorehub_impl & operator = (const xblockstorehub_impl &);
        public:
            base::xvblockstore_t*  create_block_store(base::xvdbstore_t* xvdb_ptr);
            base::xvblockstore_t*  get_block_store();
        private:
            base::xiothread_t*     m_monitor_thread;
        };

        xblockstorehub_impl::xblockstorehub_impl()
        {
            m_monitor_thread = nullptr;

            m_monitor_thread = base::xcontext_t::instance().find_thread(base::xiothread_t::enum_xthread_type_monitor, false);
            if(NULL == m_monitor_thread)
                m_monitor_thread = base::xiothread_t::create_thread(base::xcontext_t::instance(),base::xiothread_t::enum_xthread_type_monitor,-1);
            xassert(m_monitor_thread != nullptr);
        }
        xblockstorehub_impl::~xblockstorehub_impl()
        {
            if(m_monitor_thread != nullptr) //note:dont stop thread, let run until process quit
                m_monitor_thread->release_ref();
        }

        base::xvblockstore_t*  xblockstorehub_impl::get_block_store()
        {
            static xvblockstore_impl * _static_blockstore = nullptr;
            if(_static_blockstore)
                return _static_blockstore;

            base::xvdbstore_t* xvdb_ptr = base::xvchain_t::instance().get_xdbstore();
            _static_blockstore = new xvblockstore_impl(*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id(),xvdb_ptr);

            //set into global management
            base::xvchain_t::instance().set_xblockstore(_static_blockstore);
            return _static_blockstore;
        }

        base::xvblockstore_t*  xblockstorehub_impl::create_block_store(base::xvdbstore_t* xvdb_ptr)
        {
            if(NULL == xvdb_ptr)
                xvdb_ptr = base::xvchain_t::instance().get_xdbstore();

            xvblockstore_impl * _blockstore = new xvblockstore_impl(*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id(),xvdb_ptr);
            return _blockstore;
        }

        base::xvblockstore_t*  get_vblockstore()
        {
            static xblockstorehub_impl _static_blockstore_hub;
            return _static_blockstore_hub.get_block_store();
        }

        base::xvblockstore_t*  create_vblockstore(base::xvdbstore_t* xvdb_ptr)
        {
            static xblockstorehub_impl _static_blockstore_hub;
            return _static_blockstore_hub.create_block_store(xvdb_ptr);
        }
    
        bool  install_block_recycler(base::xvdbstore_t* xvdb_ptr)
        {
            xkinfo("install_block_recycler start");
            
            if(base::xvchain_t::instance().get_xrecyclemgr()->get_block_recycler() != NULL)
                return true; //has been installed
            
            if(NULL == xvdb_ptr)
                xvdb_ptr = base::xvchain_t::instance().get_xdbstore();
            
            xassert(xvdb_ptr != NULL); //xvdb_ptr must be valid
            base::xblockrecycler_t* recycler = new xvblockprune_impl(*xvdb_ptr);
            //recycler will be transfered to xrecyclemgr who manage lifecycle
            return base::xvchain_t::instance().get_xrecyclemgr()->set_block_recycler(*recycler);
        }
    
        bool enable_block_recycler(bool enable)
        {
            if (enable)
                return base::xvchain_t::instance().get_xrecyclemgr()->turn_on_recycler(base::enum_vdata_recycle_type_block);
            else
                return base::xvchain_t::instance().get_xrecyclemgr()->turn_off_recycler(base::enum_vdata_recycle_type_block);
        }

    };//end of namespace of vstore
};//end of namespace of top
