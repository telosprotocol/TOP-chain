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

    xstatistics_data_t block_statistics = make_block_statistics(fulltable_build_para->get_blocks_from_last_full());
    xfulltable_block_para_t fulltable_para(fulltable_build_para->get_latest_offstate(), block_statistics);

    xdbg("xfulltable_builder_t::build_block %s,full_height=%ld,binlog_height=%ld",
        cs_para.dump().c_str(), fulltable_build_para->get_latest_offstate()->get_full_height(), fulltable_build_para->get_latest_offstate()->get_binlog_height());

    base::xvblock_t* _proposal_block = data::xfull_tableblock_t::create_next_block(fulltable_para, prev_block.get());
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);
    proposal_table->set_consensus_para(cs_para);
    return proposal_table;
}

xstatistics_data_t xfulltable_builder_t::make_block_statistics(const std::vector<xblock_ptr_t> & blocks) {
    // TODO(jimmy) should record property
    data::xstatistics_data_t _statistics_data = tableblock_statistics(blocks);
    return _statistics_data;
}


NS_END2
