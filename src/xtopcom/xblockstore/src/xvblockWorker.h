// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvaccount.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvledger.h"
#include <atomic>
namespace top
{
    namespace store
    {
        //manage to prune blocks
        class xvblockWorker 
        {
         
         public:

            bool   push_block();

         private:

            thread                                          m_execute_thread;

    
        std::map<std::string, std::vector<uint64_t>> m_account_blocks;
        std::map<std::string, uint64_t> m_account_execute_height;

         /*

                     std::vector<std::thread> all_thread;
            for (size_t i = 0; i < THREAD_NUM; i++) {
                std::vector<std::string> addresses = thread_address[i];
                std::thread th(db_delta_migrate_v2_to_v3_addresses, addresses, _vblockdb.get(), dst_blockstore, std::ref(thread_total_count[i]));
                all_thread.emplace_back(std::move(th));
            }
            */
        };
    
    }
}
