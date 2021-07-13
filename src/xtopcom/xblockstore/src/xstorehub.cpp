// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvledger/xvledger.h"
#include "xblockstore_face.h"
#include "xvunithub.h"

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
            _static_blockstore = new xvblockstore_impl(xvdb_ptr->get_store_path(),*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id(),xvdb_ptr);

            //set into global management
            base::xvchain_t::instance().set_xblockstore(_static_blockstore);
            return _static_blockstore;
        }

        base::xvblockstore_t*  xblockstorehub_impl::create_block_store(base::xvdbstore_t* xvdb_ptr)
        {
            if(NULL == xvdb_ptr)
                xvdb_ptr = base::xvchain_t::instance().get_xdbstore();

            xvblockstore_impl * _blockstore = new xvblockstore_impl(xvdb_ptr->get_store_path(),*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id(),xvdb_ptr);
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

    };//end of namespace of vstore
};//end of namespace of top
