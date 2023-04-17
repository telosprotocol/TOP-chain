// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xblockmaker/xblock_builder.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xchain_fork/xutility.h"
#include "xdata/xblockbuild.h"
#include "xvledger/xvblock_offdata.h"

NS_BEG2(top, blockmaker)

void xunitbuildber_txkeys_mgr_t::add_txkey(const std::string & address, const base::xvtxkey_t & txkey) {    
    auto iter = m_account_txkeys.find(address);
    if (iter == m_account_txkeys.end()) {
        base::xvtxkey_vec_t txkeys;
        txkeys.push_back(txkey);
        m_account_txkeys[address] = txkeys;
    } else {
        iter->second.push_back(txkey);
    }
}

void xunitbuildber_txkeys_mgr_t::add_pack_tx(const data::xcons_transaction_ptr_t & tx) {
    {
        const std::string & address = tx->get_account_addr();
        base::xvtxkey_t txkey(tx->get_tx_hash(), tx->get_tx_subtype());
        add_txkey(address, txkey);
    }

    if (tx->is_send_tx() && tx->get_inner_table_flag()) { // TODO(jimmy) add recvtx key to recvaddress
        const std::string & address = tx->get_target_addr();
        base::xvtxkey_t txkey(tx->get_tx_hash(), base::enum_transaction_subtype_recv);
        add_txkey(address, txkey);
    }
}

base::xvtxkey_vec_t xunitbuildber_txkeys_mgr_t::get_account_txkeys(const std::string & address) {
    auto iter = m_account_txkeys.find(address);
    if (iter != m_account_txkeys.end()) {
        return iter->second;
    } else {
        return {};
    }
}

bool xunitbuilder_t::can_make_full_unit_v2(uint64_t proposal_height) {
    // XTODO not depend on prev blcok
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    if (proposal_height % max_limit_lightunit_count == 0) {
        return true;
    }
    return false;
}

base::xvblock_ptr_t  xunitbuilder_t::create_unit(std::string const& account, uint64_t height, std::string const& last_block_hash, const data::xunit_block_para_t & bodypara, const data::xblock_consensus_para_t & cs_para) {
    bool is_full_unit = xunitbuilder_t::can_make_full_unit_v2(height);
    xassert(!last_block_hash.empty());
    std::shared_ptr<base::xvblockmaker_t> vblockmaker = std::make_shared<data::xunit_build2_t>(
        account, height, last_block_hash, is_full_unit, cs_para);    
    base::xvblock_ptr_t proposal_block = vblockmaker->build_new_block();
    if (proposal_block->get_cert()->is_consensus_flag_has_extend_cert()) {
        proposal_block->get_cert()->set_parent_viewid(cs_para.get_viewid());
        proposal_block->set_extend_cert("1");
        proposal_block->set_extend_data("1");
    }
    proposal_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    return proposal_block;
}

void xunitbuilder_t::make_unitblock_and_unitstate(data::xaccountstate_ptr_t const& accountstate, const data::xblock_consensus_para_t & cs_para, xunit_build_result_t & result) {
    auto & unitstate = accountstate->get_unitstate();
    assert(unitstate->is_state_changed());
    assert(!unitstate->get_bstate()->get_last_block_hash().empty());

    // create unit header firstly for update unitstate information
    bool is_full_unit = xunitbuilder_t::can_make_full_unit_v2(unitstate->height());
    std::shared_ptr<data::xunit_build2_t> vblockmaker = std::make_shared<data::xunit_build2_t>(
        unitstate->get_bstate()->get_account(), unitstate->height(), unitstate->get_bstate()->get_last_block_hash(), is_full_unit, cs_para);    

    // update final unitstate information
    unitstate->get_bstate()->update_final_block_info(vblockmaker->get_header(), cs_para.get_viewid());

    // make bodypara and create unitblock body
    data::xunit_block_para_t bodypara;
    std::string binlog = unitstate->take_binlog();
    bodypara.set_binlog(binlog);
    assert(!binlog.empty());
  
    base::enum_xaccountindex_version_t _accountindex_version;
    std::string snapshot;    
    if (false == chain_fork::xutility_t::is_forked(fork_points::v11200_block_fork_point, cs_para.get_clock())) {
        snapshot = unitstate->take_snapshot();
        _accountindex_version = base::enum_xaccountindex_version_snapshot_hash;   
    } else {
        unitstate->get_bstate()->serialize_to_string(snapshot);
        result.unitstate_bin = snapshot;// before fork, it is empty    
        _accountindex_version = base::enum_xaccountindex_version_state_hash;
    }
    assert(!snapshot.empty());
    std::string snapshot_hash = vblockmaker->get_qcert()->hash(snapshot);
    bodypara.set_fullstate_bin(snapshot);
    bodypara.set_fullstate_bin_hash(snapshot_hash);

    vblockmaker->create_block_body(bodypara);
    base::xvblock_ptr_t proposal_block = vblockmaker->build_new_block();
    if (proposal_block->get_cert()->is_consensus_flag_has_extend_cert()) {
        proposal_block->get_cert()->set_parent_viewid(cs_para.get_viewid());
        proposal_block->set_extend_cert("1");
        proposal_block->set_extend_data("1");
    }
    proposal_block->set_block_flag(base::enum_xvblock_flag_authenticated);    

    result.unitblock = proposal_block;
    result.unitstate = unitstate;
    result.accountindex = base::xaccount_index_t(_accountindex_version, proposal_block->get_height(), proposal_block->get_block_hash(), bodypara.get_fullstate_bin_hash(), accountstate->get_tx_nonce());
    accountstate->update_account_index(result.accountindex);
    xdbg("xunitbuilder_t::make_unitblock_and_unitstate cs_para=%s,%s,unitstate=%s,unit=%s",cs_para.dump().c_str(), result.accountindex.dump().c_str(),result.unitstate->get_bstate()->dump().c_str(), result.unitblock->dump().c_str());
}

