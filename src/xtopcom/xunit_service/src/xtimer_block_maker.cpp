// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xvblock.h"
#include "xdata/xrootblock.h"
#include "xunit_service/xtimer_block_maker.h"

NS_BEG2(top, xunit_service)

xtimer_block_maker_t::xtimer_block_maker_t(const std::shared_ptr<xcons_service_para_face> para) : m_param(para) {
}

base::xauto_ptr<base::xvblock_t> xtimer_block_maker_t::get_latest_block(const std::string &account) {
    auto block_store = m_param->get_resources()->get_vblockstore();
    auto latest_block = block_store->get_latest_cert_block(account);
    return latest_block;
}

base::xvblock_t *xtimer_block_maker_t::make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t &leader_xip) {
    auto                           prev_block = get_latest_block(account);
    base::xvblock_t *              block = data::xemptyblock_t::create_next_emptyblock(prev_block.get(), base::enum_xvblock_type_clock);
    block->get_cert()->set_clock(clock);
    block->get_cert()->set_viewid(viewid);
    block->get_cert()->set_validator(leader_xip);
    block->reset_prev_block(prev_block.get());
    return block;
}

base::xvblock_t *xtimer_block_maker_t::make_block(const std::string &account, const xblock_maker_para_t &para, const xvip2_t &leader_xip) {
    xassert(0); // TODO(jimmy) need rewrite
    return nullptr;
}

int xtimer_block_maker_t::verify_block(base::xvblock_t *proposal_block) {
    // no application level checking
    return 0; // OK
}

NS_END2
