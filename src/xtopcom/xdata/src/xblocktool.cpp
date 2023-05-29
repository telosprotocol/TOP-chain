// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xblockbuild.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtx_factory.h"
#include "xdata/xblockextract.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvblockbuild.h"
#include "xmbus/xevent_behind.h"
#include "xstatestore/xstatestore_face.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

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
    xtransaction_ptr_t tx = xtx_factory::create_genesis_tx_with_balance(account, top_balance);

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

base::xvblock_t * xblocktool_t::create_genesis_lightunit(const xobject_ptr_t<base::xvbstate_t> & state, const xobject_ptr_t<base::xvcanvas_t> & canvas) {
    auto account = state->get_account();
    auto tx = xtx_factory::create_genesis_tx_with_balance(account, 0);

    std::string property_binlog;
    std::string fullstate_bin;
    canvas->encode(property_binlog);
    state->take_snapshot(fullstate_bin);
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

base::xvblock_t * xblocktool_t::create_genesis_lightunit(std::string const & account, chain_data::data_processor_t const & data) {
    xtransaction_ptr_t tx = xtx_factory::create_genesis_tx_with_balance(account, data.top_balance);

    xtransaction_result_t result;
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(account, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
    auto propobj = bstate->new_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME, canvas.get());
    if (data.create_time > 0) {
        propobj->set(data.create_time, canvas.get());
    } else {
        propobj->set(base::TOP_BEGIN_GMTIME, canvas.get());
    }
    if (data.top_balance + data.lock_token + data.lock_balance > 0) {
        auto propobj = bstate->new_token_var(XPROPERTY_BALANCE_AVAILABLE, canvas.get());
        auto balance = propobj->deposit(base::vtoken_t(data.top_balance + data.lock_token + data.lock_balance), canvas.get());
        xassert(balance == data.top_balance + data.lock_token + data.lock_balance);
    }
    if (data.burn_balance > 0) {
        auto propobj = bstate->new_token_var(XPROPERTY_BALANCE_BURN, canvas.get());
        auto balance = propobj->deposit(base::vtoken_t(data.burn_balance), canvas.get());
        xassert(balance == data.burn_balance);
    }
    if (data.tgas_balance > 0) {
        auto propobj = bstate->new_token_var(XPROPERTY_BALANCE_PLEDGE_TGAS, canvas.get());
        auto balance = propobj->deposit(base::vtoken_t(data.tgas_balance), canvas.get());
        xassert(balance == data.tgas_balance);
    }
    if (data.vote_balance > 0) {
        auto propobj = bstate->new_token_var(XPROPERTY_BALANCE_PLEDGE_VOTE, canvas.get());
        auto balance = propobj->deposit(base::vtoken_t(data.vote_balance), canvas.get());
        xassert(balance == data.vote_balance);
    }
    if (data.lock_tgas > 0) {
        auto propobj = bstate->new_uint64_var(XPROPERTY_LOCK_TGAS, canvas.get());
        auto res = propobj->set(base::vtoken_t(data.lock_tgas), canvas.get());
        xassert(res == true);
    }
    if (data.unvote_num > 0) {
        auto propobj = bstate->new_uint64_var(XPROPERTY_UNVOTE_NUM, canvas.get());
        auto res = propobj->set(base::vtoken_t(data.unvote_num), canvas.get());
        xassert(res == true);
    }
    if (data.expire_vote > 0) {
        auto propobj = bstate->new_string_var(XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, canvas.get());
        auto res = propobj->reset(base::xstring_utl::tostring(data.expire_vote), canvas.get());
        xassert(res == true);
    }
    if (data.pledge_vote.size() > 0) {
        auto propobj = bstate->new_string_map_var(XPROPERTY_PLEDGE_VOTE_KEY, canvas.get());
        for (auto const & item : data.pledge_vote) {
            uint64_t vote_num = 0;
            uint16_t duration = 0;
            uint64_t lock_time = 0;
            base::xstream_t out{base::xcontext_t::instance(), (uint8_t *)item.data(), static_cast<uint32_t>(item.size())};
            out >> vote_num;
            out >> duration;
            out >> lock_time;
            base::xstream_t in{base::xcontext_t::instance()};
            in << duration;
            in << lock_time;
            auto vote_key_str = std::string((char*)in.data(), in.size());
            in.reset();
            in << vote_num;
            auto vote_val_str =  std::string((char*)in.data(), in.size());
            auto res = propobj->insert(vote_key_str, vote_val_str, canvas.get());
            xassert(res == true);
        }
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
//std::string xblocktool_t::make_address_user_contract(const std::string & public_key_address) {
//    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
//    return base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_custom_contract, ledger_id, public_key_address);
//}

std::vector<std::string> xblocktool_t::make_all_table_addresses() {
    std::vector<std::string> table_addrs;
    const std::vector<std::pair<base::enum_xchain_zone_index, uint16_t>> table = {
        std::make_pair(base::enum_chain_zone_consensus_index, enum_vledger_const::enum_vbucket_has_tables_count),
        std::make_pair(base::enum_chain_zone_zec_index, MAIN_CHAIN_ZEC_TABLE_USED_NUM),
        std::make_pair(base::enum_chain_zone_beacon_index, MAIN_CHAIN_REC_TABLE_USED_NUM),
        std::make_pair(base::enum_chain_zone_evm_index, MAIN_CHAIN_EVM_TABLE_USED_NUM),
        std::make_pair(base::enum_chain_zone_relay_index, MAIN_CHAIN_RELAY_TABLE_USED_NUM),
    };
    for (auto const & t : table) {
        for (auto i = 0; i < t.second; i++) {
            std::string addr = base::xvaccount_t::make_table_account_address(t.first, i);
            table_addrs.push_back(addr);
        }
    }
    return table_addrs;
}

bool xblocktool_t::verify_latest_blocks(const base::xblock_mptrs & latest_blocks) {
    return verify_latest_blocks(latest_blocks.get_latest_cert_block(), latest_blocks.get_latest_locked_block(), latest_blocks.get_latest_committed_block());
}

bool xblocktool_t::verify_latest_blocks(base::xvblock_t* latest_cert_block, base::xvblock_t* lock_block, base::xvblock_t* commited_block) {
    // check committed anc connectted flag first
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

bool xblocktool_t::alloc_transaction_receiptid(const xcons_transaction_ptr_t & tx, base::xreceiptid_pair_t & receiptid_pair) {
    if (tx->is_self_tx() || tx->get_inner_table_flag()) {
        xassert(false);
        return false;
    }

    base::xtable_shortid_t self_tableid = tx->get_self_tableid();
    base::xtable_shortid_t peer_tableid = tx->get_peer_tableid();

    uint64_t current_receipt_id = 0;
    uint64_t current_rsp_id = 0;
    if (tx->is_send_tx()) {
        // alloc new receipt id for send tx
        current_receipt_id = receiptid_pair.get_sendid_max() + 1;
        receiptid_pair.set_sendid_max(current_receipt_id);
        
        if (!tx->get_not_need_confirm()) {
            current_rsp_id = receiptid_pair.get_send_rsp_id_max() + 1;
            receiptid_pair.set_send_rsp_id_max(current_rsp_id);
        }
    } else if (tx->is_recv_tx()) {
        // copy last actiion receipt id for recv tx and confirm tx
        current_receipt_id = tx->get_last_action_receipt_id();
        if (current_receipt_id != receiptid_pair.get_recvid_max() + 1) {
            xassert(false);
            return false;
        }
        receiptid_pair.set_recvid_max(current_receipt_id);
        current_rsp_id = tx->get_last_action_rsp_id();
    } else if (tx->is_confirm_tx()) {
        current_receipt_id = tx->get_last_action_receipt_id();
        current_rsp_id = tx->get_last_action_rsp_id();
        if (current_rsp_id == 0) {  // not enable rsp id, confirmid must contious increase
            if ((current_receipt_id != receiptid_pair.get_confirmid_max() + 1)
                || (receiptid_pair.get_confirm_rsp_id_max() != 0) ) {
                xassert(false);
                return false;
            }
        } else {  // enable rsp id, rsp id must contious increase
            if ( (current_rsp_id != receiptid_pair.get_confirm_rsp_id_max() + 1)
                || (current_receipt_id <= receiptid_pair.get_confirmid_max())) {
                xassert(false);
                return false;
            }
        }
        receiptid_pair.set_confirmid_max(current_receipt_id);
        receiptid_pair.set_confirm_rsp_id_max(current_rsp_id);
    }
    // update receipt id info to tx action result
    tx->set_current_receipt_id(self_tableid, peer_tableid, current_receipt_id);
    if (current_rsp_id != 0) {
        tx->set_current_rsp_id(current_rsp_id);
    }
    if (tx->is_send_tx()) {
        tx->set_current_sender_confirmed_receipt_id(receiptid_pair.get_confirmid_max());
    }
    xdbg("xblocktool_t::alloc_transaction_receiptid tx=%s,receipt=%s", tx->dump().c_str(), receiptid_pair.dump().c_str());
    return true;
}

// =============== action & txreceipt tool =============
std::vector<xlightunit_action_t> xblocktool_t::unpack_all_txreceipt_action(base::xvblock_t* commit_block) {
    if (commit_block == nullptr) {
        xassert(false);
        return {};
    }
    std::vector<xlightunit_action_t> txreceipt_actions;

    auto f = [&txreceipt_actions](const base::xvaction_t & action) { 
        xlightunit_action_t txaction(action);
        if (txaction.is_need_make_txreceipt()) {
            txreceipt_actions.push_back(txaction);
        }
    };
    xblockextract_t::loop_top_txactions(commit_block, f);
    xdbg("xblocktool_t::unpack_all_txreceipt_action block=%s,count=%zu",commit_block->dump().c_str(),txreceipt_actions.size());
    return txreceipt_actions;
}

std::vector<xlightunit_action_t> xblocktool_t::unpack_one_txreceipt_action(base::xvblock_t* commit_block, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, enum_transaction_subtype subtype) {
    if (commit_block == nullptr) {
        xassert(false);
        return {};
    }
    std::vector<xlightunit_action_t> txreceipt_actions;
    auto f = [&txreceipt_actions,&peer_table_sid,&receipt_id,&subtype](const base::xvaction_t & action) { 
        enum_transaction_subtype _actionid = xlightunit_action_t::get_txaction_subtype(action);
        if (_actionid == subtype) {
            xlightunit_action_t txaction(action);
            xinfo("xblocktool_t::create_one_txreceipt peer tableid:%d:%d, receiptid:%llu:%llu", peer_table_sid, txaction.get_receipt_id_peer_tableid(), receipt_id, txaction.get_receipt_id());
            if (txaction.get_receipt_id_peer_tableid() == peer_table_sid && txaction.get_receipt_id() == receipt_id) {
                if (txaction.is_need_make_txreceipt()) {
                    txreceipt_actions.push_back(txaction);         
                } else {
                    xassert(false);
                }          
            }
        }
    };
    xblockextract_t::loop_top_txactions(commit_block, f);
    return txreceipt_actions;
}


std::vector<xcons_transaction_ptr_t> xblocktool_t::create_txreceipts(base::xvblock_t* commit_block, base::xvblock_t* cert_block, const std::vector<xlightunit_action_t> & txactions) {
    if (commit_block == nullptr || cert_block == nullptr || txactions.empty()) {
        xassert(false);
        return {};
    }
    if ( (cert_block->get_justify_cert_hash() != commit_block->get_input_root_hash())
        || (commit_block->get_height() + 2 != cert_block->get_height())
        || (commit_block->get_account() != cert_block->get_account())
        || (commit_block->get_block_level() != base::enum_xvblock_level_table) ) {  // txreceipts make from table block
        xassert(false);
        return {};
    }
    // get all leafs firstly for performance
    std::error_code ec;
    auto input_object = commit_block->load_input(ec);    
    std::vector<std::string> all_leafs = base::xvblockmaker_t::get_input_merkle_leafs(input_object);

    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle(all_leafs);
    // #3 calc leaf path and make rceipt
    std::vector<xcons_transaction_ptr_t> receipt_txs;
    for (auto & action : txactions) {
        base::xmerkle_path_256_t hash_path;
        if (false == base::xvblockmaker_t::calc_merkle_path(action, hash_path, merkle)) {
            xassert(false);
            return {};
        }
        std::string path_bin = hash_path.serialize_to_string();
        base::enum_xprove_cert_type prove_type = base::enum_xprove_cert_type_table_justify;
        // xtx_receipt_t* _receipt = new xtx_receipt_t(action, cert_block->get_cert(), path_bin, prove_type);
        base::xtx_receipt_ptr_t _receipt_ptr = make_object_ptr<base::xtx_receipt_t>(action, cert_block->get_cert(), path_bin, prove_type);
        // _receipt_ptr.attach(_receipt);
        std::string orgtx_bin;
        if (action.is_send_tx()) {
            orgtx_bin = commit_block->query_input_resource(action.get_org_tx_hash());// only sendtx has origin raw tx bin
            xassert(!orgtx_bin.empty());
        }
        base::xfull_txreceipt_t full_txreceipt(_receipt_ptr, orgtx_bin);
        xcons_transaction_ptr_t constx = make_object_ptr<data::xcons_transaction_t>(full_txreceipt);
        receipt_txs.push_back(constx);
    }
    xdbg("xblocktool_t::create_all_txreceipts,block=%s,receipts=%zu,allleafs=%zu,",
        commit_block->dump().c_str(), receipt_txs.size(), all_leafs.size());
    return receipt_txs;
}

xcons_transaction_ptr_t xblocktool_t::create_one_txreceipt(base::xvblock_t* commit_block, base::xvblock_t* cert_block, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, enum_transaction_subtype subtype) {
    if (commit_block == nullptr || cert_block == nullptr) {
        xassert(false);
        return nullptr;
    }
    if (subtype != enum_transaction_subtype_send && subtype != enum_transaction_subtype_recv) {
        xassert(false);
        return nullptr;
    }

    std::vector<xlightunit_action_t> txreceipt_actions = xblocktool_t::unpack_one_txreceipt_action(commit_block, peer_table_sid, receipt_id, subtype);
    if (txreceipt_actions.size() != 1) {  // not find txhash
        xerror("xblocktool_t::create_one_txreceipt not find.block=%s,tableid:%d receiptid:%llu,subtype=%d", commit_block->dump().c_str(), peer_table_sid, receipt_id, subtype);
        return nullptr;
    }

    std::vector<xcons_transaction_ptr_t> txreceipts = create_txreceipts(commit_block, cert_block, txreceipt_actions);
    if (txreceipts.size() != 1) {  // not find txhash
        xassert(false);
        return nullptr;
    }
    return txreceipts[0];
}

std::vector<xcons_transaction_ptr_t> xblocktool_t::create_all_txreceipts(base::xvblock_t* commit_block, base::xvblock_t* cert_block) {
    std::vector<xlightunit_action_t> txreceipt_actions = xblocktool_t::unpack_all_txreceipt_action(commit_block);
    if (txreceipt_actions.empty()) {
        return {};
    }

    return create_txreceipts(commit_block, cert_block, txreceipt_actions);
}

base::xvproperty_prove_ptr_t xblocktool_t::create_receiptid_property_prove(base::xvblock_t* commit_block, base::xvblock_t* cert_block, base::xvbstate_t* bstate) {    
    return base::xpropertyprove_build_t::create_property_prove(commit_block, cert_block, bstate, data::xtable_bstate_t::get_receiptid_property_name());
}

base::xreceiptid_state_ptr_t xblocktool_t::get_receiptid_from_property_prove(const base::xvproperty_prove_ptr_t & prop_prove) {
    xtableblock_action_t _tb_action(prop_prove->get_action_prove()->get_action());
    base::xtable_shortid_t self_tableid = _tb_action.get_self_tableid();
    uint64_t self_height = _tb_action.get_self_table_height();
    auto & property = prop_prove->get_property();

    std::string address = base::xvaccount_t::make_table_account_address(self_tableid);
    base::xauto_ptr<base::xvproperty_holder_t> _temp_bstate = new base::xvproperty_holder_t(address, self_height);
    _temp_bstate->add_property(property.get());  // should always operate property by bstate

    base::xreceiptid_state_ptr_t receiptid = xtable_bstate_t::make_receiptid_from_state(_temp_bstate.get());
    return receiptid;
}

xrelay_block* xblocktool_t::create_genesis_relay_block(const xrootblock_para_t & bodypara)
{
    if(bodypara.m_genesis_nodes.size() < 1) {
        xwarn("xrelay_block::create_genesis_relay_block genesis node size is 0");
    }

    xrelay_block *_relay_block = new xrelay_block();
    xrelay_election_group_t _election_group;
    auto const max_relay_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_group_size);
    for(uint64_t index = 0; (index < max_relay_group_size) && (index < bodypara.m_genesis_nodes.size()) ; index++) {
        auto &node = bodypara.m_genesis_nodes[index];
        auto pubkey_str = base::xstring_utl::base64_decode(node.m_publickey.to_string());
        xbytes_t bytes_x(pubkey_str.begin() + 1, pubkey_str.begin() + 33);
        xbytes_t bytes_y(pubkey_str.begin() + 33, pubkey_str.end());
        _election_group.elections_vector.push_back(data::xrelay_election_node_t(evm_common::h256(bytes_x), evm_common::h256(bytes_y)));
    }

    _relay_block->set_elections_next(_election_group);
    _relay_block->build_finish();

    return _relay_block;
}

base::xvblock_t* xblocktool_t::create_genesis_wrap_relayblock() {
    auto _relay_block = data::xrootblock_t::get_genesis_relay_block();
    std::error_code ec;
    xobject_ptr_t<base::xvblock_t> wrap_relayblock = data::xblockextract_t::pack_relayblock_to_wrapblock(_relay_block, ec);
    if (ec) {
        xerror("");
        return nullptr;
    }
    
    wrap_relayblock->add_ref();
    return wrap_relayblock.get();
}

NS_END2
