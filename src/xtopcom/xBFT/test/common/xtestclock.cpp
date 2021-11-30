// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestclock.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xdata/xnative_contract_address.h"

#ifdef __MAC_PLATFORM__
    #include "xblockstore/xblockstore_face.h"
    #include "xtestdb.hpp"
#endif

namespace top
{
    namespace test
    {
        xtestclocker_t::xtestclocker_t()
        {
            m_blockstore = NULL;
            
            m_clock_account = sys_contract_beacon_timer_addr;
 
#ifdef __MAC_PLATFORM__
            const std::string  default_path = std::string("/");
            xstoredb_t* _persist_db = NULL;
            if(base::xvchain_t::instance().get_xdbstore() == NULL)
            {
                _persist_db = new xstoredb_t(default_path);
                base::xvchain_t::instance().set_xdbstore(_persist_db);
            }
            //m_blockstore = store::get_vblockstore();
            m_blockstore = store::create_vblockstore(_persist_db);
#else
            m_blockstore = new xunitblockstore_t();
#endif
        }
        
        xtestclocker_t::~xtestclocker_t()
        {
        }
        
        base::xauto_ptr<base::xvblock_t>   xtestclocker_t::get_latest_clock()
        {
            return m_blockstore->get_latest_cert_block(m_clock_account);
        }
        
        base::xvblock_t*   xtestclocker_t::on_clock_fire()
        {
            base::xauto_ptr<base::xvblock_t> last_block = m_blockstore->get_latest_cert_block(m_clock_account); //return ptr that has been added reference
            if(last_block == nullptr)//blockstore has been closed
                return NULL;
            
            std::string empty_txs;
            base::xauto_ptr<base::xvblock_t> last_full_block = m_blockstore->get_latest_committed_block(m_clock_account);
            base::xvblock_t* clock_block = xclockblock_t::create_clockblock(m_clock_account,last_block->get_height() + 1,last_block->get_clock() + 1,last_block->get_viewid() + 1,last_block->get_block_hash(),last_full_block->get_block_hash(),last_full_block->get_height(),empty_txs,empty_txs);
            clock_block->reset_prev_block(last_block.get()); //point previous block
 
            xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
            clock_block->get_cert()->set_validator(_wildaddr);
            
            //convert proposal to commit block for test purpose
            clock_block->set_verify_signature("test-signature");
            clock_block->set_block_flag(base::enum_xvblock_flag_authenticated);
            clock_block->set_block_flag(base::enum_xvblock_flag_locked);
            clock_block->set_block_flag(base::enum_xvblock_flag_committed);
            
            xdbg("xtestclocker_t::on_clock_fire,new clock(%s)",clock_block->dump().c_str());
            base::xvaccount_t account(clock_block->get_account());
            m_blockstore->store_block(account,clock_block);
            return clock_block;
        }
    };
};
