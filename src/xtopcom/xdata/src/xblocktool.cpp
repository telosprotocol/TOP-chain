// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xvledger/xvblockstore.h"

NS_BEG2(top, data)

base::xvblock_t* xblocktool_t::create_genesis_empty_block(const std::string & account) {
    base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(account);
    if (addrtype == base::enum_vaccount_addr_type_block_contract) {
        return xemptyblock_t::create_genesis_emptytable(account);
    } else if (account == sys_contract_beacon_timer_addr || account == sys_drand_addr) {
        return xemptyblock_t::create_genesis_emptyroot(account);
    } else {
        return xemptyblock_t::create_genesis_emptyunit(account);
    }
}

base::xvblock_t*  xblocktool_t::create_genesis_empty_unit(const std::string & account) {
    return create_genesis_empty_block(account);
}
base::xvblock_t*  xblocktool_t::create_genesis_empty_table(const std::string & account) {
    return create_genesis_empty_block(account);
}

base::xvblock_t*   xblocktool_t::create_genesis_lightunit(const std::string & account, int64_t top_balance) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(top_balance);
    tx->make_tx_transfer(asset);
    // genesis transfer tx is a special transaction
    tx->set_same_source_target_address(account);
    tx->set_fire_timestamp(0);
    tx->set_expire_duration(0);
    tx->set_deposit(0);
    tx->set_digest();
    tx->set_len();
    xtransaction_result_t result;
    result.m_balance_change = top_balance;
    return xlightunit_block_t::create_genesis_lightunit(account, tx, result);
}

base::xvblock_t * xblocktool_t::create_genesis_lightunit(const std::string & account,
                                                         const xtransaction_ptr_t & genesis_tx,
                                                         const xtransaction_result_t & result) {
    return xlightunit_block_t::create_genesis_lightunit(account, genesis_tx, result);
}

base::xvblock_t*  xblocktool_t::create_next_emptyblock(base::xvblock_t* prev_block) {
    return xemptyblock_t::create_next_emptyblock(prev_block);
}

base::xvblock_t*   xblocktool_t::create_next_emptyblock(base::xvblock_t* prev_block, base::enum_xvblock_type blocktype) {
    return xemptyblock_t::create_next_emptyblock(prev_block, blocktype);
}

base::xvblock_t*  xblocktool_t::create_next_lightunit(const xlightunit_block_para_t & para, base::xvblock_t* prev_block) {
    return xlightunit_block_t::create_next_lightunit(para, prev_block);
}

base::xvblock_t*  xblocktool_t::create_next_fullunit(const xfullunit_block_para_t & para, base::xvblock_t* prev_block) {
    return xfullunit_block_t::create_next_fullunit(para, prev_block);
}

base::xvblock_t*  xblocktool_t::create_next_tableblock(const xtable_block_para_t & para, base::xvblock_t* prev_block) {
    return xtable_block_t::create_next_tableblock(para, prev_block);
}

base::xvblock_t*  xblocktool_t::create_next_tableblock(const xtable_block_para_t & para, const xblock_consensus_para_t & cs_para, base::xvblock_t* prev_block) {
    // unit should set the same consensus pare with parent tableblock
    for (auto & unit : para.get_account_units()) {
        unit->set_consensus_para(cs_para);
    }

    base::xvblock_t* proposal_block = xtable_block_t::create_next_tableblock(para, prev_block);
    data::xblock_t* block = (data::xblock_t*)proposal_block;
    block->set_consensus_para(cs_para);
    return proposal_block;
}

base::xvblock_t*   xblocktool_t::create_next_fulltable(const xfulltable_block_para_t & para, base::xvblock_t* prev_block) {
    return xfull_tableblock_t::create_next_block(para, prev_block);
}

base::xvblock_t*   xblocktool_t::create_next_emptyblock(xblockchain2_t* chain) {
    return xemptyblock_t::create_next_emptyblock(chain);
}
base::xvblock_t*   xblocktool_t::create_next_lightunit(const xlightunit_block_para_t & para, xblockchain2_t* chain) {
    return xlightunit_block_t::create_next_lightunit(para, chain);
}
base::xvblock_t*   xblocktool_t::create_next_fullunit(xblockchain2_t* chain) {
    return xfullunit_block_t::create_next_fullunit(chain);
}