void xtablebuilder_t::make_table_prove_property_hashs(base::xvbstate_t* bstate, std::map<std::string, std::string> & property_hashs) {
    std::string property_receiptid_bin = data::xtable_bstate_t::get_receiptid_property_bin(bstate);
    if (!property_receiptid_bin.empty()) {
        uint256_t hash = utl::xsha2_256_t::digest(property_receiptid_bin);
        XMETRICS_GAUGE(metrics::cpu_hash_256_receiptid_bin_calc, 1);
        std::string prophash = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
        property_hashs[data::xtable_bstate_t::get_receiptid_property_name()] = prophash;
    }
}

bool     xtablebuilder_t::update_receipt_confirmids(const data::xtablestate_ptr_t & tablestate, 
                                            const std::map<base::xtable_shortid_t, uint64_t> & changed_confirm_ids) {
    for (auto & confirmid_pair : changed_confirm_ids) {
        xdbg("xtablebuilder_t::update_receipt_confirmids set confirmid,self:%d,peer:%d,receiptid:%llu,proposal height:%llu",
             tablestate->get_receiptid_state()->get_self_tableid(),
             confirmid_pair.first,
             confirmid_pair.second,
             tablestate->height());
        base::xreceiptid_pair_t receiptid_pair;
        tablestate->find_receiptid_pair(confirmid_pair.first, receiptid_pair);
        if (confirmid_pair.second > receiptid_pair.get_confirmid_max() && confirmid_pair.second <= receiptid_pair.get_sendid_max()) {
            receiptid_pair.set_confirmid_max(confirmid_pair.second);
            tablestate->set_receiptid_pair(confirmid_pair.first, receiptid_pair);  // save to modified pairs
        } else {
            xerror("xtablebuilder_t::update_receipt_confirmids set confirmid,self:%d,peer:%d,receiptid:%llu,proposal height:%llu,receiptid_pair=%s",
                   tablestate->get_receiptid_state()->get_self_tableid(),
                   confirmid_pair.first,
                   confirmid_pair.second,
                   tablestate->height(),
                   receiptid_pair.dump().c_str());
            return false;
        }
    }
    return true;
}

void     xtablebuilder_t::make_table_block_para(const data::xtablestate_ptr_t & tablestate,
                                                txexecutor::xexecute_output_t const& execute_output, 
                                                data::xtable_block_para_t & lighttable_para) {
    int64_t tgas_balance_change = 0;
    std::vector<data::xlightunit_tx_info_ptr_t> txs_info;
    std::map<std::string, std::string> property_hashs;

    // change to xvaction and calc tgas balance change
    for (auto & txout : execute_output.pack_outputs) {
        txs_info.push_back(data::xblockaction_build_t::build_tx_info(txout.m_tx));
        tgas_balance_change += txout.m_vm_output.m_tgas_balance_change;
        for (auto & v : txout.m_vm_output.m_contract_create_txs) {
            txs_info.push_back(data::xblockaction_build_t::build_tx_info(v));
        }
    }

    // make receiptid property hashs
    xtablebuilder_t::make_table_prove_property_hashs(tablestate->get_bstate().get(), property_hashs);

    std::string binlog = tablestate->take_binlog();
    std::string snapshot = tablestate->take_snapshot();
    if (binlog.empty() || snapshot.empty()) {
        xassert(false);
    }

    lighttable_para.set_property_binlog(binlog);
    lighttable_para.set_fullstate_bin(snapshot);
    lighttable_para.set_tgas_balance_change(tgas_balance_change);
    lighttable_para.set_property_hashs(property_hashs);
    lighttable_para.set_txs(txs_info);
}

data::xblock_ptr_t  xtablebuilder_t::make_light_block(const data::xblock_ptr_t & prev_block, const data::xblock_consensus_para_t & cs_para, data::xtable_block_para_t const& lighttable_para) {
    std::shared_ptr<base::xvblockmaker_t> vbmaker = std::make_shared<data::xtable_build2_t>(prev_block.get(), lighttable_para, cs_para);
    auto _new_block = vbmaker->build_new_block();
    data::xblock_ptr_t proposal_block = data::xblock_t::raw_vblock_to_object_ptr(_new_block.get());

    return proposal_block;
}

NS_END2
