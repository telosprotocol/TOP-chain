// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../xvaccount.h"
#include "../xvblock.h"
#include "../xvstate.h"
#include "../xvbbuild.h"
#include "../xvledger.h"
#include "xunitblock.hpp"
 
int test_block_builder(bool is_stress_test)
{
    top::base::xvblock_t::register_object(top::base::xcontext_t::instance());
    
    //create test account
    const std::string account_addr = top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account,top::base::xvaccount_t::make_ledger_id(top::base::enum_main_chain_id, top::base::enum_chain_zone_consensus_index),std::string("1234567890abcdef"));
    {
        const xvid_t account_id = top::base::xvaccount_t::get_xid_from_account(account_addr);
        xassert(account_id != 0);
        xassert(top::base::xvaccount_t::get_addrtype_from_account(account_addr) == top::base::enum_vaccount_addr_type_secp256k1_user_account);
        xassert(top::base::xvaccount_t::get_chainid_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_main_chain_id);
        xassert(top::base::xvaccount_t::get_zoneindex_from_ledgerid(top::base::xvaccount_t::get_ledgerid_from_account(account_addr)) == top::base::enum_chain_zone_consensus_index);
    }
    
    top::base::xauto_ptr<top::base::xvblock_t> genesis_block(top::base::xvchain_t::instance().get_xblockstore()->load_block_object(account_addr,0,0,false));
    
    top::base::xauto_ptr<top::base::xvblock_t> block_1( top::test::xunitblock_t::create_unitblock(account_addr,1,1,1,genesis_block->get_block_hash(),genesis_block->get_block_hash(),genesis_block->get_height()));

    
    top::base::xvbmaker_t * unit_maker = new top::base::xvbmaker_t(*block_1->get_header(),NULL);
    
    top::test::tep0_tx * deposit = new top::test::tep0_tx(account_addr,100);
    top::test::tep0_tx * withdraw = new top::test::tep0_tx(account_addr,-10);
    std::vector<top::base::xvtransact_t*> input_txs;
    input_txs.push_back(deposit);
    input_txs.push_back(withdraw);
    
    xassert(unit_maker->build_entity(input_txs));
    xassert(unit_maker->make_input());
    xassert(unit_maker->make_output());
    xassert(unit_maker->build_block(block_1->get_cert()));
 
    delete unit_maker;
    
    printf("/////////////////////////////// [test_block_builder] finish ///////////////////////////////  \n");
    return 0;
}
