// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xtable_builder.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xfulltable_statistics.h"
#include "xchain_fork/xutility.h"
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
        xinfo("make_table_prove_property_hashs hash1=%s", base::xstring_utl::to_hex(prophash).c_str());
    }
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
    std::shared_ptr<xfulltable_builder_para_t> fulltable_build_para = std::dynamic_pointer_cast<xfulltable_builder_para_t>(build_para);
    xassert(fulltable_build_para != nullptr);

    auto & blocks = fulltable_build_para->get_blocks_from_last_full();

    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block.get(), cs_para.get_clock());

    std::map<std::string, std::string> property_hashs;
    std::string property_binlog;
    make_binlog(_temp_header, prev_bstate, property_binlog, property_hashs);

    int64_t tgas_balance_change_total = 0;
    for(auto & block : blocks) {
        if (block->get_block_class() == base::enum_xvblock_class_light) {
            int64_t tgas_balance_change = block->get_pledge_balance_change_tgas();
            tgas_balance_change_total += tgas_balance_change;
            xdbg("tgas_balance_change_total=%lld, cur=%lld, account=%s", tgas_balance_change_total, tgas_balance_change, block->dump().c_str());
        }
    }
    
    #ifndef  XBUILD_CONSORTIUM
        data::xstatistics_data_t block_statistics = make_block_statistics(blocks);
        data::xfulltable_block_para_t fulltable_para(property_binlog, block_statistics, tgas_balance_change_total);
    #else
        data::xstatistics_cons_data_t block_statistics = make_block_statistics_cons(blocks);
        data::xfulltable_block_para_t fulltable_para(property_binlog, block_statistics, tgas_balance_change_total);
    #endif

    fulltable_para.set_property_hashs(property_hashs);
    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_fulltable(fulltable_para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_table;
    proposal_table.attach((data::xblock_t*)_proposal_block);

    xdbg("xfulltable_builder_t::build_block %s,account=%s,height=%ld,binlog_size=%zu,binlog=%ld",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        property_binlog.size(), base::xhash64_t::digest(property_binlog));
    return proposal_table;
}

data::xstatistics_data_t xfulltable_builder_t::make_block_statistics(const std::vector<xblock_ptr_t> & blocks) {
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

data::xstatistics_cons_data_t xfulltable_builder_t::make_block_statistics_cons(const std::vector<xblock_ptr_t> & blocks){
    data::xstatistics_cons_data_t _statistics_data = tableblock_statistics_consortium(blocks);
    return _statistics_data;
}

NS_END2
