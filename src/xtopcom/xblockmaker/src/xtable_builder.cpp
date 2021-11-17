// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xtable_builder.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xfulltable_statistics.h"
#include "xdata/xemptyblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xtable_bstate.h"

NS_BEG2(top, blockmaker)

void make_table_prove_property_hashs(base::xvbstate_t* bstate, std::map<std::string, std::string> & property_hashs) {
    std::string property_receiptid_bin = data::xtable_bstate_t::get_receiptid_property_bin(bstate);
    if (!property_receiptid_bin.empty()) {
        uint256_t hash = utl::xsha2_256_t::digest(property_receiptid_bin);
        XMETRICS_GAUGE(metrics::cpu_hash_256_receiptid_bin_calc, 1);
        std::string prophash = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
        property_hashs[data::xtable_bstate_t::get_receiptid_property_name()] = prophash;
    }
}

void xlighttable_builder_t::make_light_table_binlog(const xobject_ptr_t<base::xvbstate_t> & proposal_bstate,
                                                           const std::vector<xblock_ptr_t> & units,
                                                           std::string & property_binlog,
                                                           std::map<std::string, std::string> & property_hashs,
                                                           const std::vector<xlightunit_tx_info_ptr_t> & txs_info) {
    xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();

    data::xtable_bstate_t proposal_tbstate(proposal_bstate.get());

    // make account index property binlog
    for (auto & unit : units) {
        // read old index
        xaccount_index_t _old_aindex;
        proposal_tbstate.get_account_index(unit->get_account(), _old_aindex);
        // update unconfirm sendtx flag
        bool has_unconfirm_sendtx = _old_aindex.is_has_unconfirm_tx();
        if (unit->get_block_class() == base::enum_xvblock_class_full) {
            has_unconfirm_sendtx = false;
        } else if (unit->get_block_class() == base::enum_xvblock_class_light) {
            has_unconfirm_sendtx = unit->get_unconfirm_sendtx_num() != 0;
        }
        // update light-unit consensus flag, light-unit must push to committed status for receipt make
        base::enum_xblock_consensus_type _cs_type = _old_aindex.get_latest_unit_consensus_type();
        if (unit->get_block_class() == base::enum_xvblock_class_light) {
            _cs_type = base::enum_xblock_consensus_flag_authenticated;  // if light-unit, reset to authenticated
        } else {
            if (_cs_type == base::enum_xblock_consensus_flag_authenticated) {  // if other-unit, update type
                _cs_type = base::enum_xblock_consensus_flag_locked;
            } else if (_cs_type == base::enum_xblock_consensus_flag_locked) {
                _cs_type = base::enum_xblock_consensus_flag_committed;
            } else if (_cs_type == base::enum_xblock_consensus_flag_committed) {
                // do nothing
            }
        }

        xaccount_index_t _new_aindex(unit.get(), has_unconfirm_sendtx, _cs_type, false, 0);  // TODO(jimmy)
        proposal_tbstate.set_account_index(unit->get_account(), _new_aindex, canvas.get());
    }

    // make receiptid property binlog
    base::xreceiptid_check_t receiptid_check;
    xblock_t::txs_to_receiptids(txs_info, receiptid_check);

    base::xreceiptid_pairs_ptr_t modified_pairs = std::make_shared<base::xreceiptid_pairs_t>();

    const std::map<base::xtable_shortid_t, std::set<uint64_t>> & sendids = receiptid_check.get_sendids();
    for (auto & v : sendids) {
        base::xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        if (false == modified_pairs->find_pair(tableid, pair)) {  // find modified pairs firstly
            proposal_tbstate.find_receiptid_pair(tableid, pair);
        }
        pair.set_sendid_max(maxid);
        modified_pairs->add_pair(tableid, pair);  // save to modified pairs
    }
    const std::map<base::xtable_shortid_t, std::set<uint64_t>> & recvids = receiptid_check.get_recvids();
    for (auto & v : recvids) {
        base::xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        if (false == modified_pairs->find_pair(tableid, pair)) {  // find modified pairs firstly
            proposal_tbstate.find_receiptid_pair(tableid, pair);
        }
        pair.set_recvid_max(maxid);
        modified_pairs->add_pair(tableid, pair);  // save to modified pairs
    }
    const std::map<base::xtable_shortid_t, std::set<uint64_t>> & confirmids = receiptid_check.get_confirmids();
    for (auto & v : confirmids) {
        base::xtable_shortid_t tableid = v.first;
        const std::set<uint64_t> & ids = v.second;
        uint64_t maxid = *ids.rbegin();
        base::xreceiptid_pair_t pair;
        if (false == modified_pairs->find_pair(tableid, pair)) {  // find modified pairs firstly
            proposal_tbstate.find_receiptid_pair(tableid, pair);
        }
        pair.set_confirmid_max(maxid);
        modified_pairs->add_pair(tableid, pair);  // save to modified pairs
    }
    // make modified pairs to binlog
    const std::map<base::xtable_shortid_t, base::xreceiptid_pair_t> & all_pairs = modified_pairs->get_all_pairs();
    for (auto & v : all_pairs) {
        proposal_tbstate.set_receiptid_pair(v.first, v.second, canvas.get());
    }

    canvas->encode(property_binlog);
    xassert(!property_binlog.empty());

    make_table_prove_property_hashs(proposal_bstate.get(), property_hashs);

    xdbg("jimmy xlighttable_builder_t::make_light_table_binlog units_size=%zu,sendids=%zu,recvids=%zu,confirmids=%zu,all=%zu,binlog_size=%zu",
        units.size(), sendids.size(), recvids.size(), confirmids.size(), all_pairs.size(), property_binlog.size());
}

