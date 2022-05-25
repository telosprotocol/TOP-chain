// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xnode_id.h"

NS_BEG2(top, common)

bool is_t0(xaccount_address_t const & account_address);
bool is_t2(xaccount_address_t const & account_address);
bool is_t8(xaccount_address_t const & account_address);
bool is_t6(xaccount_address_t const & account_address);

NS_END2
