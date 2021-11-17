// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xsyncvstore_face.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"

#include "xmetrics/xmetrics.h"

namespace top
{
    namespace store
    {
        xsyncvstore_t::xsyncvstore_t(base::xvcertauth_t & _certauth,base::xvblockstore_t & _xdb)
        {
            _certauth.add_ref();
            m_vcertauth_ptr = &_certauth;
            m_vblockstore_ptr = &_xdb;

            xkinfo("xsyncvstore_t::create");
        }

        xsyncvstore_t::~xsyncvstore_t()
        {
            xkinfo("xsyncvstore_t::destroy");
            m_vcertauth_ptr->release_ref();
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

            //XTODO,add more restrict for genesis block from sync way,right now temporary enable it
            if( (target_block->get_height() == 0) && (target_block->get_header()->get_block_level() == base::enum_xvblock_level_unit) )
            {
                xwarn("xsyncvstore_t::store_block,not allow sync genesis block for unit block,which must be generated from local");
            }
            
            
            XMETRICS_TIME_RECORD_KEY("blockstore_sync_store_block_time", target_block->get_account() + ":" + std::to_string(target_block->get_height()));
            

            base::xvaccount_t target_account(target_block->get_account());
            #ifndef __PRE_CHECK_AUTH_BEFORE_SYNC_STORE__
            //first check existing one or not
            base::xauto_ptr<base::xvbindex_t> cert_idx(get_vblockstore()->load_block_index(target_account, target_block->get_height(), target_block->get_block_hash()));
            if(cert_idx)
            {
                xdbg_info("xsyncvstore_t::store_block,an duplicated block=%s",target_block->dump().c_str());
                return true;
            }
            
            XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_blockstore, 1);
            target_block->reset_block_flags(); //No.1 safe rule: clean all flags first when sync/replicated one block
            base::enum_vcert_auth_result result = get_vcertauth()->verify_muti_sign(target_block);
            if(result != base::enum_vcert_auth_result::enum_successful)//do heavy job at caller'thread without xsyncvstore_t ' lock involved
            {
                xwarn("xsyncvstore_t::store_block,fail-certauth for block=%s",target_block->dump().c_str());
                return false;
            }
            target_block->set_block_flag(base::enum_xvblock_flag_authenticated); //now safe to set flag of enum_xvblock_flag_authenticated
            #endif

            //try best to do any heavy job before acquired lock
            if(   (false == target_block->is_input_ready(true))
               || (false == target_block->is_output_ready(true))
               || (false == target_block->is_valid(true)) )
            {
                xerror("xsyncvstore_t::store_block,an unvalid block=%s",target_block->dump().c_str());
                return false;
            }
           
            auto res = get_vblockstore()->store_block(target_account,target_block);//store block with cert status and to let consensus know it first
            xdbg("xsyncvstore_t::store_block %s result:%d", target_block->dump().c_str(), res);
           
            XMETRICS_COUNTER_INCREMENT("blockstore_sync_store_block", 1);
           
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
            XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_blockstore, 1);
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
            auto res = get_vblockstore()->store_block(target_account,target_block); //store block with cert status and to let consensus know it first
            xdbg("xsyncvstore_t::store_block %s result:%d", target_block->dump().c_str(), res);
            return res;
        }

    };//end of namespace of store

};//end of namespace of top
