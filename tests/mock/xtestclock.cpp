// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestclock.hpp"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xgenesis_data.h"

namespace top
{
    namespace mock
    {
        xtestclocker_t::xtestclocker_t()
        {
            m_blockstore = NULL;
            // const std::string account_publick_addr = "clock1234567890000000";
            // m_clock_account =  top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account, 0, account_publick_addr);

            m_clock_account = sys_contract_beacon_timer_addr;

            // m_blockstore = new xunitblockstore_t();
        }

        xtestclocker_t::~xtestclocker_t()
        {
            if(m_blockstore != NULL)
                m_blockstore->release_ref();
        }

        base::xvblock_t*   xtestclocker_t::on_clock_fire()
        {
            std::string empty_txs;
            std::vector<base::xvblock_t*> parents_blocks;
            base::xvblock_t*  clock_block = m_blockstore->create_proposal_block(m_clock_account,empty_txs,empty_txs);
            // clock_block->get_cert()->set_validator(-1);
            // clock_block->get_cert()->set_verify_threshhold(1); //any one

            //convert proposal to commit block for test purpose
            clock_block->set_verify_signature("test-signature");
            clock_block->set_block_flag(base::enum_xvblock_flag_authenticated);

            m_blockstore->store_block(clock_block);
            return clock_block;
        }
    };
};
