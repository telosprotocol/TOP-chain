#pragma once

#include <string>
#include "xbase/xmem.h"
#include "xdata/xblocktool.h"

using namespace top;
using namespace top::data;

class test_blocktuil {
 public:
    static base::xvblock_t*   create_genesis_empty_unit(const std::string & account) {
        return xblocktool_t::create_genesis_empty_unit(account);
    }
    static base::xvblock_t*   create_genesis_empty_table(const std::string & account) {
        return xblocktool_t::create_genesis_empty_table(account);
    }
    static base::xvblock_t*   create_genesis_lightunit(const std::string & account, int64_t top_balance) {
        return xblocktool_t::create_genesis_lightunit(account, top_balance);
    }
    static base::xvblock_t*   create_genesis_lightunit(const std::string & account,
                                                       const xtransaction_ptr_t & genesis_tx,
                                                       const xtransaction_result_t & result) {
        return xblocktool_t::create_genesis_lightunit(account, genesis_tx, result);
    }
    static base::xvblock_t*   create_next_emptyblock(base::xvblock_t* prev_block, uint64_t clock = 0, uint64_t viewid = 0) {
        auto next_block = xblocktool_t::create_next_emptyblock(prev_block);
        do_block_consensus(next_block, clock, viewid);
        return next_block;
    }
    static base::xvblock_t*   create_next_emptyblock_with_cert_flag(base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_emptyblock(prev_block);
        do_block_consensus(next_block, clock);
        next_block->reset_block_flags();
        next_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        return next_block;
    }
    static base::xvblock_t*   create_next_emptyblock_with_justify(base::xvblock_t* prev_block, base::xvblock_t* justify_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_emptyblock(prev_block);
        next_block->get_cert()->set_justify_cert_hash(justify_block->get_cert()->get_output_root_hash());
        do_block_consensus(next_block, clock);
        return next_block;
    }
    static base::xvblock_t*   create_next_lightunit(const xlightunit_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_lightunit(para, prev_block);
        return next_block;
    }
    static base::xvblock_t*   create_next_fullunit(const xfullunit_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_fullunit(para, prev_block);
        return next_block;
    }
    static base::xvblock_t*   create_next_lightunit_with_consensus(const xlightunit_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_lightunit(para, prev_block);
        do_block_consensus(next_block, clock);
        return next_block;
    }
    static base::xvblock_t*   create_next_fullunit_with_consensus(const xfullunit_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_fullunit(para, prev_block);
        do_block_consensus(next_block, clock);
        return next_block;
    }
    static base::xvblock_t*   create_next_tableblock(xtable_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        for (auto & unit : para.get_account_units()) {
            set_block_consensus_para(unit.get(), clock, prev_block->get_viewid() + 1);
            xdbg("test_blocktuil::create_next_tableblock unit. header:%s, cert:%s, viewid:%ld",
            unit->dump_header().c_str(), unit->dump_cert().c_str(), prev_block->get_viewid() + 1);
        }
        auto next_block = xblocktool_t::create_next_tableblock(para, prev_block);
        // if (prev_block->get_prev_block() != nullptr) {
        //     next_block->get_cert()->set_justify_cert_hash(prev_block->get_prev_block()->get_output_root_hash());
        // }
        do_block_consensus(next_block, clock, prev_block->get_viewid() + 1);
        return next_block;
    }
    static base::xvblock_t*   create_next_indextable_with_consensus(const xfulltable_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        auto next_block = xblocktool_t::create_next_fulltable(para, prev_block);
        do_block_consensus(next_block, clock);
        return next_block;
    }
    static base::xvblock_t*   create_next_tableblock_with_next_two_emptyblock(xtable_block_para_t & para, base::xvblock_t* prev_block, uint64_t clock = 0) {
        for (auto & unit : para.get_account_units()) {
            set_block_consensus_para(unit.get(), clock);
        }
        auto commit_block = xblocktool_t::create_next_tableblock(para, prev_block);
        do_block_consensus(commit_block, clock);
        // commit_block->reset_prev_block(prev_block);

        base::xauto_ptr<base::xvblock_t> lock_block(create_next_emptyblock(commit_block));
        // commit_block->reset_next_block(lock_block.get());
        // lock_block->reset_prev_block(commit_block);

        base::xauto_ptr<base::xvblock_t> highqc_block(create_next_emptyblock(lock_block.get()));
        // lock_block->reset_next_block(highqc_block.get());
        // highqc_block->reset_prev_block(lock_block.get());

        return commit_block;
    }
    static base::xvblock_t*   create_next_emptyblock(xblockchain2_t* chain) {
        auto next_block = xblocktool_t::create_next_emptyblock(chain);
        do_block_consensus(next_block);
        return next_block;
    }
    static base::xvblock_t*   create_next_lightunit(const xlightunit_block_para_t & para, xblockchain2_t* chain) {
        auto next_block = xblocktool_t::create_next_lightunit(para, chain);
        do_block_consensus(next_block);
        return next_block;
    }
    static base::xvblock_t*   create_next_fullunit(xblockchain2_t* chain) {
        auto next_block = xblocktool_t::create_next_fullunit(chain);
        do_block_consensus(next_block);
        return next_block;
    }

 public:
    static void set_block_consensus_para(base::xvblock_t* block, uint64_t clock = 0, uint64_t viewid = 0) {
        xassert(block->get_block_hash().empty());

        if (clock != 0) {
            block->get_cert()->set_clock(clock);
        }
        if (viewid != 0) {
            block->get_cert()->set_viewid(viewid);
        }
        //  else {
        //     block->get_cert()->set_viewid(block->get_viewid() + 1);
        // }
        block->get_cert()->set_validator({1, (uint64_t)-1});
        block->get_cert()->set_viewtoken(1111);
    }
    static void do_block_consensus(base::xvblock_t* block, uint64_t clock = 0, uint64_t viewid = 0) {
        xassert(block->get_block_hash().empty());

        do_block_consensus_without_set_flag(block, clock, viewid);

        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        block->set_block_flag(base::enum_xvblock_flag_locked);
        block->set_block_flag(base::enum_xvblock_flag_committed);
        xassert(!block->get_block_hash().empty());
    }
    static void do_block_consensus_with_cert_flag(base::xvblock_t* block, uint64_t clock = 0, uint64_t viewid = 0) {
        xassert(block->get_block_hash().empty());

        do_block_consensus_without_set_flag(block, clock, viewid);

        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(!block->get_block_hash().empty());
    }
    static void do_block_consensus_without_set_flag(base::xvblock_t* block, uint64_t clock = 0, uint64_t viewid = 0) {
        xassert(block->get_block_hash().empty());
        set_block_consensus_para(block, clock, viewid);

        if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
            block->set_extend_cert("1");
            block->set_extend_data("1");
        } else {
            block->set_verify_signature("1");
        }
    }
};
