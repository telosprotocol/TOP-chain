// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnetwork/xdummy_chain_timer.h"

NS_BEG3(top, tests, vnetwork)

top::xobject_ptr_t<xdummy_chain_timer_t> xdummy_chain_timer{ top::make_object_ptr<xdummy_chain_timer_t>() };

NS_END3
