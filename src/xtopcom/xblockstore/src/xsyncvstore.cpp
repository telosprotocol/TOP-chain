// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xsyncvstore_face.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"

#ifdef ENABLE_METRICS
    #include "xmetrics/xmetrics.h"
#endif

namespace top
{
    namespace store
    {
        xsyncvstore_t::xsyncvstore_t(base::xvcertauth_t & _certauth,base::xvblockstore_t & _xdb)
        {
            _certauth.add_ref();
            m_vcertauth_ptr = &_certauth;
            _xdb.add_ref();
            m_vblockstore_ptr = &_xdb;

            xkinfo("xsyncvstore_t::create");
        }

        xsyncvstore_t::~xsyncvstore_t()
        {
            xkinfo("xsyncvstore_t::destroy");
            m_vcertauth_ptr->release_ref();
            m_vblockstore_ptr->release_ref();
        }

        bool  xsyncvstore_t::store_block(base::xvblock_t* target_block)  //cache and hold block
        {
            if(is_close())
            {
                xwarn_err("xsyncvstore_t has closed");
                return false;
            }
            if(nullptr == target_block)
                return false;

#ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_sync_store_block_time", target_block->get_account() + ":" + std::to_string(target_block->get_height()));
#endif
            target_block->reset_block_flags(); //No.1 safe rule: clean all flags first when sync/replicated one block
            if(   (false == target_block->is_input_ready(true))
               || (false == target_block->is_output_ready(true))
               || (false == target_block->is_valid(true)) )
            {
                xerror("xsyncvstore_t::store_block,an unvalid block=%s",target_block->dump().c_str());
                return false;
            }
            base::enum_vcert_auth_result result = get_vcertauth()->verify_muti_sign(target_block);
            if(result != base::enum_vcert_auth_result::enum_successful)//do heavy job at caller'thread without xsyncvstore_t ' lock involved
            {
                xwarn("xsyncvstore_t::store_block,fail-certauth for block=%s",target_block->dump().c_str());
                return false;
            }
            target_block->set_block_flag(base::enum_xvblock_flag_authenticated); //now safe to set flag of enum_xvblock_flag_authenticated

            #if defined(DEBUG) //TODO,remove those test code later
            if(!target_block->get_cert()->get_extend_cert().empty())
            {
                target_block->set_block_flag(base::enum_xvblock_flag_locked);
                target_block->set_block_flag(base::enum_xvblock_flag_committed);
            }
            #endif
            auto res = get_vblockstore()->store_block(target_block);//store block with cert status and to let consensus know it first
            xdbg("xsyncvstore_t::store_block %s result:%d", target_block->dump().c_str(), res);
#ifdef ENABLE_METRICS
            XMETRICS_COUNTER_INCREMENT("blockstore_sync_store_block", 1);
#endif
            return res;
        }

        bool  xsyncvstore_t::store_block(base::xvaccount_t & target_account,base::xvblock_t* target_block)
        {
            if(is_close())
            {
                xwarn_err("xsyncvstore_t has closed");
                return false;
            }
            if(nullptr == target_block)
                return false;

            if(target_account.get_address() != target_block->get_account())
            {
                xerror("xsyncvstore_t::store_block,block NOT match account:%",target_account.get_address().c_str());
                return false;
            }
            target_block->reset_block_flags(); //No.1 safe rule: clean all flags first when sync/replicated one block
            if(   (false == target_block->is_input_ready(true))
               || (false == target_block->is_output_ready(true))
               || (false == target_block->is_valid(true)) )
            {
                xerror("xsyncvstore_t::store_block,an unvalid block=%s",target_block->dump().c_str());
                return false;
            }
            base::enum_vcert_auth_result result = get_vcertauth()->verify_muti_sign(target_block);
            if(result != base::enum_vcert_auth_result::enum_successful)//do heavy job at caller'thread without xsyncvstore_t ' lock involved
            {
                xwarn("xsyncvstore_t::store_block,fail-certauth for block=%s",target_block->dump().c_str());
                return false;
            }
            target_block->set_block_flag(base::enum_xvblock_flag_authenticated); //now safe to set flag of enum_xvblock_flag_authenticated

            #if defined(DEBUG) //TODO,remove those test code later
            if(!target_block->get_cert()->get_extend_cert().empty())
            {
                target_block->set_block_flag(base::enum_xvblock_flag_locked);
                target_block->set_block_flag(base::enum_xvblock_flag_committed);
            }
            #endif
            auto res = get_vblockstore()->store_block(target_block); //store block with cert status and to let consensus know it first
            xdbg("xsyncvstore_t::store_block %s result:%d", target_block->dump().c_str(), res);
            return res;
        }

    };//end of namespace of store

};//end of namespace of top
