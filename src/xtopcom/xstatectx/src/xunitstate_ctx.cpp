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
#include "xstatectx/xunitstate_ctx.h"

NS_BEG2(top, statectx)

xunitstate_ctx_t::xunitstate_ctx_t(const data::xunitstate_ptr_t & unitstate)
: m_cur_unitstate(unitstate) {
}

xunitstate_ctx_t::xunitstate_ctx_t(const data::xunitstate_ptr_t & unitstate, const data::xblock_ptr_t & prev_block)
: m_cur_unitstate(unitstate), m_prev_block(prev_block) {
}

NS_END2
