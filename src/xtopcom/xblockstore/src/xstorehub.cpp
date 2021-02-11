// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xthread.h"

#include "xblockstore_face.h"
#include "xvunithub.h"
#include "xvtablestore.h"

namespace top
{
    namespace store
    {
        class xblockstorehub_impl : public xblockstorehub_t
        {
            friend class xblockstorehub_t;
        private:
            xblockstorehub_impl();
            virtual ~xblockstorehub_impl();
            xblockstorehub_impl(const xblockstorehub_impl &);
            xblockstorehub_impl & operator = (const xblockstorehub_impl &);
        public:
            virtual base::xvblockstore_t*  get_block_store(xstore_face_t & _persist_db,const std::string & account) override;

        public://debug purpose only
            virtual base::xvblockstore_t*  create_block_store(xstore_face_t & _persist_db,const std::string & store_path) override;  //create multiple blockstore

        private:
            base::xiothread_t*    m_monitor_thread;
        };

        xblockstorehub_impl::xblockstorehub_impl()
        {
            m_monitor_thread = nullptr;

            m_monitor_thread = base::xcontext_t::instance().find_thread(base::xiothread_t::enum_xthread_type_monitor, false);
            if(NULL == m_monitor_thread)
                m_monitor_thread = base::xiothread_t::create_thread(base::xcontext_t::instance(),base::xiothread_t::enum_xthread_type_monitor,-1);
            xassert(m_monitor_thread != nullptr);

            xvblockstore_impl::init_store(base::xcontext_t::instance());//do initialize for all store objects

        }
        xblockstorehub_impl::~xblockstorehub_impl()
        {
            if(m_monitor_thread != nullptr) //note:dont stop thread, let run until process quit
                m_monitor_thread->release_ref();
        }

        base::xvblockstore_t*  xblockstorehub_impl::get_block_store(xstore_face_t & _persist_db,const std::string & account)
        {
            static xvblockstore_impl * _static_blockstore = nullptr;
            if(_static_blockstore)
                return _static_blockstore;

            _static_blockstore = new xvtablestore_impl(std::string("/"),_persist_db,*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id());
            return _static_blockstore;
        }

        //create multiple blockstore
        base::xvblockstore_t*     xblockstorehub_impl::create_block_store(xstore_face_t & _persist_db,const std::string & store_path)  //debug purpose only
        {
            #ifdef DEBUG
            return new xvtablestore_impl(store_path,_persist_db,*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id());
            #else
            return new xvtablestore_impl(store_path,_persist_db,*m_monitor_thread->get_context(),m_monitor_thread->get_thread_id());
            #endif
        }

        xblockstorehub_t &  xblockstorehub_t::instance()
        {
            static xblockstorehub_impl _static_blockstore_hub;
            return _static_blockstore_hub;
        }

        xblockstorehub_t::xblockstorehub_t()
        {
        }
        xblockstorehub_t::~xblockstorehub_t()
        {
        }
    };//end of namespace of vstore
};//end of namespace of top
