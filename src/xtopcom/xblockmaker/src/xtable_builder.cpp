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

NS_BEG2(top, blockmaker)

xblock_ptr_t        xlighttable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xaccount_ptr_t & prev_state,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xlighttable_builder_para_t> lighttable_build_para = std::dynamic_pointer_cast<xlighttable_builder_para_t>(build_para);
    xassert(lighttable_build_para != nullptr);

    xtable_block_para_t lighttable_para;
    lighttable_para.set_batch_units(lighttable_build_para->get_batch_units());
    lighttable_para.set_extra_data(cs_para.get_extra_data());
    base::xvblock_t* _proposal_block = data::xtable_block_t::create_next_tableblock(lighttable_para, prev_block.get());
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);
    proposal_table->set_consensus_para(cs_para);
    return proposal_table;
}

xblock_ptr_t        xfulltable_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xaccount_ptr_t & prev_state,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xfulltable_builder_para_t> fulltable_build_para = std::dynamic_pointer_cast<xfulltable_builder_para_t>(build_para);
    xassert(fulltable_build_para != nullptr);

    // TODO(jimmy) always create now highqc binlog
    const xtable_mbt_binlog_ptr_t & commit_binlog = fulltable_build_para->get_latest_commit_index_binlog();
    xtable_mbt_binlog_ptr_t highqc_binlog = make_object_ptr<xtable_mbt_binlog_t>(commit_binlog->get_accounts_index());

    auto & uncommit_blocks = fulltable_build_para->get_uncommit_blocks();
    xassert(uncommit_blocks.size() == 2);
    xassert(uncommit_blocks[1]->get_height() + 1 == uncommit_blocks[0]->get_height());
    xassert(uncommit_blocks[0]->get_height() == prev_block->get_height());

    for (auto iter = uncommit_blocks.rbegin(); iter != uncommit_blocks.rend(); iter++) {
        auto & block = *iter;
        std::map<std::string, xaccount_index_t> changed_indexs = block->get_units_index();
        xassert(!changed_indexs.empty());
        highqc_binlog->set_accounts_index(changed_indexs);
    }

    std::string block_statistics = make_block_statistics(fulltable_build_para->get_blocks_from_last_full());

    xfulltable_block_para_t fulltable_para(fulltable_build_para->get_last_full_index(), highqc_binlog);
    fulltable_para.set_block_statistics_data(block_statistics);
    xdbg("xfulltable_builder_t::build_block %s,lastroot=%s,binlog=%d,%d,newroot=%s,statistics_size=%d",
        cs_para.dump().c_str(), base::xstring_utl::to_hex(fulltable_build_para->get_last_full_index()->get_root_hash()).c_str(),
        highqc_binlog->get_account_size(), commit_binlog->get_account_size(), base::xstring_utl::to_hex(fulltable_para.get_new_state_root()).c_str(), block_statistics.size());

    base::xvblock_t* _proposal_block = data::xfull_tableblock_t::create_next_block(fulltable_para, prev_block.get());
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);
    proposal_table->set_consensus_para(cs_para);
    return proposal_table;
}

std::string xfulltable_builder_t::make_block_statistics(const std::vector<xblock_ptr_t> & blocks) {
    // data::xstatistics_data_t _statistics_data = tableblock_statistics(blocks, nodesvr_ptr);
    return {};
}


NS_END2