std::string xblocktool_t::make_address_table_account(base::enum_xchain_zone_index zone, uint16_t subaddr) {
    xassert(zone <= base::enum_chain_zone_zec_index);
    xassert(subaddr < enum_vbucket_has_tables_count);

    static std::string tableblock_prefix[] = {
        sys_contract_sharding_table_block_addr,
        sys_contract_beacon_table_block_addr,
        sys_contract_zec_table_block_addr
    };

    std::string tableblock_addr = make_address_by_prefix_and_subaddr(tableblock_prefix[zone], subaddr).value();
    return tableblock_addr;
}
std::string xblocktool_t::make_address_shard_table_account(uint16_t subaddr) {
    return make_address_table_account(base::enum_chain_zone_consensus_index, subaddr);
}
std::string xblocktool_t::make_address_user_account(const std::string & public_key_address) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    return base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, ledger_id, public_key_address);
}
std::string xblocktool_t::make_address_native_contract(base::enum_xchain_zone_index zone, const std::string & public_key_address, uint16_t subaddr) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, zone);
    return base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_native_contract, ledger_id, public_key_address, subaddr);
}
std::string xblocktool_t::make_address_shard_sys_account(const std::string & public_key_address, uint16_t subaddr) {
    return make_address_native_contract(base::enum_chain_zone_consensus_index, public_key_address, subaddr);
}
std::string xblocktool_t::make_address_zec_sys_account(const std::string & public_key_address, uint16_t subaddr) {
    return make_address_native_contract(base::enum_chain_zone_zec_index, public_key_address, subaddr);
}
std::string xblocktool_t::make_address_beacon_sys_account(const std::string & public_key_address, uint16_t subaddr) {
    return make_address_native_contract(base::enum_chain_zone_beacon_index, public_key_address, subaddr);
}
std::string xblocktool_t::make_address_user_contract(const std::string & public_key_address) {
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    return base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_custom_contract, ledger_id, public_key_address);
}

bool xblocktool_t::is_connect_and_executed_block(base::xvblock_t* block) {
    if (block->is_genesis_block()) {
        return true;
    }
    const int block_flags = block->get_block_flags();
    if (((block_flags & base::enum_xvblock_flag_connected) != 0)
        && ((block_flags & base::enum_xvblock_flag_executed) != 0)) {
        return true;
    }
    return false;
}

