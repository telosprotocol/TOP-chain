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

std::string xlighttable_builder_t::make_light_table_binlog(const xobject_ptr_t<base::xvbstate_t> & proposal_bstate,
                                                           const std::vector<xblock_ptr_t> & units) {
    xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();

    data::xtable_bstate_t proposal_tbstate(proposal_bstate.get());

    // make account index property binlog
    for (auto & unit : units) {
        bool has_unconfirm_sendtx = false;
        if (unit->get_block_class() == base::enum_xvblock_class_nil) {  // nil unit should read last unconfirm state
            xaccount_index_t _old_aindex;
            bool ret = proposal_tbstate.get_account_index(unit->get_account(), _old_aindex);
            xassert(ret == true);  // must can read success
            has_unconfirm_sendtx = _old_aindex.is_has_unconfirm_tx();
        } else if (unit->get_block_class() == base::enum_xvblock_class_full) {
            has_unconfirm_sendtx = false;
        } else {
            has_unconfirm_sendtx = unit->get_unconfirm_sendtx_num() != 0;
        }
        xaccount_index_t _aindex(unit.get(), has_unconfirm_sendtx, false);
        proposal_tbstate.set_account_index(unit->get_account(), _aindex, canvas.get());
    }

    // make receiptid property binlog
    base::xreceiptid_check_t receiptid_check;
    xblock_t::batch_units_to_receiptids(units, receiptid_check);  // units make changed receiptids

    base::xreceiptid_pairs_ptr_t modified_pairs = make_object_ptr<base::xreceiptid_pairs_t>();

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
    std::string property_binlog;
    canvas->encode(property_binlog);
    xassert(!property_binlog.empty());

    xdbg("jimmy xlighttable_builder_t::make_light_table_binlog units_size=%zu,sendids=%zu,recvids=%zu,confirmids=%zu,all=%zu,binlog_size=%zu",
        units.size(), sendids.size(), recvids.size(), confirmids.size(), all_pairs.size(), property_binlog.size());

    return property_binlog;
}

xblock_ptr_t        xlighttable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xlighttable_builder_para_t> lighttable_build_para = std::dynamic_pointer_cast<xlighttable_builder_para_t>(build_para);
    xassert(lighttable_build_para != nullptr);

    base::xauto_ptr<base::xvblock_t> _temp_block = data::xblocktool_t::create_next_emptyblock(prev_block.get());
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_block.get(), *prev_bstate.get());

    std::string property_binlog = make_light_table_binlog(proposal_bstate, lighttable_build_para->get_batch_units());
    xtable_block_para_t lighttable_para;
    lighttable_para.set_property_binlog(property_binlog);
    lighttable_para.set_batch_units(lighttable_build_para->get_batch_units());
    lighttable_para.set_extra_data(cs_para.get_extra_data());
    std::string fullstate_bin;
    proposal_bstate->take_snapshot(fullstate_bin);
    lighttable_para.set_fullstate_bin(fullstate_bin);

    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_tableblock(lighttable_para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);

    xdbg("xlighttable_builder_t::build_block %s,account=%s,height=%ld,binlog_size=%zu,binlog=%ld,state_size=%zu",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        property_binlog.size(), base::xhash64_t::digest(property_binlog), fullstate_bin.size());
    return proposal_table;
}

std::string xfulltable_builder_t::make_binlog(const xblock_ptr_t & prev_block,
                                                const xobject_ptr_t<base::xvbstate_t> & prev_bstate) {
    base::xauto_ptr<base::xvblock_t> _temp_block = data::xblocktool_t::create_next_emptyblock(prev_block.get());
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_block.get(), *prev_bstate.get());

    std::string property_snapshot;
    auto canvas = proposal_bstate->rebase_change_to_snapshot();  // TODO(jimmy)
    canvas->encode(property_snapshot);
#if 1 //test
xobject_ptr_t<base::xvbstate_t> test_bstate = make_object_ptr<base::xvbstate_t>(*_temp_block.get());
bool ret_apply = test_bstate->apply_changes_of_binlog(property_snapshot);
xassert(ret_apply == true);
#endif
    return property_snapshot;
}


xblock_ptr_t        xfulltable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xfulltable_builder_para_t> fulltable_build_para = std::dynamic_pointer_cast<xfulltable_builder_para_t>(build_para);
    xassert(fulltable_build_para != nullptr);

    xstatistics_data_t block_statistics = make_block_statistics(fulltable_build_para->get_blocks_from_last_full());
    std::string property_binlog = make_binlog(prev_block, prev_bstate);

    xfulltable_block_para_t fulltable_para(property_binlog, block_statistics);
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


NS_END2
