// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"


NS_BEG2(top, data)

template <xtop_action_type_t ActionTypeV>
class xtop_consensus_action;

template <xtop_action_type_t ActionTypeV>
using xconsensus_action_t = xtop_consensus_action<ActionTypeV>;

NS_END2
