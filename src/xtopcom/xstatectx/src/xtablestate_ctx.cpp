// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xtablestate_ctx.h"

NS_BEG2(top, statectx)


// ================ xtablestate_ctx_t =========
xtablestate_ctx_t::xtablestate_ctx_t(const data::xtablestate_ptr_t & table_state)
: m_table_state(table_state) {
    xassert(table_state->get_block_height() > 0);
}

uint64_t xtablestate_ctx_t::get_table_commit_height() const {
    if (m_table_state->get_block_height() <= 3) {
        return 0;
    }
    return m_table_state->get_block_height() - 3;
}

NS_END2
