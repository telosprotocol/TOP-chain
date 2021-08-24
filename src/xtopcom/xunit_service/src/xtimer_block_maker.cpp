// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvledger/xvblock.h"
#include "xdata/xrootblock.h"
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xunit_service/xtimer_block_maker.h"

NS_BEG2(top, xunit_service)

xtimer_block_maker_t::xtimer_block_maker_t(const std::shared_ptr<xcons_service_para_face> para) : m_param(para) {
}

base::xauto_ptr<base::xvblock_t> xtimer_block_maker_t::get_latest_block(const std::string &account) {
    auto block_store = m_param->get_resources()->get_vblockstore();
    auto latest_block = block_store->get_latest_cert_block(account, metrics::blockstore_access_from_us_timer_blk_maker);
    return latest_block;
}

base::xvblock_t *xtimer_block_maker_t::make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t &leader_xip) {
    auto                           prev_block = get_latest_block(account);
    if (clock < prev_block->get_clock()) {
        xunit_warn("xtimer_block_maker_t::make_block fail-clock cur=%ull,prev=%ull", clock, prev_block->get_clock());
        return nullptr;
    }

    uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
    xblock_consensus_para_t cs_para(account, clock, viewid, viewtoken, prev_block->get_height() + 1);
    cs_para.set_validator(leader_xip);

    base::xvblock_t *              block = data::xblocktool_t::create_next_emptyblock(prev_block.get(), cs_para);
    block->reset_prev_block(prev_block.get());
    return block;
}

int xtimer_block_maker_t::verify_block(base::xvblock_t *proposal_block) {
    // no application level checking
    return 0; // OK
}

NS_END2
