// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xstatectx/xunitstate_ctx.h"

NS_BEG2(top, statectx)

xunitstate_ctx_t::xunitstate_ctx_t(data::xunitstate_ptr_t const& unitstate, base::xaccount_index_t const& accoutindex)
: m_accountstate(std::make_shared<data::xaccount_state_t>(unitstate, accoutindex)) {
}

NS_END2
