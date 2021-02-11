// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#if defined(__MAC_PLATFORM__) && defined(__ENABLE_MOCK_XSTORE__)
    // dummy include
#else
    #include "xdata/xtableblock.h"
#endif

#include "xblockstore_face.h"
#include "xvtablestore.h"


namespace top
{
    namespace store
    {

        xvtablestore_impl::xvtablestore_impl(const std::string & blockstore_path,xstore_face_t & _persist_db,base::xcontext_t & _context,const int32_t target_thread_id)
            :xvblockstore_impl(blockstore_path, _persist_db, _context, target_thread_id)
        {
        }

        xvtablestore_impl::~xvtablestore_impl()
        {
        }

        bool xvtablestore_impl::store_tableblock_units(base::xvblock_t* block)
        {
        #if defined(__MAC_PLATFORM__) && defined(__ENABLE_MOCK_XSTORE__)
            // do nothing
        #else
            if(block == nullptr)
                return false;

#if 0
            if(block->get_header()->get_block_class() != base::enum_xvblock_class_nil) //only split for non-nil block
            {
                if(  (block->get_header()->get_block_level() == base::enum_xvblock_level_table)
                   &&(block->check_block_flag(base::enum_xvblock_flag_committed)) ) //only handle commit of tableblcok
                {
                    xassert(!block->get_input().empty());
                    xassert(!block->get_output().empty());

                    auto tableblock = dynamic_cast<data::xtable_block_t*>(block);
                    xassert(tableblock != nullptr);
                    if (tableblock != nullptr)
                    {
                        auto & unitblocks = tableblock->get_consensused_units();
                        xassert(!unitblocks.empty());
                        for (auto & unitblock : unitblocks)
                        {
                            xvblockstore_impl::store_block(unitblock);
                        }
                    }
                }
            }
#endif
        #endif
            return true;
        }

        bool  xvtablestore_impl::store_block(base::xvblock_t* block)    //update old one or insert as new
        {
            store_tableblock_units(block);
            return xvblockstore_impl::store_block(block);
        }

        bool xvtablestore_impl::store_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            store_tableblock_units(block);
            return xvblockstore_impl::store_block(account, block);
        }

        bool xvtablestore_impl::store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks)
        {
            for (auto & block : batch_store_blocks)
            {
                store_tableblock_units(block);
            }
            return xvblockstore_impl::store_blocks(account, batch_store_blocks);
        }

    } //end of namespace of store
} //end of namespace of top
