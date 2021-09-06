// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
class xtop_action_session;

template <typename ActionT>
using xaction_session_t = xtop_action_session<ActionT>;

NS_END2
