// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xblockbuild.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstate.h"

NS_BEG2(top, data)

base::xvblock_t* xblocktool_t::create_genesis_empty_block(const std::string & account) {
    xemptyblock_build_t bbuild(account);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
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
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(account, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
    {
        auto propobj = bstate->new_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME, canvas.get());
        propobj->set(base::TOP_BEGIN_GMTIME, canvas.get());
    }
    if (top_balance > 0) {
        auto propobj = bstate->new_token_var(XPROPERTY_BALANCE_AVAILABLE, canvas.get());
        auto balance = propobj->deposit(base::vtoken_t(top_balance), canvas.get());
        xassert(balance == top_balance);
    }

    std::string property_binlog;
    canvas->encode(property_binlog);
    std::string fullstate_bin;
    bstate->take_snapshot(fullstate_bin);
    // TODO(jimmy) block builder class
    xinfo("xlightunit_builder_t::build_block account=%s,height=0,binlog_size=%zu",
        account.c_str(), property_binlog.size());

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    xlightunit_block_para_t bodypara;
    bodypara.set_one_input_tx(cons_tx);
    bodypara.set_binlog(property_binlog);
    bodypara.set_fullstate_bin(fullstate_bin);
    xlightunit_build_t bbuild(account, bodypara);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t * xblocktool_t::create_genesis_lightunit(const std::string & account,
                                                         const xtransaction_ptr_t & genesis_tx,
                                                         const xtransaction_result_t & result) {
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(genesis_tx.get());
    xlightunit_block_para_t bodypara;
    bodypara.set_one_input_tx(cons_tx);
    bodypara.set_binlog(result.get_property_binlog());
    bodypara.set_fullstate_bin(result.m_full_state);
    xlightunit_build_t bbuild(account, bodypara);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t* xblocktool_t::create_genesis_root_block(base::enum_xchain_id chainid, const std::string & account, const xrootblock_para_t & bodypara) {
    xrootblock_build_t bbuild(chainid, account, bodypara);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t*  xblocktool_t::create_next_emptyblock(base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    xemptyblock_build_t bbuild(prev_block, cs_para);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t*  xblocktool_t::create_next_emptyblock(base::xvblock_t* prev_block) {
    xemptyblock_build_t bbuild(prev_block);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t* xblocktool_t::create_next_lightunit(const xlightunit_block_para_t & bodypara, base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    xlightunit_build_t bbuild(prev_block, bodypara, cs_para);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t*  xblocktool_t::create_next_fullunit(const xfullunit_block_para_t & bodypara, base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    xfullunit_build_t bbuild(prev_block, bodypara, cs_para);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t*  xblocktool_t::create_next_tableblock(const xtable_block_para_t & bodypara, base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    xlighttable_build_t bbuild(prev_block, bodypara, cs_para);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

base::xvblock_t*   xblocktool_t::create_next_fulltable(const xfulltable_block_para_t & bodypara, base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    xfulltable_build_t bbuild(prev_block, bodypara, cs_para);
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->add_ref();
    return _new_block.get();  // TODO(jimmy) xblocktool_t return auto ptr
}

std::string xblocktool_t::make_address_table_account(base::enum_xchain_zone_index zone, uint16_t subaddr) {
    return base::xvaccount_t::make_table_account_address(zone, subaddr);
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
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(_vaccount, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xblocktool_t::get_latest_connectted_light_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account) {
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_connected_block(account);
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }
    if (vblock->get_block_class() == base::enum_xvblock_class_full) {
        return nullptr;
    }
    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(account, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        if (prev_vblock->get_block_class() == base::enum_xvblock_class_full) {
            return nullptr;
        }
        current_height = prev_vblock->get_height();
    }
    return nullptr;
}


base::xauto_ptr<base::xvblock_t> xblocktool_t::get_latest_genesis_connectted_lightunit(base::xvblockstore_t* blockstore, const std::string & account) {
    base::xvaccount_t _vaccount(account);
    // there is mostly two empty units
    XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_genesis_connected_block(_vaccount);
    if (vblock == nullptr) {
        return nullptr;
    }
    xinfo("xblocktool_t::get_latest_genesis_connectted_lightunit addr:%s,height=%llu", account.c_str(), vblock->get_height());
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(_vaccount, current_height - 1, base::enum_xvblock_flag_committed, false);
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
    XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->load_block_object(_vaccount, max_height, base::enum_xvblock_flag_committed, false);
    xassert(vblock->check_block_flag(base::enum_xvblock_flag_committed));
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(_vaccount, current_height - 1, base::enum_xvblock_flag_committed, false);
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
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_verify_latest_blocks");
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
    }

    base::xtable_shortid_t self_tableid = tx->get_self_tableid();
    base::xtable_shortid_t peer_tableid = tx->get_peer_tableid();
    base::xreceiptid_pair_t receiptid_pair;
    receiptid_state->find_pair_modified(peer_tableid, receiptid_pair);
    uint64_t current_receipt_id = 0;
    if (tx->is_send_tx()) {
        // alloc new receipt id for send tx
        current_receipt_id = receiptid_pair.get_sendid_max() + 1;
        receiptid_pair.inc_sendid_max();
    } else {
        // copy last actiion receipt id for recv tx and confirm tx
        current_receipt_id = tx->get_last_action_receipt_id();
    }
    // update receipt id info to tx action result
    tx->set_current_receipt_id(self_tableid, peer_tableid, current_receipt_id);
    receiptid_state->add_pair_modified(peer_tableid, receiptid_pair);  // save to modified pairs
    xdbg("xblocktool_t::alloc_transaction_receiptid tx=%s,receiptid=%ld", tx->dump().c_str(), current_receipt_id);
}

NS_END2