xblock_ptr_t        xlighttable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    XMETRICS_TIMER(metrics::cons_tablebuilder_lighttable_tick);
    const std::string & account = prev_block->get_account();
    std::shared_ptr<xlighttable_builder_para_t> lighttable_build_para = std::dynamic_pointer_cast<xlighttable_builder_para_t>(build_para);
    xassert(lighttable_build_para != nullptr);

    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block.get(), cs_para.get_clock());
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate.get());

    auto & txs_info = build_para->get_txs();
    std::map<std::string, std::string> property_hashs;  // need put in table self action for prove
    std::string property_binlog;
    make_light_table_binlog(proposal_bstate, lighttable_build_para->get_batch_units(), property_binlog, property_hashs, txs_info);
    xtable_block_para_t lighttable_para;
    lighttable_para.set_property_binlog(property_binlog);
    lighttable_para.set_batch_units(lighttable_build_para->get_batch_units());
    lighttable_para.set_extra_data(cs_para.get_extra_data());
    lighttable_para.set_tgas_balance_change(lighttable_build_para->get_tgas_balance_change());
    std::string fullstate_bin;
    proposal_bstate->take_snapshot(fullstate_bin);
    lighttable_para.set_fullstate_bin(fullstate_bin);
    lighttable_para.set_property_hashs(property_hashs);
    lighttable_para.set_txs(txs_info);

    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_tableblock(lighttable_para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);

    xdbg("xlighttable_builder_t::build_block %s,account=%s,height=%ld,binlog_size=%zu,binlog=%ld,state_size=%zu,tgas_balance_change=%lld",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        property_binlog.size(), base::xhash64_t::digest(property_binlog), fullstate_bin.size(), lighttable_build_para->get_tgas_balance_change());
    return proposal_table;
}

void xfulltable_builder_t::make_binlog(const base::xauto_ptr<base::xvheader_t> & _temp_header,
                                                const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                std::string & property_binlog,
                                                std::map<std::string, std::string> & property_hashs) {    
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate.get());

    std::string property_snapshot;
    auto canvas = proposal_bstate->rebase_change_to_snapshot();  // TODO(jimmy)
    canvas->encode(property_snapshot);
    property_binlog = property_snapshot;
    
    make_table_prove_property_hashs(proposal_bstate.get(), property_hashs);
}


xblock_ptr_t        xfulltable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    XMETRICS_TIMER(metrics::cons_tablebuilder_fulltable_tick);
    const std::string & account = prev_block->get_account();
    std::shared_ptr<xfulltable_builder_para_t> fulltable_build_para = std::dynamic_pointer_cast<xfulltable_builder_para_t>(build_para);
    xassert(fulltable_build_para != nullptr);

    auto & blocks = fulltable_build_para->get_blocks_from_last_full();
    xstatistics_data_t block_statistics = make_block_statistics(blocks);

    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block.get(), cs_para.get_clock());

    std::map<std::string, std::string> property_hashs;
    std::string property_binlog;
    make_binlog(_temp_header, prev_bstate, property_binlog, property_hashs);

    int64_t tgas_balance_change_total = 0;
    for(auto & block : blocks) {
        if (block->get_block_class() == base::enum_xvblock_class_light) {
            auto out_entity = block->get_output()->get_primary_entity();
            if (out_entity != nullptr) {
                int64_t tgas_balance_change = base::xstring_utl::toint64(out_entity->query_value(base::xvoutentity_t::key_name_tgas_pledge_change()));
                tgas_balance_change_total += tgas_balance_change;
                xdbg("tgas_balance_change_total=%lld, cur=%lld, account=%s", tgas_balance_change_total, tgas_balance_change, block->dump().c_str());
            }
        }
    }

    xfulltable_block_para_t fulltable_para(property_binlog, block_statistics, tgas_balance_change_total);
    fulltable_para.set_property_hashs(property_hashs);
    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_fulltable(fulltable_para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);

    xdbg("xfulltable_builder_t::build_block %s,account=%s,height=%ld,binlog_size=%zu,binlog=%ld",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        property_binlog.size(), base::xhash64_t::digest(property_binlog));
    return proposal_table;
}

xstatistics_data_t xfulltable_builder_t::make_block_statistics(const std::vector<xblock_ptr_t> & blocks) {
    // TODO(jimmy) should record property
    data::xstatistics_data_t _statistics_data = tableblock_statistics(blocks);
    return _statistics_data;
}

xblock_ptr_t        xemptytable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_emptyblock(prev_block.get(), cs_para);
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);
    return proposal_table;
}

NS_END2