base::xauto_ptr<base::xvblock_t> xblocktool_t::get_latest_committed_lightunit(base::xvblockstore_t* blockstore, const std::string & account) {
    base::xvaccount_t _vaccount(account);
    // there is mostly two empty units
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_committed_block(_vaccount);
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(_vaccount, current_height - 1, base::enum_xvblock_flag_committed, true);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xblocktool_t::get_committed_lightunit(base::xvblockstore_t* blockstore, const std::string & account, uint64_t max_height) {
    base::xvaccount_t _vaccount(account);
    // there is mostly two empty units
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->load_block_object(_vaccount, max_height, base::enum_xvblock_flag_committed, true);
    xassert(vblock->check_block_flag(base::enum_xvblock_flag_committed));
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(_vaccount, current_height - 1, base::enum_xvblock_flag_committed, true);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

bool xblocktool_t::verify_latest_blocks(const base::xblock_mptrs & latest_blocks) {
    return verify_latest_blocks(latest_blocks.get_latest_cert_block(), latest_blocks.get_latest_locked_block(), latest_blocks.get_latest_committed_block());
}

bool xblocktool_t::verify_latest_blocks(base::xvblock_t* latest_cert_block, base::xvblock_t* lock_block, base::xvblock_t* commited_block) {
    // check committed anc connectted flag first
    if (!commited_block->check_block_flag(base::enum_xvblock_flag_connected)) {
        xwarn("xblocktool_t::verify_latest_blocks, fail-committed not connected. commit_block=%s",
            commited_block->dump().c_str());
        return false;
    }
    // cert height, lock height, commit height is 0
    if (latest_cert_block->get_height() == 0) {
        if (latest_cert_block->check_block_flag(base::enum_xvblock_flag_committed)
            && lock_block->check_block_flag(base::enum_xvblock_flag_committed)) {
            return true;
        }
        xassert(false);
        return false;
    }

    // cert height is not 0, cert block should not be committed or locked
    if (latest_cert_block->check_block_flag(base::enum_xvblock_flag_committed)
        || latest_cert_block->check_block_flag(base::enum_xvblock_flag_locked)) {
        xwarn("xblocktool_t::verify_latest_blocks fail-cert block is invalid. %s", latest_cert_block->dump().c_str());
        return false;
    }

    // cert block should match lock block
    if (latest_cert_block->get_height() != lock_block->get_height() + 1
        || latest_cert_block->get_last_block_hash() != lock_block->get_block_hash()) {
        xwarn("xblocktool_t::verify_latest_blocks fail-cert block not match lock. cert=%s,lock=%s",
            latest_cert_block->dump().c_str(), lock_block->dump().c_str());
        return false;
    }

    // lock block flags check
    if (lock_block->get_height() == 0) {
        if (lock_block->check_block_flag(base::enum_xvblock_flag_committed)) {
            return true;
        }
        xassert(false);
        return false;
    }

    // lock height is not 0, cert block should be locked
    if (lock_block->check_block_flag(base::enum_xvblock_flag_committed)
        || !lock_block->check_block_flag(base::enum_xvblock_flag_locked)) {
        xwarn("xblocktool_t::verify_latest_blocks fail-lock block is committed. %s", lock_block->dump().c_str());
        return false;
    }

    // lock block should match commit block
    if (lock_block->get_height() != commited_block->get_height() + 1
        || lock_block->get_last_block_hash() != commited_block->get_block_hash()) {
        xwarn("xblocktool_t::verify_latest_blocks fail-lock block not match commit. lock=%s,commit=%s",
            lock_block->dump().c_str(), commited_block->dump().c_str());
        return false;
    }
    return true;
}

bool xblocktool_t::can_make_next_empty_block(const base::xblock_mptrs & latest_blocks, uint32_t max_empty_num) {
    uint64_t latest_cert_block_height = latest_blocks.get_latest_cert_block()->get_height();
    if (latest_cert_block_height == 0) {
        return false;  // height #1 should not be empty block
    }

    uint32_t empty_block_num = 0;
    // check if cert block is empty block
    if (latest_blocks.get_latest_cert_block()->get_block_class() == base::enum_xvblock_class_nil) {
        empty_block_num += 1;
        // check if locked block is empty block
        if (latest_blocks.get_latest_locked_block()->get_height() + 1 == latest_blocks.get_latest_cert_block()->get_height()
            && latest_blocks.get_latest_locked_block()->get_block_class() == base::enum_xvblock_class_nil) {
            empty_block_num += 1;
            // check if committed block is empty block
            if (latest_blocks.get_latest_committed_block()->get_height() + 1 == latest_blocks.get_latest_locked_block()->get_height()
                && latest_blocks.get_latest_committed_block()->get_block_class() == base::enum_xvblock_class_nil) {
                empty_block_num += 1;
            }
        }
    }

    if (empty_block_num >= max_empty_num) {
        return false;
    }
    return true;
}

bool xblocktool_t::can_make_next_full_table(base::xvblock_t* latest_cert_block, uint32_t max_light_num) {
    uint64_t last_full_block_height = latest_cert_block->get_last_full_block_height();
    if ( (latest_cert_block->get_block_class() != base::enum_xvblock_class_full)
        && (latest_cert_block->get_height() - last_full_block_height >= max_light_num) ) {
        return true;
    }
    return false;
}

void xblocktool_t::alloc_transaction_receiptid(const xcons_transaction_ptr_t & tx, const base::xreceiptid_state_ptr_t & receiptid_state) {
    if (tx->is_self_tx()) {
        return;  // self tx has none receiptid
    } else if (tx->is_send_tx()) {
        base::xvaccount_t _vaccount(tx->get_transaction()->get_target_addr());
        base::xtable_shortid_t target_sid = _vaccount.get_short_table_id();

        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair_modified(target_sid, receiptid_pair);

        uint64_t current_receipt_id = receiptid_pair.get_sendid_max() + 1;
        receiptid_pair.inc_sendid_max();
        tx->set_current_receipt_id(target_sid, current_receipt_id);
        receiptid_state->add_pair_modified(target_sid, receiptid_pair);  // save to modified pairs
    } else if ( tx->is_recv_tx()) {
        base::xvaccount_t _vaccount(tx->get_transaction()->get_source_addr());
        base::xtable_shortid_t source_sid = _vaccount.get_short_table_id();
        // copy receipt id from last phase to current phase
        uint64_t receipt_id = tx->get_last_action_receipt_id();
        tx->set_current_receipt_id(source_sid, receipt_id);
    } else if (tx->is_confirm_tx() ) {
        base::xvaccount_t _vaccount(tx->get_transaction()->get_target_addr());
        base::xtable_shortid_t target_sid = _vaccount.get_short_table_id();
        // copy receipt id from last phase to current phase
        uint64_t receipt_id = tx->get_last_action_receipt_id();
        tx->set_current_receipt_id(target_sid, receipt_id);
    }
}

NS_END2
